# 메모리 시스템 할 일 목록 (순차 진행용)

작성 2026-09-28. 근거는 `Document/Memory_현황분석_2026-09-21.md`(§4 문제 목록 P0 7건·P1 9건, §5 단계 A~E)와 `Document/Memory/2026-09-21_Memory_현황분석.md`.
위에서부터 순서대로 진행한다. Phase 1(풀 무관 안전장치) → 2(풀 재설계) → 3(흡수 1차) → 4(GC 구조) → 5(흡수 2차) → 6(관측) → 7(정리) 순서이며, 각 단계 끝에 검증과 커밋 항목이 있다.
한 항목이 끝나면 `[x]`로 바꾸고, 한 단계가 끝나면 `Document/Memory/2026-09-21_Memory_현황분석.md` 끝의 "진행 기록"에 한 줄 추가한다.
`결정 필요` 표시 항목은 착수 전에 사용자 확인을 받는다. 파일:행 표기는 2026-09-28 소스 기준이므로 편집 후에는 어긋날 수 있다.
진행 상태: Phase 0·1 완료(2026-09-28, 1-5 커밋은 사용자 몫). Phase 2~7 미착수. PSO 해시 수정(`PipelineState.cpp` `computeGraphicsPSOHash`)은 커밋됨.

**순서 변경 (2026-09-28, 사용자 결정).** 풀 실패 처리를 "malloc 폴백"이 아니라 "클래스에 페이지를 더 붙이는 성장"으로 하기로 정했다(모든 블록이 청크 안에 있다는 전제를 지키기 위해). 그러면 옛 1-1(실패 처리)은 옛 4-1(성장형 풀)의 일부가 되고 옛 1-4(카운터)는 새 풀 설계에 들어가야 하므로, **풀 재설계를 Phase 2로 앞당기고** 풀 구조와 무관한 안전장치(로그 스위치·대입 의미론·static_assert)만 Phase 1에 남겼다. GC 제어 블록(옛 3)은 새 헤더 레이아웃이 정해진 뒤가 맞아 Phase 4로, 흡수 1차(옛 2)는 풀이 성장할 수 있게 된 뒤 Phase 3으로 옮겼다. 원래 번호는 각 Phase 제목에 적어 두었다.

검증 루프(모든 단계 공통):
파일을 추가·삭제했으면 `Build/BatchFiles`에서 `JGBuildTool.exe`, JGCLASS/JGPROPERTY를 바꿨으면 `JGHeaderTool.exe`(둘 다 종료 시 세그폴트 139, 산출물은 정상) → MSBuild `JGEngine.sln` DevelopEngine|x64 오류 0(경고 기준선 C4244 4건·LNK4098 1건) → `Document/Memory/tools/capture_devscene.ps1 -WaitSeconds 60`으로 런처 실행(정상 상태 300프레임 이상) → **`Bin/DevelopEngine/jg_log.txt`를 먼저 복사**(다음 실행이 덮어쓴다) → `bash Document/Memory/tools/memlog_stats.sh <사본>`(1-1 이후에는 `JG_MEMORY_TRACE` 빌드에서만 데이터가 나온다) → 종료 코드 0, `[error]`/`[critical]` 0, D3D12 디버그 레이어 메시지 0.
메모리 합격 기준: 풀 Allocate 수 == Deallocate 수, `Memory Chunk Shutdown` 2줄, 정상 상태 구간에서 모든 크기 클래스 live 델타 0, 어떤 클래스도 한도 근접 없음(Phase 2 이후에는 예상 밖 페이지 성장 없음), `Graphics PSO created` 시작 뒤 증가 없음.
기준선(2026-09-28 실행, `Document/Memory/2026-09-28_memory_baseline.txt`): 풀 Allocate 157,892 / 메인 청크 피크 2,296블록 / 32B 클래스 피크 123·종료 시 0 / 스레드별 청크 예약 100.69MB. 1-2 뒤와 Phase 2 뒤에 같은 파일에 v2, v3를 덧붙인다.
재현 훅(0-2): `JG_MEMTEST=exhaust32|oversize` 환경 변수. 실행 방법은 0-2 항목 참조. Phase 2-1의 인수 테스트이며 2-6 뒤 제거.

설계 규칙(이 목록 진행 중 지킬 것):
- **풀 `Allocate`/`Deallocate` 경로에서는 `JG_LOG`를 부르지 않는다.** `AddLog`가 `PString::Format`으로 문자열을 만들므로, 문자열이나 로깅 경로가 풀을 쓰게 되는 순간 무한 재귀다(현황분석 §5 E-6). 1-1에서 기존 호출을 스위치 뒤로 뺀다. 성장·실패 로그(2-1)는 실패 분기에서만 부르고, 그 안에서 풀을 다시 부르지 않는지 확인한다.
- **모든 블록은 청크 안에 있다.** 고갈은 페이지 성장으로 풀고 외부 블록을 만들지 않는다. 대형 블록 경로를 두더라도 풀 API 안에서 헤더 표식으로 처리하고 별도 카운터로 센다(2-1 결정).
- 엔진 컨테이너(`HList`/`HHashMap`/`HMap`/`HHashSet`)는 static 저장소에 두지 않는다(`Graphics_TODO.md` 2026-09-17 설계 규칙). 잔존 위반은 4-3에서 정리한다.
- 1-2가 끝나기 전까지 스마트 포인터 해제는 `= nullptr`이 아니라 `Reset()`으로 쓴다(`= nullptr`은 현재 no-op).
- 2-1이 끝나기 전까지 1MB를 넘을 수 있는 버퍼는 `std::vector`로 둔다(`Graphics_TODO.md` 5-23 우회 유지).
- 풀을 새로 쓰게 되는 항목(Phase 3, 5)은 그 객체가 메모리 시스템 해제(`Core/CoreSystem.cpp:104`)보다 먼저 반납되는지 종료 순서를 확인한다.

---

## Phase 0. 준비 · 기준선 — 완료 2026-09-28

- [x] **0-1. 기준선 기록** — 완료 2026-09-28. 증분 빌드 오류 0·재링크 없음(Bin이 HEAD `7972462`와 이미 일치, `Document/Memory/build_2026-09-28_phase0.log`). `capture_devscene.ps1 -WaitSeconds 60` → 종료 코드 0, 로그 315,973줄, `[error]`/`[critical]` 0, D3D12 디버그 메시지 0, `Graphics PSO created` 2회. `memlog_stats.sh` 결과 `Document/Memory/2026-09-28_memory_baseline.txt`: Allocate = Deallocate 157,892, `Dismatch` 55, 메인 청크 피크 2,296블록(16B 854 · 128B 616 · 32B 123), 종료 시 모든 클래스 live 0, 40,000줄 간격 추이 평탄. 예상 기준선(32B ~120, 피크 ~2,300)과 일치.
  현재 HEAD(`7972462`)로 검증 루프를 한 번 돌려 `memlog_stats.sh` 출력을 `Document/Memory/2026-09-28_memory_baseline.txt`로 저장한다. Phase 1 이후 비교 대상.
  완료 조건: 파일 존재, 위 기준선 수치와 같은 자릿수(32B live 120 안팎, 피크 2,300블록 안팎).

- [x] **0-2. 고갈·초과 재현 코드(임시)** — 완료 2026-09-28. `Source/Runtime/Devkit/DevScene.cpp`에 익명 네임스페이스 `runMemoryPoolReproIfRequested()`를 넣고 `JGDevScene::OnInitialize()` 끝에서 부른다. 환경 변수 `JG_MEMTEST`가 없으면 아무 일도 하지 않는다(기본 실행 영향 없음. 훅을 넣은 빌드로 20초 실행해 종료 코드 0·셧다운 2줄 확인). 두 경우 모두 재현됐고 스택은 `Document/Memory/2026-09-28_memtest_stacks.txt`.
  실행 방법: `JG_MEMTEST=exhaust32 Document/Memory/tools/crashwalk/crashwalk.exe C:\JG\JGEngine\Bin\DevelopEngine\JGLauncher.exe C:\JG\JGEngine\Bin\DevelopEngine 40`(Git Bash. 2차 예외에서 스택을 찍음) 또는 PowerShell `$env:JG_MEMTEST="oversize"; & capture_devscene.ps1 -WaitSeconds 30`(종료 코드만 확인). 실행 뒤 `jg_log.txt`를 복사해 끝부분을 본다.
  (a) `exhaust32`: 풀에서 32B 블록을 4,005개 받은 뒤 `nullptr`(그 시점 live 91). 풀은 로그 한 줄 없이 실패하고, 이어지는 `HList<uint8>(32)`가 `HAllocator<uint8>::construct`(`Allocator.h:43`)에서 주소 0에 써서 0xC0000005. 로그는 `[MemTest]` 경고 + 16B 디버그 프록시 할당 1줄로 끝나고 `Memory Chunk Shutdown` 없음.
  (b) `oversize`: `HList<uint8>(2MB)` → `HMemoryChunk::HasSpace`의 `MemoryQueues.at()`(`MemoryPool.cpp:75`, `Allocate` 86행, `HMemoryPool::Allocate` 183행, `HAllocator::allocate` `Allocator.h:27`)에서 `std::out_of_range`(0xE06D7363). 엔진 소스에 `catch`가 없어 `main`까지 전파 → terminate → 종료 코드 3. 로그는 (a)와 같은 형태로 끊김. 캡처 스크립트로 돌리면 abort 경로에서 메인 스레드가 멈춘 채 창이 30초 이상 남아 있다가 종료 코드 3.
  `Source/Runtime/Devkit/DevScene.cpp` 또는 DevConsole 명령으로 (a) 32B 블록 4,097개 이상 보유, (b) 2MB `HList<uint8>` 할당을 임의로 일으킬 수 있게 한다. 2-1의 완료 조건 확인용이며 **2-6 검증 뒤 제거**한다.
  완료 조건: 현재 소스에서 (a)는 로그 없이 크래시(널 포인터 쓰기), (b)는 종료 코드 3·로그 없음이 재현된다(= 고칠 대상 확인).

---

## Phase 1. 풀 무관 안전장치 (옛 1-3, 1-2, 1-5 일부 · 현황분석 A-2, A-3 · P0-3, P1-11, P1-16)

풀 구조를 건드리지 않는 작은 항목만 둔다. 1-1은 Phase 2에서 풀을 뜯는 동안 계측을 가리는 로그를 치우고 "풀 안 로그 금지" 규칙을 코드로 박기 위해 먼저 한다.

- [x] **1-1. 할당 로그를 컴파일 스위치 뒤로** — 완료 2026-09-28. `MemoryPool.cpp` 맨 위에 `#ifndef`로 가드한 `#define JG_MEMORY_TRACE 0`을 두고 `Allocate`/`Deallocate`의 로그를 `#if JG_MEMORY_TRACE`로 감쌌다(이 파일만 다시 컴파일하면 켜고 끌 수 있다). 청크 생성·종료 2줄과 `Dismatch` 경고는 유지. 포맷은 `%llu`, 주소는 `memlog_stats.sh`가 소문자 16진수를 읽으므로 MSVC `%p`(대문자·고정폭) 대신 `%llx`. `flush_on(warn)`(크래시 직전 Info 줄이 잘릴 수 있어 원인 추적 시 trace로 되돌리라는 주석). `Allocate` 위에 재귀 금지 주석, `memlog_stats.sh` 머리말에 트레이스 빌드 안내. 결과: 기본 빌드 60초 실행 로그 **211줄·18,680 bytes**(이전 315,973줄·약 25 MB), `[Memory]` 59줄(청크 2+2, Dismatch 55), 할당/해제 줄 0. 트레이스 빌드로 `memlog_stats.sh`를 다시 돌려 기준선과 피크가 같음을 확인(1-4).
  `Source/Runtime/Core/Memory/MemoryPool.cpp:43, 60, 100, 115, 210`, `Core/Misc/Log.cpp:15-16`
  `[Memory] Allocated/Deallocated` 로그를 `#if JG_MEMORY_TRACE`로 감싸고 기본 꺼짐. 청크 생성/종료 2줄과 `Dismatch` 경고는 남긴다. `flush_on(trace)`(16행)를 `warn`으로. 포맷 `%u`(uint64)·`%x`(포인터)를 `%llu`/`%p`로(지금은 32비트로 절단돼 찍힌다).
  `Allocate`/`Deallocate` 본문 위에 "이 경로에서 JG_LOG 금지(재귀)" 주석을 남기고, `Document/Memory/tools/memlog_stats.sh` 머리말에 "`JG_MEMORY_TRACE` 빌드에서만 데이터가 나온다"를 적는다.
  완료 조건: 기본 빌드에서 정상 상태 프레임당 `[Memory]` 줄 0, 로그 파일이 실행 1분에 1MB 미만. 스위치 켠 빌드에서 `memlog_stats.sh`가 0-1과 같은 결과를 낸다.

- [x] **1-2. `PSharedPtr`/`PWeakPtr` 대입 의미론** — 완료 2026-09-28. 두 클래스에 `assign()` 헬퍼(새 참조를 먼저 잡고 옛 참조를 놓는다, 같은 객체면 변화 없음)를 두고 copy/set이 이를 쓴다. move는 자기 이동 가드 + 같은 객체면 rhs가 들던 참조 하나를 대신 반납. `operator=(nullptr_t)` 추가. 만료된 약참조도 표준처럼 그대로 복사(이전에는 `IsValid` 검사에 걸려 대입이 무시됐다). 검증: `Document/Memory/tools/sharedptr_semantics_test.cpp`(Memory.h 코드를 그대로 옮긴 재현 + 같은 시나리오를 `std::shared_ptr`/`weak_ptr`로 돌려 비교) **15케이스 ALL OK**. 런처 60초·20초 실행 종료 코드 0, D3D12 디버그 0, 모듈 종료 순서 로그 이전과 동일. 피크 사용량은 v1과 같다(`= nullptr` 사이트가 대부분 종료 경로여서 정상 상태 수치에는 영향 없음).
  `Source/Runtime/Core/Memory/Memory.h:241-275`(copy/move), `PWeakPtr`의 set/copy/move
  빈 rhs 대입·빈 rhs 이동 대입은 `Reset()`과 같아야 하고(지금은 early return으로 no-op), `operator=(nullptr_t)`를 추가하고, 자기 대입은 유지(지금은 `Reset()`이 먼저 돌아 참조 −1 후 널), 이동 후 rhs는 비어야 한다. `PWeakPtr`도 같은 검사.
  `= nullptr` 대입 28곳(변수 21종, 예 `Source/Runtime/Graphics/JGGraphics.cpp:58`)이 이제 실제로 해제되므로, 해제 시점이 앞당겨져 문제가 되는 곳이 없는지 본다(특히 `ShutdownModule`의 `_graphicsAPI`: 파괴가 `DisconnectModule`의 Flush로 앞당겨진다. 2026-09-17 null 가드가 있으므로 안전해야 함).
  완료 조건: `Memory.h`의 copy/move를 옮긴 재현 코드(현황분석 §6-1의 5케이스: `p=nullptr`, `p=empty`, `p=p`, `p=move(empty)`, `p=q`)가 표준 `shared_ptr`과 같은 결과를 낸다. 런처 회귀 없음.

- [x] **1-3. 타입 검사 `static_assert`** — 완료 2026-09-28. `static_assert(!is_class<T> || is_base_of<IMemoryObject, T>)`로 교체. 전체 빌드에서 발동한 곳 없음(= 기존 `Allocate<T>` 사용 약 50곳이 모두 적법). `enable_if<...>::type = 0` 12곳을 `enable_if_t<..., int32> = 0`으로.
  `Source/Runtime/Core/Memory/Memory.h:565-570`
  `Allocate<T>`의 "IMemoryObject 파생" 검사를 런타임 `JG_ASSERT`에서 `static_assert`로(`Wrap`은 이미 그렇다. 릴리스 구성에서 UB 방지). `PSharedPtr` 변환 생성자의 `std::enable_if<...>::type`에 `typename`(MSVC 전용 표기 제거). 옛 1-5의 iterator 락 문제는 2-3에서 그 함수를 다시 쓰므로 거기서 처리한다.
  완료 조건: 전체 빌드 오류 0, 런처 회귀 없음.

- [x] **1-4. Phase 1 검증** — 완료 2026-09-28. 전체 빌드 오류 0, 경고 기준선 유지(C4244 4·LNK4098 1, `Document/Memory/build_2026-09-28_phase1.log`). 기본 빌드 60초: 로그 211줄, `[error]`/`[critical]` 0, D3D12 0, `Graphics PSO created` 2, ExitCode 0. 트레이스 빌드 60초: allocs = deallocs 121,108, 메인 청크 피크 2,296(16B 854 · 32B 123 · 64B 157 · 128B 616 · 256B 493) — **v1과 동일**, Dismatch 55, 종료 시 live 0, ExitCode 0 → 기준선 파일에 **v2** 추가. 트레이스 0 복귀 빌드 뒤 20초 실행 ExitCode 0. 총 할당 수(v1 157,892 → v2 121,108)는 60초 동안 돈 프레임 수 차이이고 프레임당 구성과 피크는 같다.
  공통 검증 루프(`JG_MEMORY_TRACE` 켠 빌드로 `memlog_stats.sh` 1회) + 1-2 재현 코드 5케이스 통과 + 기본 빌드 로그 줄 수 확인. 결과를 기준선 파일에 **v2**로 덧붙인다(1-2로 해제가 앞당겨져 32B live·피크가 줄 수 있음. 늘면 원인 확인).

- [ ] **1-5. 커밋** — "메모리 할당 로그 스위치, 스마트 포인터 대입 수정, Allocate static_assert" (사용자가 직접 커밋). 포함: `Core/Memory/MemoryPool.cpp`, `Core/Memory/Memory.h`, `Core/Misc/Log.cpp`, `Document/Memory/tools/memlog_stats.sh`, `Document/Memory/tools/sharedptr_semantics_test.cpp`(신규), 기준선 파일(v2 추가), `build_2026-09-28_phase1.log`. Phase 0 산출물(`Devkit/DevScene.cpp` 임시 재현 코드, `2026-09-28_memtest_stacks.txt`, `build_2026-09-28_phase0.log`, `Memory_TODO.md`, `tools/crashwalk/crashwalk.exe`)이 아직 미커밋이면 함께 넣거나 앞 커밋으로 분리한다.

---

## Phase 2. 풀 재설계 (옛 Phase 4 + 옛 1-1 + 옛 1-4 · 현황분석 A-1, A-4, C · P0-1, P0-5, P0-6, P1-9, P1-10, P1-12, P1-14)

상세 설계는 **`Document/Memory_풀재설계_설계_2026-09-28.md`**(자료구조·할당/해제 경로·스레드 모델·통계·검증·결정 4건). 아래 항목의 "결정 필요"는 그 문서 §6과 같다.

- [x] **2-1. 청크를 페이지 목록으로: 성장형 크기 클래스와 실패 처리** — 완료 2026-09-28. `MemoryPool.h/.cpp` 전면 교체(설계 문서 §3, 구현 차이 §3-15): 청크 = 클래스 18개(8B~1MB)의 페이지 목록(64KB `VirtualAlloc`, 32KB 이상은 블록 하나짜리 `malloc` 페이지, lazy). 클래스가 비면 페이지 추가(Info 한 줄, 청크 커밋 16·32·64·128MB 문턱 Warning). 1MB 초과·정렬 16 초과는 대형 경로(요청 크기 그대로 + 캐시: 용량×4 ≤ 요청×5, 예산 64MB, 유휴 10초). 빈 페이지는 같은 클래스가 재사용(LIFO)하고 10초 유휴면 `Tick`(`GMemoryGlobalSystem::Update`)이 회수. 남은 실패는 시스템 힙 실패 하나로 Critical 로그 뒤 `abort()`. 상주 메모리 private 428.5 → 230.2 MB(WS 335.6 → 137.6 MB). `Graphics_TODO.md` 5-23·5-27 종결. 원래 계획: `Source/Runtime/Core/Memory/MemoryPool.h/.cpp`(`HMemoryChunk::Initialize` 7-44행, `Allocate` 78-100행, `MakeMemoryChunk` 228-251행), `Core/Memory/Memory.h:565-575`, `Core/Memory/Allocator.h:18-32` — `결정 필요`
  `HMemoryChunk`가 클래스마다 페이지 목록을 갖게 한다. 페이지는 같은 헤더 형식(`HMemoryHeader`, `OwnerChunkID` 동일)의 블록 N개 묶음이고, 큐가 비면 페이지를 하나 더 만들어 붙인다(**외부 블록을 만들지 않는다**). 페이지 목록은 `std::vector`(풀 자기 자신을 쓸 수 없음). `Shutdown`은 페이지 전부 해제.
  초기 예약은 기준선(메인 피크 2,296블록·303KB, 워커 5.5MB)에 맞춰 스레드당 수 MB로 낮춰 100.69MB 상주를 없앤다.
  성장이 일어나면 Warning 로그 1줄(클래스, 스레드, 페이지 수. 실패 분기이므로 재귀 규칙과 충돌 없음을 확인). `JG_ASSERT("no memory space left.")`(88행)와 `JG_ASSERT(true)`(109행)는 조건식으로. `HasSpace`의 `MemoryQueues.at()`(75행)은 클래스 조회로 바꿔 예외를 없앤다. `GMemoryGlobalSystem::Allocate`의 `new(MemPtr)`(575행)와 `HAllocator::allocate`(18행)에는 `JG_CHECK(ptr != nullptr)`.
  결정 1 — 페이지 크기: 권장 64KB(작은 클래스는 블록 수백 개, 큰 클래스는 최소 4블록). 대안: 클래스별 고정 블록 수.
  결정 2 — **확정 2026-09-28**: 상한 1MB. 초과 요청은 청크가 소유하는 블록 하나짜리 페이지로 요청 크기 그대로 잡고(헤더 32B: `Raw`·`Capacity`·표식 `MemorySize = Large`), 놓으면 풀 전체 대형 캐시에 넣어 재사용한다(요청 ≤ 용량 ≤ 요청 × 1.25, 총량 64MB, 10초 유휴 해제, 프레임 틱은 `GMemoryGlobalSystem::Update`). 상세·시나리오는 설계 문서 §3-9. 캐시 파라미터 세 값(1.25배·64MB·10초)과 페이지 64KB·성장 로그 수준·디버그 채움/격리·회수 방식(메인 Tick + 청크별 뮤텍스)은 **2026-09-28 확정**(설계 문서 §6).
  결정 3 — **확정 2026-09-28**: 빈 페이지는 개수 제한 없이 같은 클래스가 재사용하고 10초 유휴면 프레임 틱이 회수한다(대형 캐시와 같은 규칙. "보관 1개" 규칙 폐기). Empty·All 페이지 목록만 청크별 뮤텍스, 블록 경로는 무락. 설계 문서 §3-8b.
  완료 조건: 0-2 (a)가 성장 Info 로그 뒤 계속 실행(종료 코드 0), (b)가 결정 2대로 동작. 시작 직후 상주 메모리(작업 관리자 Private Bytes)가 기준선보다 150MB 이상 감소. `Graphics_TODO.md` 5-23 종결 표시((i)면 `std::vector` 우회는 2-4 뒤 필요 시 되돌림).

- [x] **2-2. 클래스별 카운터와 통계 도구** — 완료 2026-09-28. `HSizeClass`의 atomic 카운터(live·peak·total·pages·empty·growths·releases·alloc), 청크의 원격 반납·대형 블록 카운터, 풀의 대형 캐시 카운터를 `GetStatInfo`가 relaxed 로 읽기만 한다(락·청크 워크 없음). 위젯(`MemoryStatistics.cpp`): 풀 요약 2줄 + 스레드별 막대(청크마다 자기 값. 옛 `[0]` 버그 수정) + 청크별 클래스 표(live/total·페이지(빈)·성장·회수·사용률·피크. 성장 있으면 노랑, 사용률 90% 이상 빨강). 스크린샷 `Document/Memory/2026-09-28_phase2_memwidget_capture.png`(exhaust32 중 32B 행 성장 6·회수 6). 부수 수정: `HGUI::PlotBarGroups`가 그룹 1개일 때 ImPlot 틱 라벨을 넘겨 읽던 크래시(`GUI.cpp`). 원래 계획: `Core/Memory/MemoryPool.h/.cpp`(`GetStatInfo` 118-150행), `Source/Editor/DevStatistics/MemoryStatistics.cpp:59-61`
  새 청크가 처음부터 클래스별 live/피크/페이지 수(=한도)/성장 횟수, 대형 블록 수·바이트를 갖고 `Allocate`/`Deallocate`에서 O(1)로 증감한다. `GetStatInfo`의 청크 전체 워크(30,768블록)를 카운터 기반으로 바꾼다.
  위젯은 루프 안 `MemoryChunkStateInfos[0]`을 `ChunkStatInfo`로(모든 그룹이 메인 스레드 값을 보이는 버그), 스레드별 막대 아래에 클래스별 사용률(live/한도)·피크·성장 횟수 표, 90% 이상 강조.
  완료 조건: 위젯을 열어 둔 채 0-2 (a)를 돌리면 32B 행의 성장 횟수가 오르는 것이 보이고, 프레임 시간이 위젯 개폐에 따라 달라지지 않는다.

- [x] **2-3. 스레드 간 해제 경합 제거와 iterator 락** — 완료 2026-09-28. TLS 청크 캐시(모듈 사본은 스레드 id 비교로 1회 보정), 청크 테이블 인덱스 = 헤더 `OwnerChunkID`, 키는 `std::thread::id`(해시 충돌 없음), 타 스레드 반납은 lock-free MPSC 스택에 push 하고 소유 스레드가 다음 할당 때 pop-all(대기 256 이상이면 즉시). `Dismatch` 경고는 없어지고 `RemoteFreeTotal` 카운터로. 스트레스(별도 스레드 ↔ 메인 10라운드 × 10,000블록 × 양방향, 8B~1MB+1 혼합, 바이트 패턴 검증) 손상 0·pending 0·live 복귀, 1.4초. 원래 계획: `Source/Runtime/Core/Memory/MemoryPool.cpp:166-211`
  소유 스레드의 `Allocate`가 청크 큐를 잠금 없이 만지고, 타 스레드 `Deallocate`의 배타 락은 상대가 없어 보호가 안 된다(`Dismatch` 55건/실행). 타 스레드 해제는 소유 청크의 반납 큐(뮤텍스 또는 lock-free 스택)에 넣고 소유 스레드의 다음 `Allocate`가 회수한다. `HMemoryPool::Allocate`가 `shared_lock`을 풀고 쓰는 iterator(옛 P0-6)는 락 안에서 쓴다. `std::hash<std::thread::id>` 충돌 시 청크가 섞이므로 충돌 감지(스케줄러의 시작 시 검사를 풀 쪽으로) 또는 스레드 로컬 인덱스.
  완료 조건: 임시 스트레스 테스트(로드 스레드에서 `HList` 10만 개 할당 → 메인에서 해제, 반대 방향도)를 디버그 빌드에서 10회 돌려 손상·assert 없음. `Dismatch` 경고는 정보 로그로 낮추거나 제거.

- [x] **2-4. `HAllocator` 완성** — 완료 2026-09-28. `operator==`/`!=`, `is_always_equal`, `allocate`가 `alignof(T)`를 풀에 전달(16 초과는 대형 정렬 경로), `JG_CHECK(Result != nullptr)`, 메모리 시스템 종료 뒤 `deallocate`는 무시 + 디버그 1회 `OutputDebugString`. `GMemoryGlobalSystem::Allocate<T>`도 `alignof(T)` + `JG_CHECK`. 검증: `alignas(32)` `HList` 1,000개 모두 32의 배수, `HList::swap`·이동 대입 컴파일·동작. `ResourceStagingManager::RecordUploads`의 "복사 후 clear" 우회를 `swap`으로 되돌림. 원래 계획: `Source/Runtime/Core/Memory/Allocator.h`
  `operator==`/`operator!=`(항상 같음)와 `is_always_equal = true_type`(`HList` swap·이동 대입 C2678 해소). `alignof(T)`가 16을 넘으면 정렬 인자를 풀에 전달(현재 정렬은 개수 표가 우연히 맞아 16까지만 보장. 페이지 시작과 블록 stride를 정렬에 맞춘다). 메모리 시스템 해제 뒤 `deallocate`는 지금처럼 무시하되 디버그에서 1회 경고. 이후 `Graphics`·`StagingManager`의 "복사 후 clear" 우회를 swap으로 되돌린다.
  완료 조건: `HList<T>::swap`과 이동 대입이 컴파일된다. `alignas(32)` 타입을 담은 `HList` 임시 테스트에서 주소가 32의 배수.

- [x] ~~**2-5. 값 타입에서 `IMemoryObject` 분리 검토**~~ — **하지 않음 (사용자 결정 2026-09-29).** 실익이 작다: 값 하나당 vptr 8B(메인 청크 풀 할당 336KB 규모라 수십 KB 이하), 실제 오용인 `JGType` GC 할당 4곳도 무해해 그대로 둔다. 4-1·7-1의 전제도 아니다. 검토 기록(측정 크기·방안 비교): `Document/Memory_2-5_값타입분리_검토_2026-09-29.md`. 아래는 원래 항목 설명.
  `PString`·`PName`·`JGType`·`PSharedPtr`·`PWeakPtr`이 `IMemoryObject`를 상속해 vptr 8B를 지니고(`PSharedPtr` 32B), `Allocate<>` 대상 표식과 값 타입이 섞여 있다. 권장안: GC 대상 표식을 별도 태그 타입(`IGCObject`)으로 분리하고 값 타입은 상속을 끊는다(`Allocate<PString>` 같은 사용이 있으면 `static_assert`로 드러남). 대안: 현행 유지(문서화만). 4-1(제어 블록)이 이 결정에 영향을 받으므로 여기서 정한다.
  완료 조건(적용 시): `sizeof(PSharedPtr<T>) == 24`, 전체 빌드 오류 0, 리플렉션·JSON 직렬화 회귀 없음(`Allocate(const T&)`로 `JGType`을 만드는 `ObjectGlobals.cpp:42, 58`은 유지 여부 확인).

- [x] **2-6. Phase 2 검증** — 완료 2026-09-28. (1) 전체 빌드 오류 0, 경고 LNK4098 1(`build_2026-09-28_phase2*.log`). (2) 임시 테스트 6종(`JG_MEMTEST=exhaust32|oversize|align|stress|largecache|emptypages`, crashwalk 아래 실행) **37/37 PASS**, 크래시 0, 종료 코드 0 — `Document/Memory/2026-09-28_memtest_phase2_results.txt`. (3) 트레이스 빌드 60초 리플레이 → 기준선 **v3**(Allocate = Deallocate 354,081, Dismatch 0, 클래스 live 평탄, 종료 0). (4) 상주 메모리 private 428.5 → **230.2 MB**, WS 335.6 → 137.6 MB(`tools/measure_resident.ps1.txt`). (5) 위젯 스크린샷. (6) 종료 로그 `Memory Chunk Shutdown` 청크마다 1줄(pages·committed·live·remote pending), 대형 잔존 0, D3D12 오류 0. (7) 임시 코드 제거(DevScene.cpp 검증 블록·호출, DevStatistics.cpp 위젯 자동 열기). **새 풀이 드러낸 기존 버그 2건 수정**: `PGraphicsPipelineState::BindShader`의 non-const 키 `HPair` 순회(HList 임시 복사 → `_desc.VS/PS` dangling. 설계 문서 §8)와 `HGUI::PlotBarGroups` 그룹 1개 크래시. 원래 계획: 공통 검증 루프 + 0-2 (a)(b) 재현이 2-1의 정해진 동작을 함 + 상주 메모리 비교 + 2-3 스트레스 테스트 + 2-4 정렬 테스트 + 위젯 클래스별 표 확인. 결과를 기준선 파일에 **v3**로 덧붙인다. 확인 뒤 **0-2·2-3 임시 코드 제거**(`git diff`로 확인. DevScene.cpp의 `runMemoryPoolReproIfRequested`와 호출 1줄).

- [ ] **2-7. 커밋** — "메모리 풀 재설계: 페이지 성장형 클래스, 대형 블록 캐시, 클래스별 카운터, 스레드 간 해제 스택, HAllocator 완성" (사용자가 직접 커밋. 1-5 와 함께 해도 된다). 포함: `Core/Memory/MemoryPool.h/.cpp`·`Allocator.h`·`Memory.h/.cpp`, `Editor/DevStatistics/MemoryStatistics.cpp`, `Graphics/DirectX12/Classes/PipelineState.cpp`(BindShader 키 const)·`ResourceStagingManager.cpp`(swap), `GUI/GUI.cpp`(PlotBarGroups), `Devkit/DevScene.cpp`(임시 코드 제거 뒤 HEAD 와 같음), `Document/Memory/tools/`(memlog_stats.sh 패턴, crashwalk.cpp/.exe 행 덤프, measure_resident.ps1.txt), 문서(`Memory_TODO.md`, 설계 문서, 인수인계, `Graphics_TODO.md` 5-23·5-27), 기준선 v3·테스트 결과·스크린샷·빌드 로그.

---

## Phase 3. 풀 밖 할당 흡수 1차 (옛 Phase 2 · 현황분석 E-1~E-3 · 선행 조건 없음, 풀이 성장할 수 있게 된 뒤 진행)

**건너뜀 (사용자 결정 2026-09-29). 아래 3-1~3-5는 하지 않는다.** 세 항목 모두 지금 고칠 문제가 없다. 할당 횟수가 적고(모듈 시작 때 8번, 고유 문자열당 24B 한 번, 32B 넘는 델리게이트 바인딩만), 목적인 "통계에 보이게"도 풀 커밋 약 4MB 대 프로세스 Private 230MB라 거의 달라지지 않는다(나머지는 D3D12 드라이버·ImGui·rapidjson·문자열 본문 등 이 Phase 대상이 아님). 3-1은 적힌 대로 하면 위험하다(아래 주의).

- [ ] **3-1. `HDelegate` 힙 할당을 풀로** — `Source/Runtime/Core/Misc/Delegate.h:66, 81`
  `HDelegates::SetAllocationCallbacks`(81행) 후크가 있는데 호출자가 0이고 기본값이 malloc/free 람다(66행)다. `GCoreSystem::Create`에서 메모리 시스템 등록 직후(`Core/CoreSystem.cpp:36` 다음) 풀 `Allocate`/`Deallocate`로 연결한다. Free 콜백은 `GMemoryGlobalSystem::IsValid()`가 false면 그냥 반환(종료 후 static 델리게이트 대비, `HAllocator::deallocate`와 같은 규칙).
  인라인 버퍼 32B를 넘는 바인딩만 힙을 타므로 크기는 작다. 풀 `Allocate` 안에서 델리게이트를 만드는 경로가 없는지 확인(재귀 규칙).
  **주의(2026-09-29 확인): 적힌 대로 하면 힙이 깨진다.** `HDelegatesInteral::Alloc`/`Free`는 헤더의 네임스페이스 `static` 변수(`Delegate.h:66-67`, 내부 연결)라 번역 단위·DLL마다 사본이 따로 있고, `SetAllocationCallbacks`(81행)는 부른 파일의 사본만 바꾼다. 한 곳에서만 풀로 바꾸면 풀에서 할당한 블록을 다른 파일이 `free`하거나 그 반대가 된다. 지금은 모든 사본이 malloc/free라 안전하다. 하려면 콜백이 모든 번역 단위·DLL에서 같은 값을 보도록 구조부터 바꿔야 한다.
  완료 조건: `JG_MEMORY_TRACE` 빌드에서 32B 초과 델리게이트 바인딩이 풀 할당으로 보이고, 정상 종료 시 Allocate == Deallocate 유지.

- [ ] **3-2. 모듈 인터페이스 객체를 풀로** — `Source/Runtime/Core/Misc/Module.h:11`(`JG_MODULE_IMPL`), `Core/Misc/Module.cpp:137, 217, 264`
  `HPlatform::Allocate`/`Deallocate` 자체는 `DevelopUnit.h:12`와 JGHeaderTool(`Programs/JGHeaderTool/Class/HeaderTool.cpp` 20여 곳)도 쓰므로 그대로 두고, 모듈 매크로와 `Module.cpp`의 해제 3곳만 풀 경로로 바꾼다. 모듈 객체는 `IMemoryObject`가 아니므로 `GMemoryGlobalSystem::Allocate<T>`(GC 추적)가 아니라 `HMemoryPool` 직접 할당 + placement new/소멸자 호출로 한다(DLL 안에서 생성, Core에서 파괴되지만 풀은 `GCoreSystem` 인스턴스 하나를 공유하므로 경계 문제 없음).
  완료 조건: 모듈 8개(`Graphics`, `GUI`, `Asset`, `Devkit`, `DevConsole`, `DevStatistics`, `JGDev_Graphics`, `JGEditor`) 연결·해제 정상, 종료 코드 0. 해제가 `GModuleGlobalSystem::Destroy`(메모리 시스템 해제보다 앞) 안에서 끝나는지 로그로 확인.

- [ ] **3-3. `GStringTable` 참조 카운트 제어 블록을 풀로** — `Source/Runtime/Core/String/StringTable.cpp:47`
  `std::make_shared<HAtomicInt32>()`를 `std::allocate_shared<HAtomicInt32>(HAllocator<HAtomicInt32>())`로. `PName::_weakRefCount`의 제어 블록이 같은 할당이므로 이 한 줄이 현황분석 §2-1 목록의 "PName 제어 블록"과 "GStringTable RefCount" 둘을 함께 옮긴다(32B 클래스 하나). 7-1에서 `PName`을 ID만 들도록 바꾸면 이 항목은 자연히 사라진다.
  주의: 약참조가 살아 있는 동안 제어 블록이 유지되므로, 메모리 시스템 해제 뒤까지 살아남는 `PName`(static 저장소의 PName, forceFlush 이후 파괴되는 객체의 멤버)이 있으면 죽은 풀에 반납한다. `grep -rn "static.*PName" Source/Runtime Source/Editor`로 후보를 확인하고, 있으면 `PName`이 아닌 `uint64` ID나 리터럴로 바꾼다.
  완료 조건: 정상 종료 시 Allocate == Deallocate 유지, 종료 코드 0, `PName` 관련 `Dismatch`(또는 2-3 이후의 반납 큐 통계) 증가 없음(문자열 등록은 에셋 로드 스레드에서도 일어난다).

- [ ] **3-4. Phase 3 검증** — 공통 검증 루프. 추가로 32B·16B 클래스 live가 v3 기준선 대비 얼마나 늘었는지, 페이지 성장이 일어났는지 기록.

- [ ] **3-5. 커밋** — "델리게이트·모듈 인터페이스·문자열 참조 카운트를 메모리 풀로" (사용자가 직접 커밋). 포함: Core/Misc/Delegate.h, Core/CoreSystem.cpp, Core/Misc/Module.h/.cpp, Core/String/StringTable.cpp.

---

## Phase 4. GC · 스마트 포인터 구조 (옛 Phase 3 · 현황분석 B · P0-4, P0-7, P1-8)

**검토 2026-09-29 (`결정 필요`): 4-1을 "약참조 카운터 수명"으로 줄여서 하고, 나머지(4-1의 맵·뮤텍스 제거, 4-2, 4-3, 4-4)는 지금 문제가 없어 하지 않는 것을 권장.**
- 지금 있는 문제(코드 확인): GC가 객체를 파괴하면서 참조 카운터를 같이 해제한다(`Memory.cpp:105` `AllocatedMemoryBlocks.erase`, WeakCount 무시). 만료된 `PWeakPtr`는 그 주소를 계속 써서 `IsValid()`·소멸자가 해제 메모리를 읽고(`Memory.h:491`, 소멸자 386행), `Reset()`·재대입·이동은 해제 메모리에 쓴다(468, 514, 557행). 확인된 실행 경로는 에셋 로드마다 한 번: 로드가 끝나면 `_loadingAssets.erase`가 `HTaskHandle`의 `PWeakPtr<PTask>`를 소멸시키는데, 그 전에 GC(`GMemoryGlobalSystem::Update`가 모듈 갱신보다 먼저 돈다)가 PTask를 파괴해 카운터가 이미 없다. 크래시로 관측된 적은 없다(디버그 힙이 해제 메모리를 0xDD로 채워 음수로 읽힘). `Pin()`은 주소로 객체를 다시 찾아서(481행) Phase 2 풀의 즉시 주소 재사용과 겹치면 다른 객체를 잡을 수 있다. 종료 강제 파괴 때 남은 스마트 포인터가 해제된 카운터에 쓰는 문제(2026-09-17 기록)도 같은 원인이다.
- 축소판 진행: ① 재현(임시 코드로 GC가 카운터를 해제하는 대신 표식값으로 채워 두고, 약참조가 그 값을 읽으면 경고) → ② 수정: 카운터를 제어 블록 하나로 묶고 객체가 죽어도 약참조가 남아 있으면 유지(std::shared_ptr 방식, 약참조 수 + 살아 있는 동안 1), `Pin()`·`Wrap()`은 "참조 수가 0보다 크면 1 올리기"(CAS)로, `Wrap`이 약참조 카운터도 채우게, 강제 파괴 중에는 제어 블록 해제를 끝까지 미룸 → ③ 검증 후 임시 코드 제거. 범위 `Core/Memory/Memory.h/.cpp`, 100줄 안팎.
- 하지 않는 것: 4-1의 맵·전역 뮤텍스·객체당 할당 4회 정리(측정된 비용 없음), 4-2(매 프레임 전체 순회는 측정 안 했고 문제 보고 없음, 파괴 순서는 지금 종료 코드 0, D3D12 오류 0), 4-3(누수 리포트는 관측용, static 맵 4개는 폴백이 돌 때만 생기는데 지금은 안 돈다), 4-4(`HTaskHandle`의 유일한 보유자 `GAssetDatabase`는 메서드를 부르지 않는다).

- [ ] **4-1. 제어 블록 통합** — `Source/Runtime/Core/Memory/Memory.h`(`GMemoryGlobalSystem::Allocate` 565행, `Wrap` 610행, `HMemoryBlock`), `Core/Memory/Memory.cpp:92-107`, `Core/Memory/MemoryPool.h`(`HMemoryHeader`, 2-1에서 바뀐 레이아웃 기준) — `결정 필요`
  RefCount/WeakCount/상태 플래그를 객체와 같은 블록에 둔다. 권장안: 헤더를 늘리지 않고 GC 객체에 한해 객체 앞에 고정 크기 제어 블록을 두고 `Allocate<T>`가 `sizeof(T) + 제어블록`을 요청한다(컨테이너 블록은 그대로). 대안: 헤더 확장(모든 블록이 비용을 냄). (2-5 값 타입 분리는 하지 않기로 함, 2026-09-29. 이 항목과 무관)
  `AllocatedMemoryBlocks`(unordered_map)·`AllocatedMemoryBlockQueue`·`unique_ptr<HAtomicInt32>` 2개를 없앤다. `Wrap(ptr)`은 포인터 연산 한 번(O(1), 무락)으로 제어 블록을 찾는다. `IMemoryObject` 뿌리 하나·오프셋 0 규칙은 그대로 전제.
  객체 파괴(Ref 0 → `Destruction()`·소멸자)와 메모리 반납(Weak 0)을 분리한다. 그래야 `PWeakPtr::IsValid()`가 해제된 카운터를 읽지 않는다(P0-4, `Graphics_TODO.md` 5-18의 근본 원인).
  완료 조건: 디버그 빌드에서 `PWeakPtr::IsValid()`/`Pin()`이 해제 메모리를 읽는 경로가 없음(파괴 시 제어 블록 오염값 채우기 테스트). 객체당 힙 할당이 4회 → 1회. `Wrap` 호출에 전역 뮤텍스 없음.

- [ ] **4-2. 파괴 정책: 순회 제거, 순서, GPU 타이밍** — `Source/Runtime/Core/Memory/Memory.cpp`(`garbageCollection`, `garbageCollectionInternal`), `Core/CoreSystem.cpp:35-41`(시스템 순서) — `결정 필요`
  참조가 0이 되는 순간(`subRefCount`) 지연 파괴 목록에 push하고, GC는 그 목록만 처리한다(살아 있는 객체 전부를 매 프레임 두 번 큐로 돌리는 현재 방식 제거). 파괴 순서는 push 역순(LIFO)을 기본으로 해서 부모(API·큐·할당자)가 자식(텍스처·버퍼)보다 늦게 죽게 한다(2026-09-17 크래시의 구조적 원인, 기존 P3).
  GPU 리소스: 지금은 GC가 `GCoreSystem::Update` 2번째(펜스 대기 전)에 돌아 프레임 N에 놓은 리소스가 N+1 시작에 풀린다. 권장안: 파괴 항목에 제출 펜스 값을 태깅하고 `PCommandQueue::IsFenceComplete`가 참일 때만 실행. 대안: 고정 N(=2)프레임 지연. 결정 후 `HJGGraphicsModule`이 GC에 펜스 조회를 제공하는 방식(Core가 Graphics를 모르므로 콜백/델리게이트)을 정한다.
  완료 조건: 종료 로그에서 `PDirectX12API`가 텍스처·버퍼보다 늦게 파괴됨. 프레임당 GC 비용이 살아 있는 객체 수와 무관(임시로 GC 객체 5,000개를 만들어도 프레임 시간 변화 없음). 리소스를 매 프레임 만들고 버리는 임시 테스트에서 D3D12 디버그 레이어 메시지 0.

- [ ] **4-3. 종료 규약과 static 잔존 정리** — `Source/Runtime/Core/CoreSystem.cpp:81-122`, `Core/Memory/Memory.cpp:12-17`(`forceFlush`), `Source/Runtime/Graphics/DirectX12/DirectX12API.cpp:429-449`
  `forceFlush` 시점에 살아 있는 객체를 타입·주소·참조 수와 함께 로그로 열거한다(누수 리포트. 정상이면 `PThread` 28개·`PTimer`·`PSequentialIDGenerator`·`PWindowsJWindow`만 남아야 함). GC 객체 소멸자가 이미 해제된 시스템(`GStringTable`, `GObjectGlobalSystem`)을 `GetInstance()`로 부르면 널 역참조인데, `GGlobalSystemInstance::GetInstance()`에 `JG_CHECK(instance != nullptr)`을 넣어 무음 UB를 assert로 바꾼다.
  `HDirectXAPI` 폴백의 함수 지역 static `HHashMap` 4개(431, 437, 443, 449행)는 설계 규칙 위반(`Graphics_TODO.md` 5-17). `std::unordered_map`으로 바꾸거나 의도적 leak 힙 객체로 바꾼다.
  완료 조건: 종료 시 누수 리포트가 예상 목록만 출력, 종료 코드 0. `grep -rn "static H\(List\|HashMap\|Map\|HashSet\)<" Source/Runtime Source/Editor`가 함수 지역 static을 0건 반환(멤버 static 접근자 선언은 제외). `Graphics_TODO.md` 5-17 종결 표시.

- [ ] **4-4. `HTaskHandle` 완료 감지 복구** — `Source/Runtime/Core/Thread/Task.h`(`HTaskHandle`), `Source/Runtime/Asset/AssetDatabase.h:26`, `AssetDatabase.cpp`
  4-1로 약참조가 카운터를 안전하게 읽게 되면 `HTaskHandle`이 `PTask`의 원자 변수 raw 포인터를 들 필요가 없다. 완료 상태를 제어 블록 또는 핸들 공유 소유(`PSharedPtr<PTask>` 또는 별도 공유 플래그)로 옮기고, `GAssetDatabase`의 자체 플래그 우회(4-6에서 추가)를 제거한다.
  완료 조건: 에셋 비동기 로드가 `HTaskHandle::IsCompelete()`로 감지되고 DevScene 메시 로드 회귀 없음. `Graphics_TODO.md` 5-18 종결 표시.

- [ ] **4-5. Phase 4 검증** — 공통 검증 루프 + 4-1 해제 메모리 접근 테스트 + 4-2 파괴 순서·GC 비용·매 프레임 리소스 생성/파괴 임시 테스트 + 4-3 누수 리포트 확인. 임시 테스트 코드는 `git diff`로 제거 확인.

- [ ] **4-6. 커밋** — "GC 제어 블록 통합, 지연 파괴 목록(LIFO·펜스), 종료 누수 리포트, static 컨테이너 잔존 정리, HTaskHandle 복구" (사용자가 직접 커밋). Core/Memory, Core/CoreSystem, Core/Thread/Task, Graphics/DirectX12API.cpp, Asset/AssetDatabase.

---

## Phase 5. 풀 밖 할당 흡수 2차 (현황분석 E-4, E-5 · 선행 Phase 2)

- [ ] **5-1. ImGui / ImPlot 할당자 연결** — `Source/Runtime/GUI/Backends/DX12GUIBackend.cpp:34`
  `ImGui::CreateContext()` 직전에 `ImGui::SetAllocatorFunctions(alloc, free, nullptr)`(`imconfig.h`에 주석만 있고 호출자 0). ImPlot은 ImGui 할당자를 따른다. 드로우 리스트·정점 버퍼가 1MB를 넘을 수 있어 2-1(대형 블록 경로 또는 페이지 성장) 뒤에만 한다. GUI 모듈 해제(`HGUIModule::ShutdownModule` → `ImGui::DestroyContext`)가 메모리 시스템 해제보다 앞서는지 확인.
  완료 조건: GUI 회귀 없음(메뉴·DevFeature·통계 위젯), 종료 시 Allocate == Deallocate, 2-2 위젯에서 GUI 할당이 클래스별 사용률에 반영됨.

- [ ] **5-2. rapidjson 할당자 재정의** — `Source/PCH/PCH.h` 또는 빌드 정의, `Source/Runtime/Core/FileIO/Json.h:314` — `결정 필요`
  `RAPIDJSON_DEFAULT_ALLOCATOR`/`RAPIDJSON_DEFAULT_STACK_ALLOCATOR`를 프로젝트 전역에서 `HAllocator` 기반 어댑터(`Malloc`/`Realloc`/`Free` 정적 함수 3개, `kNeedFree = true`)로 재정의한다. 정의 위치는 rapidjson 헤더가 처음 포함되는 `PCH.h` 앞이어야 하며, 헤더 툴·빌드 툴 프로그램도 같은 PCH를 쓰는지 확인(쓰면 그쪽도 메모리 시스템이 있어야 함).
  `Realloc`은 풀에 없으므로 할당·복사·해제로 구현. 큰 에셋 JSON은 단일 요청이 1MB를 넘으므로 2-1 뒤에만. 에셋 로드 스레드에서 도므로 2-3 뒤에만.
  완료 조건: 에셋 저장·로드(`SaveObject`/`LoadObject`, `Sample.jgasset`) 회귀 없음, 1MB 초과 JSON 임시 테스트 통과, 종료 시 Allocate == Deallocate.

- [ ] **5-3. Phase 5 검증** — 공통 검증 루프 + 5-1 GUI 확인 + 5-2 에셋 왕복. v3 기준선 대비 클래스별 live 증가량과 페이지 성장 횟수 기록.

- [ ] **5-4. 커밋** — "ImGui·rapidjson 할당을 메모리 풀로" (사용자가 직접 커밋).

---

## Phase 6. 관측 (현황분석 D)

- [ ] **6-1. 프레임별 클래스 live 델타 경고** — `Source/Runtime/Core/Memory/MemoryPool.cpp`, `Memory.cpp`(`Update`)
  2-2 카운터로 프레임 끝마다 클래스별 live를 기록하고, N프레임(기본 300) 연속 증가하는 클래스가 있으면 Warning 로그 1회("32 B: +3/frame for 300 frames"). 이번 PSO 캐시 누수(프레임당 +3)가 자동으로 잡히는 기준. 2-1의 성장 Warning과 함께 보면 "무엇이 자라는가"까지 나온다.
  완료 조건: 임시로 매 프레임 `HHashMap` 노드 하나를 누수시키는 코드에서 300프레임 뒤 경고 1줄, 제거 후 경고 없음.

- [ ] **6-2. 콜사이트 태깅(옵션)** — `JG_MEMORY_TRACE` 빌드에서 `Allocate<T>`에 `typeid(T).name()`을, `HAllocator<T>`에 `T` 이름을 헤더 옆 보조 표에 기록해 6-1 경고에 "누가"를 붙인다. 기본 빌드 비용 0.
  완료 조건: 6-1 임시 누수 경고에 타입 이름이 붙는다.

- [ ] **6-3. GPU 힙 계측을 통계 위젯에** — `Source/Runtime/Graphics/DirectX12/Classes/UploadAllocator.h`, `DescriptionAllocator.h`, `ResourceStagingManager.h`, `DirectX12API.h:29-32`
  풀에 넣을 수 없는 영역(현황분석 단계 E 불가 항목)은 대신 센다. 업로드 페이지 수(2MB 단위), 디스크립터 페이지 수(1,024개 단위), 대기·제출 스테이징 요청 수와 바이트, PSO/루트서명 캐시 크기, `_resourceRefCache` 등록 수를 `HJGGraphicsModule`이 노출하고 `JGMemoryStatistcs` 위젯이 GPU 절로 보인다.
  완료 조건: 위젯에 GPU 절 표시, DevScene 실행 중 값이 기대 범위(업로드 페이지 리스트당 1~2개, PSO 캐시 2).

- [ ] **6-4. 시작·종료 스냅샷 비교** — 4-3 누수 리포트를 확장해 시작 완료 시점(모듈 연결 뒤 첫 프레임)과 종료 직전의 클래스별 live를 비교 출력한다.
  완료 조건: 정상 실행에서 차이가 0이거나 설명 가능한 항목(문자열 테이블 등록 수)만 남는다.

- [ ] **6-5. Phase 6 검증** — 공통 검증 루프 + 6-1 임시 누수 테스트(제거 확인) + 위젯 확인.

- [ ] **6-6. 커밋** — "메모리 관측: 클래스별 델타 경고, 콜사이트 태깅, GPU 힙 계측, 스냅샷 비교" (사용자가 직접 커밋).

---

## Phase 7. 정리 · 재검토 (P1-13, P1-15, P1-16, 현황분석 E-6)

- [ ] **7-1. 문자열 테이블 수명 정책** — `Source/Runtime/Core/String/StringTable.cpp:147`(`removeOldStringInfos`), `String/Name.cpp` — `결정 필요`
  정리 로직이 큐에서 꺼낸 ID를 다시 넣지 않아 사실상 동작하지 않고(참조 0이어도 `FrameCount >= INT32_MAX`), 결과적으로 영구 인터닝이다. 권장안: 영구 인터닝으로 확정하고 정리 코드·`FrameCount`·`weak_ptr` 참조 카운트를 제거, `PName`은 `uint64` ID만 든다(복사마다 원자 연산 3회 제거. 3-3의 제어 블록 이전도 불필요해짐). 대안: 정리 로직 복구(큐 재삽입 + 유효 수명값).
  완료 조건(권장안): `sizeof(PName)`이 릴리스 16B(ID 8B + vptr 8B. 2-5는 하지 않기로 함), 디버그는 문자열 사본 포함 56B(2026-09-29 크기 프로브 기준 계산). 이름 비교·JSON 직렬화 회귀 없음.

- [ ] **7-2. 스케줄러 작업 객체 정리** — `Source/Runtime/Core/Thread/Scheduler.h:117, 167`, `Scheduler.cpp`(`Update`)
  `_syncTaskPool`에 `emplace`만 있고 `erase`가 없어 `Schedule*` 호출마다 작업 객체가 종료까지 남는다. 작업이 제거될 때(`bIsRemoveTask`) 풀에서도 지운다. `GUI_TODO.md` 2단계의 `Unschedule` 작업과 겹치면 그쪽 결과를 따른다.
  완료 조건: 임시로 `ScheduleOnce`를 매 프레임 부르는 코드에서 6-1 경고가 나지 않음(제거 확인).

- [ ] **7-3. 캐스트 검사 보강** — `Source/Runtime/Core/Object/ObjectGlobalSystem.h`(`Cast<>`), `Core/Memory/Memory.h`(`RawFastCast`)
  `Cast<T>`가 리플렉션으로 판정하지 못하면 `RawFastCast`(정적 관계만 보는 `static_cast`)로 떨어져 잘못된 다운캐스트가 가능하다. 디버그 빌드에서는 `dynamic_cast`로 검증하고 불일치 시 assert.
  완료 조건: 디버그 빌드 런처 회귀 없음(assert 0).

- [ ] **7-4. `PString::_rawString` 풀 이전 재검토** — `Source/Runtime/Core/String/String.h:20` — `결정 필요`
  1-1로 재귀 위험은 사라졌지만, SSO로 짧은 문자열은 힙을 안 쓰고 `GetRawString()` 44곳·`HRawString` 27곳이 `std::string`을 노출해 spdlog·rapidjson·Win32 접점을 전부 손봐야 한다. 6-4 스냅샷으로 문자열 힙 사용량을 실측한 뒤 이득이 작으면 "보류"로 종결한다.
  완료 조건: 결정 기록(진행 또는 보류)이 `Document/Memory/2026-09-21_Memory_현황분석.md`에 남는다.

- [ ] **7-5. 문서 갱신** — `Document/Memory_현황분석_2026-09-21.md`/`.html` §4 문제 목록에 해결 항목 표시, `Document/Graphics_TODO.md` 5-17·5-18·5-23 종결 확인, `Document/Memory/2026-09-21_Memory_현황분석.md`에 최종 상태와 기준선 대비 수치(상주 메모리, 피크, 클래스별 사용률) 기록.

- [ ] **7-6. 커밋** — "문자열 테이블 수명 정책, 스케줄러 작업 정리, 캐스트 검사, 메모리 문서 갱신" (사용자가 직접 커밋).
