# 애플리케이션 종료 시 크래시 분석 — `PResourceStateTracker::UnRegisterResource`

작성일: 2026-09-17
대상 콜스택: `GCoreSystem::Destroy()` → `GModuleGlobalSystem::Destroy()` → GC Flush → `~PDX12Texture()` → `UnRegisterResource` → `std::_Hash::_Find_last` 크래시

---

## 1. 결론 (요약)

**이미 파괴된 `PDirectX12API` 인스턴스에 대한 use-after-free.**

`HDirectXAPI::getDX12API()`가 `PDirectX12API*`를 **함수 지역 static에 캐시해두고 절대 무효화하지 않는다.**
종료 시 순서가 이렇게 흘러간다:

1. `HJGGraphicsModule::ShutdownModule()`이 `_graphicsAPI = nullptr` 로 참조를 끊는다 → `PDirectX12API`의 RefCount == 0
2. `GModuleGlobalSystem::Destroy()`가 그 다음 줄에서 `GMemoryGlobalSystem::Flush()` 호출
3. GC가 **FIFO 큐** 순서로 스윕 → 부팅 초기에 할당된 `PDirectX12API`가 먼저 파괴되고 메모리 풀로 반납됨
4. 같은 Flush 루프의 **뒤쪽 스윕**에서 `PDX12Texture` 들이 파괴됨
5. `~PDX12Texture()` → `HDirectXAPI::DestroyCommittedResource()` → `getDX12API()`가 **죽은 포인터(`cachedAPI`)를 그대로 반환**
6. 그 포인터로 `_resourceRefCache` (std::unordered_map)에 `find()` 수행 → 해제된 버킷 리스트를 순회 → `_Find_last`에서 크래시

`_Find_last`에서 죽는 게 결정적 증거다. 맵이 그냥 비어 있었다면 `find()`는 `end()`를 조용히 반환한다. 버킷 체인을 따라가다 죽었다는 건 **맵 객체 자체의 메모리가 이미 해제/재사용되었다**는 뜻이다.

---

## 2. 콜스택 라인별 대조

| 스택 프레임 | 실제 코드 | 비고 |
|---|---|---|
| `main()` Main.cpp:15 | — | |
| `GCoreSystem::Destroy()` CoreSystem.cpp:94 | `SystemInstanceList[i]->Destroy();` | 시스템 역순 파괴 |
| `GModuleGlobalSystem::Destroy()` Module.cpp:192 | `GMemoryGlobalSystem::GetInstance().Flush();` | **모든 모듈 Shutdown 이후** |
| `GMemoryGlobalSystem::Flush()` Memory.cpp:26 | `garbageCollection(0);` | |
| `garbageCollection(int)` Memory.cpp:56 | `while (garbageCollectionInternal(...)) {}` | 다중 스윕 |
| `garbageCollectionInternal(...)` Memory.cpp:97 | `((IMemoryObject*)ptr)->~IMemoryObject();` | |
| `~PDX12Texture()` DX12Texture.cpp:9 | `Reset();` | |
| `PDX12Texture::Reset()` DX12Texture.cpp:67 | `HDirectXAPI::DestroyCommittedResource(_dx12Resource);` | |
| `HDirectXAPI::DestroyCommittedResource` DirectX12API.cpp:389 | `return getDX12API()->DestroyCommittedResource(resource);` | **null 체크 없음** |
| `PDirectX12API::DestroyCommittedResource` DirectX12API.cpp:284 | `PResourceStateTracker::UnRegisterResource(resource.Get());` | |
| `UnRegisterResource` ResourceStateTracker.cpp:244 | `auto iter = resourceRefMap.find(d3dResource);` | **여기서 크래시** |
| `_Hash::_Find_last` xhash:1565 | — | 해제된 버킷 순회 |

모든 줄 번호가 정확히 일치한다.

---

## 3. 원인 상세

### (A) 주 원인 — `getDX12API()` static 캐시가 무효화되지 않음

`Source/Runtime/Graphics/DirectX12/DirectX12API.cpp:472`

```cpp
PDirectX12API* HDirectXAPI::getDX12API()
{
    static PDirectX12API* cachedAPI = nullptr;   // ← 한 번 채워지면 영원히 유지

    if (cachedAPI == nullptr)
    {
        ...
        cachedAPI = Dx12API.GetRawPointer();     // ← 소유권 없는 raw 포인터
    }

    return cachedAPI;                            // ← 파괴 후에도 그대로 반환
}
```

`PDirectX12API`는 `PJGGraphicsAPI : public IMemoryObject` 이므로 **엔진 GC가 관리하는 블록**이다
(`JGGraphics.cpp:25` `_graphicsAPI = Allocate<PDirectX12API>()`).
GC가 이 블록을 파괴하고 `MemoryPool.Deallocate(ptr)`로 풀에 반납해도 `cachedAPI`는 그 주소를 계속 들고 있다.

동일한 패턴이 `JGGraphics.cpp:87` `GetGraphicsAPI()`에도 있다 (`static PJGGraphicsAPI* API`).

### (B) 주 원인 — 파괴 순서: 그래픽스 API가 GPU 리소스 소유자보다 먼저 죽는다

`Source/Runtime/Core/Misc/Module.cpp:185`

```cpp
void GModuleGlobalSystem::Destroy()
{
    for (auto& pair : _modulesByType)
        pair.second->ShutdownModule();      // 1) 모든 모듈 Shutdown (그래픽스 API 파괴 예약)

    GMemoryGlobalSystem::GetInstance().Flush();   // 2) 그 다음에야 GPU 리소스들이 GC됨
    ...
}
```

`ShutdownModule()`에서 `_graphicsAPI = nullptr` 만 하고 실제 파괴는 GC에 맡기는데,
**같은 GC Flush 안에서** 텍스처/버퍼들도 함께 파괴된다.
`AllocatedMemoryBlockQueue`는 `std::queue<void*>` (FIFO)이고 `PDirectX12API`는 부팅 시점(StartupModule)에 할당되어 큐 앞쪽에 있으므로 **항상 먼저 파괴된다.**

### (C) 보조 원인 — `HDirectXAPI::` 정적 래퍼 전체에 null 가드가 없다

`getDX12API()`는 명시적으로 `nullptr`을 반환할 수 있는데(모듈 미탑재 / API 캐스트 실패), 래퍼 20여 개가 전부 무조건 역참조한다:

```cpp
void HDirectXAPI::DestroyCommittedResource(HDX12ComPtr<HDX12Resource> resource)
{
    return getDX12API()->DestroyCommittedResource(resource);   // nullptr 검사 없음
}
```

즉 (A)의 캐시만 고치면 **use-after-free가 nullptr 역참조 크래시로 바뀔 뿐**이다. (A)와 (C)는 같이 고쳐야 한다.

### (D) 보조 원인 — 모듈 파괴 순서가 비결정적

`_modulesByType`은 `HHashMap<JGType, IModuleInterface*>` — **해시 순회 순서 = 비결정적.**
`GModuleGlobalSystem::Destroy()`에 의존성 역순 보장이 전혀 없다.
실제 로그에서도 `Graphics` 모듈이 `JGDev_Graphics` 모듈보다 **먼저** Shutdown됐다 (아래 §4).
이 순서는 빌드/모듈 구성에 따라 뒤바뀔 수 있어, 크래시가 간헐적으로 보이거나 사라질 수 있다.

### 같은 경로를 타는 다른 타입들

텍스처가 우연히 먼저 걸렸을 뿐, 아래 전부 동일한 크래시 경로를 가진다:

- `DX12Texture.cpp:67`
- `DX12VertexBuffer.cpp:102`
- `DX12IndexBuffer.cpp:103`
- `DX12ConstantBuffer.cpp:96`
- `DX12StructuredBuffer.cpp:101`
- `Classes/UploadAllocator.cpp:30`

---

## 4. 로그 증거

`Bin/DevelopEngine/jg_log.txt` (마지막 실행 구간)

```
L88384  [trace][Graphics]: Shutdown Graphics Module...        ← 그래픽스 API 참조 해제
L88385~ [info][Memory]: Deallocated ... (수천 줄)              ← GC Flush 스윕
L88441  [trace][JGDev_GraphicsModule]: Shutdown JGDev_GraphicsModule Module...
L88522  (로그 끝 — 정상 종료 로그 없이 중단)
```

- `Shutdown Graphics Module...` **이후** `Remove Counting Resource` / `UnRegister ... in ResourceRefMap` 로그가 **단 한 줄도 없다.**
  → `UnRegisterResource`의 `find()`가 정상 동작한 적이 없다는 뜻.
- 로그가 정상 종료 메시지 없이 GC 스윕 도중 끊겼다.
- `Graphics` 모듈이 `JGDev_Graphics` 모듈보다 먼저 Shutdown된 것도 확인된다 → (D) 뒷받침.

---

## 5. 수정 방향 (우선순위)

### P0 — static 캐시 제거 + null 가드

`getDX12API()`의 `static PDirectX12API* cachedAPI`를 제거하고 매번 모듈에서 조회하도록 바꾼다.
동시에 `HDirectXAPI::`의 모든 래퍼에 null 가드를 넣는다. 최소한 `DestroyCommittedResource`는 반드시:

```cpp
void HDirectXAPI::DestroyCommittedResource(HDX12ComPtr<HDX12Resource> resource)
{
    PDirectX12API* api = getDX12API();
    if (api == nullptr) return;          // 종료 중 — 추적 맵이 이미 없음. 조용히 무시
    api->DestroyCommittedResource(resource);
}
```

`JGGraphics.cpp:87`의 `GetGraphicsAPI()` static 캐시도 동일하게 처리한다.

> 캐시 제거로 인한 조회 비용이 문제라면, `ShutdownModule()`에서 캐시를 명시적으로 무효화하는
> `HDirectXAPI::InvalidateCache()` 를 두는 방법도 있다. 다만 (D) 때문에 모듈 간 호출 순서에
> 의존하게 되므로 **캐시 제거 쪽이 안전하다.**

### P1 — 종료 시 리소스 추적을 명시적으로 끈다

`PDirectX12API::Destroy()`가 이미 `_resourceRefCache.clear()`를 하고 있다.
즉 종료 시점의 Register/UnRegister 카운팅은 **이미 의미가 없다.**
전역 플래그(예: `PResourceStateTracker::Shutdown()`)로 추적을 비활성화해서,
남은 리소스 파괴가 추적 맵을 아예 건드리지 않게 하는 것이 가장 깔끔하다.

### P2 — 모듈 파괴 순서 결정화

`_modulesByType` 해시 순회 대신 **연결 순서를 기록해 역순으로 Shutdown** 한다.
`ConnectModule()`에서 `HList<IModuleInterface*> _moduleOrder` 에 push,
`Destroy()`에서 역순 순회. (D)를 근본적으로 없앤다.

### P3 — 그래픽스 API를 GC 큐에서 빼기

`PDirectX12API`를 GC 관리 대상에서 제외하고 `ShutdownModule()`에서 **동기적으로 마지막에** 파괴하면,
"GC 큐 순서 때문에 API가 먼저 죽는" 구조적 위험 자체가 사라진다. (구조 변경이 커서 후순위)

---

## 5-1. 적용 결과 (2026-09-17, P0/3단계 적용 완료)

위 **P0 (static 캐시 제거 + null 가드)** 를 적용했다. 변경 파일 3개.

### 적용 내용

**`DirectX12API.h` / `DirectX12API.cpp`**

- `HDirectXAPI` 에 캐시 수명 제어를 추가하고 `PDirectX12API` 를 friend 로 지정했다.
  - `resetCache()` — `PDirectX12API::Initialize()` 에서 호출. 캐시를 비우고 조회를 다시 허용한다.
  - `invalidateCache(owner)` — `PDirectX12API::Destroy()` **말미**와 소멸자에서 호출.
    캐시를 버리고 이후 조회를 전부 막는다. `owner` 인자는 교체된 새 인스턴스의 캐시를
    죽는 인스턴스가 지우지 않게 한다.
- `~PDirectX12API()` 를 `= default` 에서 실체 있는 소멸자로 바꿨다. (캐시 무효화 안전망)
- `HDirectXAPI::` 정적 래퍼 **21개 전부**에 null 가드를 넣었다.
  - void → 조기 return / 포인터 → `nullptr` / 값 → 기본 생성 값
  - 참조 반환 7개 → 비어 있는 폴백 인스턴스. `find()` 가 전부 `end()` 로 떨어져 조용히 no-op 이 된다.
    폴백은 **함수 지역 static** 이라 폴백 경로를 한 번도 안 타면 아예 생성되지 않는다.
    const / non-const 게터 쌍이 같은 인스턴스를 보도록 접근자 함수로 감쌌다.
  - 폴백 진입은 `logFallbackOnce()` 로 딱 한 번만 로그를 남긴다. (종료 시 리소스 수천 개가 이 경로를 탄다)

> **캐시를 완전히 제거하지 않은 이유.**
> 매번 `FindModule` 로 찾는 방법도 검토했으나, `FindModule` 이 뮤텍스를 잡는데
> `HDirectXAPI::GetDevice()` 가 `DynamicDescriptionAllocator` 의 디스크립터 커밋 등
> **프레임당 수천 번 도는 경로**에 있다. 그래서 캐시는 유지하되 수명을 `PDirectX12API` 가
> 직접 관리하도록 했다. 외부 호출자가 무효화를 기억해야 하는 구조가 아니라
> **객체가 자기 죽음을 스스로 알리는** 구조라 빠뜨릴 여지가 없다.

**`JGGraphics.cpp`**

- 전역 `GetGraphicsAPI()` 의 `static PJGGraphicsAPI* API` 캐시를 **제거**했다.
  호출처가 에셋/씬 생성 경로뿐이라 매번 조회해도 비용이 문제되지 않는다.
  `PJGGraphicsAPI` 가 추상 클래스라 폴백 인스턴스를 만들 수 없어, null 이면
  Critical 로그 + `JG_CHECK` 로 크게 알린다. (오늘처럼 조용히 dangling 참조를 주는 것보다 낫다)
- `HJGGraphicsModule::ShutdownModule()` 의 `_graphicsAPI` null 가드를 추가했다. (아래 5-2 참고)

### 검증

- `Graphics.vcxproj` 빌드 성공. `JGEngine.sln` **전체 빌드 성공 (에러 0)**.
  남은 경고 2개(`DX12GraphicsCommand.cpp` C4244)와 LNK4098 은 이번 변경과 무관한 기존 경고다.
- `JGLauncher.exe` 실행 후 WM_CLOSE 로 정상 종료 → **ExitCode 0**.
- 로그가 종료 시퀀스 **끝까지 완주**한다.
  - 수정 전: GC 스윕 도중 로그가 끊김 (크래시)
  - 수정 후: `[Memory]: [ID: 0] Memory Chunk Shutdown.` 까지 도달
    (`GMemoryGlobalSystem::~GMemoryGlobalSystem()` → `MemoryPool.Shutdown()`, 종료 시퀀스의 끝)

## 5-2. [정정] `ShutdownModule()` 중복 호출은 오진이었다

적용 직후 로그에서 `Startup Graphics Module...` 1회 / `Shutdown Graphics Module...` 2회를 보고
"`HJGGraphicsModule::ShutdownModule()` 이 두 번 호출된다"고 적었으나 **틀렸다.**

원인은 `Source/Runtime/Asset/AssetModule.cpp` 의 복사-붙여넣기 오타다.

```cpp
void HAssetModule::StartupModule()   { JG_LOG(Graphics, ..., "Start Graphics Module...");    }  // L15
void HAssetModule::ShutdownModule()  { JG_LOG(Graphics, ..., "Shutdown Graphics Module..."); }  // L21
```

Asset 모듈이 Graphics 모듈의 문자열을 그대로 찍는다. 그래서 2회로 보였다.
- `Startup Graphics Module...` = Graphics (JGGraphics.cpp:49)
- `Start Graphics Module...` = Asset (오타)
- `Shutdown Graphics Module...` 2회 = Graphics + Asset(오타)

`ShutdownModule()` 은 각각 한 번씩만 호출된다.
`JGGraphics.cpp` 에 넣은 `if (_graphicsAPI != nullptr)` 가드는 여전히 유효한 방어이지만
(§5-3 시나리오 2), "확정 크래시를 막고 있다"는 앞선 서술은 취소한다.

## 5-3. 실제 종료 흐름 (로그로 재구성)

```
GCoreSystem::Destroy()                                        CoreSystem.cpp:94
 └ GModuleGlobalSystem::Destroy()                             Module.cpp:185
    ├ for (_modulesByType)   ← HHashMap 순회 = 해시 순서 (비결정적)
    │   └ [이번 실행] JGDev_Graphics 가 먼저 걸렸다
    │       └ HJGDev_GraphicsModule::ShutdownModule()          JGDev_Graphics.cpp:99
    │           ├ DisconnectModule("Devkit")         → Shutdown + Flush + Deallocate + erase
    │           ├ DisconnectModule("DevConsole")     → 〃
    │           ├ DisconnectModule("DevStatistics")  → 〃
    │           ├ DisconnectModule("Asset")          → 〃   L83491 (오타 로그)
    │           ├ DisconnectModule("GUI")            → 〃
    │           └ DisconnectModule("Graphics")       → 〃   L84428 (진짜 Graphics)
    │                                                        └ 이 Flush 에서 PDirectX12API 파괴
    │           L84485 "Shutdown JGDev_GraphicsModule Module..."
    ├ GMemoryGlobalSystem::Flush()                            Module.cpp:192
    │   └ 남은 텍스처/버퍼 파괴 → **원래 크래시 지점**
    └ ...
```

두 Shutdown 로그 사이의 Deallocate 937줄은 `DisconnectModule` 내부의
`GMemoryGlobalSystem::Flush()` (Module.cpp:172) 때문이다.

즉 이번 실행에서는 `JGDev_Graphics` 가 첫 순회 대상이 된 덕분에
그 안의 **수동 역순 Disconnect(Asset → GUI → Graphics)** 가 제대로 돌았다.
**순서가 맞은 것은 우연이다.** §5-4 참고.

## 6. 부가 발견 (크래시와 무관하지만 기록)

### 6-1. `JG_LOG` 포맷 플레이스홀더 불일치

`JG_LOG`는 `PString::Format` → **printf 스타일(`%s`)** 인데,
`ResourceStateTracker.cpp`의 아래 줄들은 **fmt 스타일(`{0}`)** 을 쓰고 있어 치환이 전혀 안 된다:

- `ResourceStateTracker.cpp:200, 206, 217, 249, 252`

로그에 `UnRegister {0} in ResourceRefMap` 이 그대로 찍히고 있다 (§4 참조).
**이번 크래시 디버깅에서 어떤 리소스가 문제였는지 특정할 수 없게 만든 원인**이므로 같이 고치는 게 좋다.

### 6-2. 메모리 풀 스레드 불일치 경고

```
[warning][Memory]: Dismatch Allocated Thread(3560278748) And Deallocated Thread(1760778082),
                   Memory Location : 6cf44490
```

`HMemoryPool`이 스레드별 청크(`MemoryChunkMap`)를 쓰는데, 다른 스레드에서 할당된 블록이
메인 스레드 GC에서 해제되고 있다. 지금은 경고만 내지만 **잠재적 힙 손상 요인**이다. 별건으로 추적 필요.

### 6-3. `PResourceStateTracker::UAVBarrier` 분기 반전

`ResourceStateTracker.cpp:53-66` — `d3dResource == nullptr` 일 때 `d3dResource.Get()`(=null)을 넘기고,
아닐 때 `nullptr`을 넘긴다. 조건과 인자가 서로 뒤바뀌어 있다. 이번 크래시와는 무관.
