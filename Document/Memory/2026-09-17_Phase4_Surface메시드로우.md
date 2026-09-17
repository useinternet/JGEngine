# 2026-09-17 작업 기록 — Phase 4. Surface 도메인 메시 드로우

다른 AI 에이전트나 다음 세션이 이어서 작업할 수 있도록 현재 상황과 이번 작업 내용을 기록한다.
할 일 목록은 `Document/Graphics_TODO.md`(Phase 4 전부 `[x]`, Phase 5에 5-17~5-22 추가). 이전 기록은 `2026-09-16_Graphics_현황분석.md`.

## 요청
Graphics_TODO.md의 "Phase 4. Surface 도메인 메시 드로우"(4-1 ~ 4-8) 진행.

## 결과
**Phase 4 완료. X Bot 메시(2 서브메시, 정점 28,464, 인덱스 147,336)가 DevScene에 깊이 테스트된 상태로 그려진다.**

- 전체 빌드(`JGEngine.sln` DevelopEngine|x64) 16/16 오류 0. 신규 경고 0(기존 C4244 4건·LNK4098만 남음). 로그: `build_2026-09-17_phase4.log`
- 런처 실행 → 20초 뒤 캡처 → WM_CLOSE → **ExitCode 0**. 로그 `[error]`/`[critical]` 0, D3D12 디버그 레이어 메시지 0. 경고 1건(`NOT Support Asset Path`, 5-20).
- 증거: `2026-09-17_phase4_capture.png` — 회청색 배경 위에 X Bot이 램버트 음영으로 보이고 팔·다리 겹침이 깊이 순서대로 가려진다.
- 커밋은 하지 않았다(사용자가 직접 커밋). `git status`: 소스 20개 수정, `Document/Memory`에 캡처·빌드 로그·이 기록·`tools/crashwalk/` 추가.

## 렌더링 구조 (DevScene 매 프레임)
1. 지오메트리 패스: G버퍼 4장(Albedo R8G8B8A8 / Normal_Metallic R16G16B16A16F / Specular_Roughness R8G8B8A8 / Depth R32F, 배경 깊이 1.0) + `D24_Unorm_S8_Uint` 깊이 텍스처 클리어 → `SetRenderTarget` → 카메라 상수 → `Draw(HDrawArguments{ Mesh->GetMesh(), 항등 월드 })` → `EndDraw`(RT → PIXEL_SHADER_RESOURCE).
2. 합성 패스: Scene 도메인 머터리얼 `DevSceneCompositeMaterial`(텍스처 프로퍼티 GAlbedo/GNormal/GDepth = G버퍼)이 SceneTexture에 풀스크린. 배경(깊이 ≥ 1)은 단색, 메시는 알베도 × (0.15 + 0.85·N·L).
3. SceneTexture를 ImGui Image로 표시.

카메라: `LookAtLH`/`PerspectiveFovLH`(45°), 메시 로드 시 `CalculateBounds()`로 경계 구에 맞춰 -Z에서 프레이밍. 행렬은 `D3DCOMPILE_PACK_MATRIX_ROW_MAJOR`로 컴파일해 HMatrix(DirectXMath 행 우선)를 전치 없이 memcpy한다(템플릿의 `mul(v, M)` 행 벡터 규약과 맞음).

## 설계 결정
1. **에셋과 렌더 메시 분리.** TODO 4-1은 "`JGStaticMesh`가 `IMesh`를 구현"이었지만, `JGStaticMesh`는 `JGObject`로 이미 `IMemoryObject` 뿌리를 가진다. `IMesh`(IJGGraphicsObject → IMemoryObject)를 다중 상속하면 2026-09-16 메모리 규칙(뿌리 하나·오프셋 0, 다이아몬드 금지) 위반. 그래서 `PStaticMesh : IMesh`(렌더 메시, 서브메시 VB/IB/머터리얼 약참조 목록 보유)를 두고 `JGStaticMesh`가 `GetMesh()`로 지연 생성해 돌려준다. 서브메시 버퍼는 두 객체가 `PSharedPtr`로 공유하며 `SetMaterial`은 둘을 함께 갱신한다. `Scene.h`에 이미 `class PStaticMesh;` 전방 선언이 있었다.
2. **GPU 정점은 POD.** `HVertex : IJsonable`은 vptr 8바이트가 앞에 붙어(sizeof 64) 그대로 올리면 입력 레이아웃 오프셋과 어긋난다. `struct HVertexData`(56바이트, static_assert)를 분리하고 `HVertex : HVertexData, IJsonable`로 두었다. 업로드 시 슬라이스 복사, 직렬화 시 역변환(`HStaticSubMesh::SetData/GetVertices`). 에셋 JSON 포맷은 그대로.
3. **static 저장소에 엔진 컨테이너 금지** (아래 종료 크래시). `GetInputLayout()`은 값으로 돌려주고 `PStaticMesh`가 멤버로 보관.
4. **머터리얼 프로퍼티 레이아웃은 Initialize에서 한 번.** 이전에는 `Compile()` 안에서 CB를 다시 만들어 컴파일 전 `Set*`가 무효였다. 이제 `buildPropertyLayout()`이 CB(256 정렬)·오프셋 맵·텍스처 슬롯(기본 텍스처)·슬롯 인덱스 CB 기록을 만들고, `Compile()`은 코드 생성·컴파일만 한다. 기본 텍스처는 `PDirectX12API::CreateRawMaterial`이 `Initialize(args, _defaultTexture)`로 넘긴다.
5. **텍스처 토큰 치환은 식별자 경계 기준.** 이전 "이름+공백" 치환 대신 `replaceIdentifier`(앞뒤가 `[A-Za-z0-9_]`가 아닐 때만). `GDepth.r` 같은 표현이 그대로 된다.
6. **서브메시는 기본 머터리얼 약참조를 들지 않는다.** `IMesh::GetMaterial()`이 비어 있으면 그릴 때 기본 머터리얼로 대체하므로, API 소유 객체를 약참조로 오래 들고 있어 종료 순서에 걸릴 이유가 없다.

## 변경 파일 (20개)
| 모듈 | 파일 | 내용 |
|---|---|---|
| Graphics | `JGGraphicsDefine.h` | `HVertexData` 분리, `GetInputLayout()` 값 반환(static 누적 버그 제거), `ETextureFilterMode`/`ETextureWrapMode`에 `JGENUMMETA()` |
| Graphics | `Classes/Mesh.h` | `IMesh : IJGGraphicsObject` API(서브메시 수, VB/IB/머터리얼, 입력 레이아웃) |
| Graphics | `Classes/StaticMesh.h/.cpp` | `HStaticSubMesh::SetData/GetVertices/GetIndices`, `PStaticMesh`, `JGStaticMesh::GetMesh/CalculateBounds/OnLoadAsset_Thread`, `GetVertex`가 `HVertexData` 반환 |
| Graphics | `JGGraphicsCommand.h` | `ClearDepthTexture` 2종 추가, 주석 블록 정리 |
| Graphics | `DirectX12/DX12GraphicsCommand.h/.cpp` | `Draw(HDrawArguments)`, `ClearDepthTexture`, `bindMaterial` 공용화, VB 스트라이드, C4244 캐스트 |
| Graphics | `Classes/Material.h` | `IRawMaterial::GetTextureCubes()` |
| Graphics | `DirectX12/DX12Material.h/.cpp` | 프로퍼티 레이아웃/텍스처 슬롯/`SetTexture`/`GetTexture`/`replaceIdentifier`, CB 이름 선지정 |
| Graphics | `DirectX12/DirectX12API.cpp` | `Initialize(args, _defaultTexture)` |
| Graphics | `DirectX12/Classes/DX12Shader.cpp` | `D3DCOMPILE_PACK_MATRIX_ROW_MAJOR` |
| Asset | `AssetDatabase.h/.cpp` | 결과 저장·`bCompleted`·뮤텍스·등록 순서·실패 콜백·`releaseAssets()`(소멸자/Destroy) |
| Asset | `AssetModule.cpp` | `ShutdownModule`에서 `UnRegisterSystemInstance<GAssetDatabase>()` |
| Asset | `AssetPath.cpp` | 엔진 경로 정규화(역슬래시, 앞 슬래시, 확장자) |
| Core | `String/String.h` | `PString::Format`의 PName 댕글링 수정(5-13) |
| Core | `String/Name.cpp` | `PName::copy/move` 자기 대입 보호 |
| Devkit | `DevScene.h/.cpp` | G버퍼·깊이·합성 머터리얼·비동기 메시 로드·카메라·2패스 렌더 |

`Temp/CodeGen/Graphics/JGGraphicsDefine.generation.cpp`는 JGHeaderTool 재실행으로 갱신됨(git 미추적). 다른 환경에서는 `Build/BatchFiles/PreBuild.bat`(헤더 툴)를 먼저 돌려야 enum 이름이 채워진다.

## 이번 세션에서 잡은 버그 (원인 → 수정)
1. **합성 머터리얼 컴파일 실패 `undeclared identifier 'null'`** — 샘플러 이름이 `_(null)(null)_`. `ETextureFilterMode`/`ETextureWrapMode`의 코드젠 이름 목록이 비어 있었다(JGENUMMETA 없는 열거자는 툴이 빼먹음) → `JGENUMMETA()` 추가 + 헤더 툴 재실행. 5-19로 툴 수정 등록.
2. **에셋 로드가 완료로 잡히지 않음 / 항상 "Loaded Asset is nullptr"** — 위 4-6 참고. 기존 "간헐적 실패"(5-15)의 실제 원인.
3. **정점 버퍼 이름이 "(null)"** — `HStaticSubMesh::ReadJson`이 멤버 `Name`을 `SetData(Name, ...)`에 참조로 넘겨 `Name = inName` 자기 대입 → `PName::copy`가 `reset()` 후 복사해 NAME_NONE. `PName::copy/move`에 guard, `SetData`는 값으로 받음.
4. **종료 시 0xC0000005 (Graphics.dll)** — 두 겹이었다.
   - `GAssetDatabase::_assetPool`이 종료까지 메시를 붙잡아 GC 객체가 메모리 시스템 소멸의 강제 정리(`~GMemoryGlobalSystem → forceFlush`)까지 살아남았다. 그 시점엔 기본 머터리얼 블록(약참조 카운터 포함)이 이미 해제되어 있어 힙이 깨진다. → Asset 모듈 종료 시 데이터베이스 해제(`DisconnectModule`이 바로 뒤에 GC Flush를 돌려 Graphics 생존 중에 메시가 정리됨) + 서브메시의 기본 머터리얼 약참조 제거.
   - 그래도 남은 크래시는 **`HVertexData::GetInputLayout()`의 함수 지역 static `HInputLayout`**. `HList`가 `HAllocator`(메모리 풀)라서 DLL 언로드 시 atexit 소멸자가 이미 `free`된 풀 메모리의 디버그 이터레이터 프록시를 읽었다(콜스택: `dynamic atexit destructor for 'inputLayout'` → `~HInputLayout` → `vector::_Tidy` → `_Orphan_all` → AV). → 값 반환 + `PStaticMesh` 멤버. 같은 패턴이 `HDirectXAPI` 폴백 static에 남아 있어 5-17로 등록.

## 검증 방법 (재현용)
- 빌드: `MSBuild.exe JGEngine.sln -p:Configuration=DevelopEngine -p:Platform=x64 -m` (약 4분).
- 실행·캡처: 런처를 `Bin/DevelopEngine` cwd로 실행, 20초 대기, `PrintWindow(PW_RENDERFULLCONTENT)`로 메인 창 캡처, `WM_CLOSE`, ExitCode 확인. 스크립트는 세션 스크래치패드에 있었으므로 필요하면 위 순서대로 다시 작성.
- 종료 크래시 콜스택: `Document/Memory/tools/crashwalk/`의 미니 디버거(DbgHelp StackWalk64, `build.bat`로 cl 컴파일). `crashwalk.exe <JGLauncher.exe 경로> <Bin/DevelopEngine> <N초 뒤 WM_CLOSE>` → 예외 발생 시 심볼 붙은 스택을 stdout에 찍는다(런처의 콘솔 로그와 섞이므로 파일로 리다이렉트해 `=== Exception`을 grep). cdb/WinDbg가 없는 이 PC에서 쓴 방법.
- 로그: `Bin/DevelopEngine/jg_log.txt`에서 `[error]`, `[critical]`, `D3D12 DebugLayer`, `Success Load Asset`, `DevScene :` 확인.

## 다음 작업자에게
- Phase 5 순서 제안: 5-17(폴백 static 컨테이너, 종료 크래시 재발 방지) → 5-18(HTaskHandle/WeakCount) → 5-7(PCamera/PScene, 5-22와 함께 DevScene의 패스 소유권 이관) → 5-21(머터리얼 재컴파일) → 5-14(스왑체인 색 공간, 지금은 FP16 선형이라 회색이 밝게 보임).
- `JGStaticMesh::SetMaterial`은 `_subMeshes`와 `_mesh` 양쪽을 갱신한다. 서브메시 목록을 바꾸는 새 API를 추가하면 `_bMeshDirty = true`를 잊지 말 것.
- 텍스처 프로퍼티는 `Compile()` 전에 `SetTexture`로 넣는 것이 기본 사용법(샘플러 이름이 코드에 박힘).
- 루트 `CLAUDE.md` 지침: 보고/분석 문서는 `Document/`, 인수인계는 `Document/Memory/`. 문서는 로컬 파일로만(외부 게시 금지). 커밋은 사용자가 직접.
