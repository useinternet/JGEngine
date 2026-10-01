# GameFrameWorks TODO

갱신 2026-10-01 13:3x — **사용자 커밋 `736ddbe` "엔진 구현"(10-01 12:16)에 Phase 2(2-1 · 2-2 · 2-3 · 2-5) · C-3 · 2-7(ER-009 흐름 교체)이 모두 들어갔다**(1-9 완료). 이전: 10-01 11:1x 2-7 완료, 09-30 밤 Phase 2 · C-3 완료. 구 TODO(`Files/GameFrameWorks_TODO.md`, 2026-09-28 작성 · 09-29 Phase 1 재구성)를 통합 · 최신화했다. **항목 번호는 구 TODO 그대로**(다른 트랙 문서가 참조). 현황은 `현황.md`.
기준: 엔진 기반 구축. "보드게임 · 카드게임 어느 것을 올려도 필요한 것"만 엔진 항목이고, 특정 게임 규칙에 맞춘 항목은 넣지 않는다. `결정 필요` 는 착수 전 사용자 확인. 한 항목이 끝나면 완료 이력 표로 옮기고 근거를 적는다.
**게임 UI(`UI/`, `JGGameWidget` · CommonUI식, 2026-10-01)는 따로 `TODO_GameUI.md`(GG-*)에서 관리한다** — 현황 `현황.md` §10.

---

## 다음에 할 일 Top 4 (권장 순서 — 착수 대상은 사용자가 정한다)

| 순서 | 항목 | 착수 조건 | 완료 기준 |
|---|---|---|---|
| 1 | **C-1. 게임 템플릿 의존성** (GameModule 트랙과 조정) | 없음. `Build/Templates/GameProject` 는 GameModule 트랙 파일 | 새 게임 프로젝트에서 `Actors/CameraActor.h` · `Components/StaticMeshComponent.h` · `Data/DataTable.h` 를 include 해도 컴파일된다: 템플릿 `module.json` 에 `Asset` · `Graphics` 추가(간단) 또는 JGBuildTool 이 의존 모듈의 include 경로를 전이(근본, `BuildTool.cpp:325-344`) |
| 2 | **2-4. 시퀀서 확장** | 연출 요구(게임 프로젝트 첫 연출)가 이벤트 정보량을 정한 뒤 | 아래 표 |
| 3 | **2-6. 프리팹** | Core 트랙의 `PJsonData::getJGObject` 미등록 타입 처리 | 아래 표 |
| 4 | **B-4. 시야 기본 구현** | 없음 (지금 착수 가능한 엔진 항목) | 아래 표 |

### 커밋 — 완료 (`736ddbe`, 2026-10-01 12:16, 사용자)

09-30 밤 · 10-01 오전의 미커밋 GFW 변경(1-9 Phase 2, C-3 위젯 이관, 2-7 흐름 교체)과 같은 파일을 고친 Server Phase 5 · 게임 UI(GG-*) · GUI 변경이 한 커밋에 들어갔다(새 `GameplayFlow.h` · `GameplayRoundTurnFlow.*` 추적, `GameplayPhaseMachine.*` 삭제 반영 확인). 커밋 체크리스트 두 개(Phase 2 · 2-7)는 할 일을 마쳐 지웠다 — 내용은 이 파일의 git 이력. 커밋 뒤 메인 트리 GFW 미커밋은 DataTable 트랙의 `Data/`(미추적 18파일)와 이 문서 갱신뿐(13:2x `git status`).

### C-3 이관 목록 (GFW → JGEditor) — **완료 2026-09-30 22:4x (JGEditor 세션), 커밋 `736ddbe`**

아래 표대로 옮겼다: `JGWorldView` → `Source/Editor/JGEditor/Widgets/SceneViewport.h/.cpp` `JGSceneViewport`(창 "Scene Viewport"), `JGGameplayDevView` 는 이름 그대로 `Source/Editor/JGEditor/Widgets/GameplayDevView.h/.cpp`. 메뉴 `Windows/Scene Viewport` · `Windows/Gameplay DevView` 는 `JGEditor.cpp` `openDefaultWidgets` 가 등록하고, 씬 뷰포트는 `connectProjectModules` 뒤 `openSceneViewport` 가 `JGGameInstance::HasInstance()` 일 때 연다. GFW 는 GUI include · `HGUIModule` 참조 0, `GameFrameWorks.module.json` 의존 `Core, Asset, Graphics`. 근거 `Document/Memory/Etc/Files/2026-09-30_jgeditor_C-3_verify.txt`, 이후 창 쪽 할 일은 `Document/Memory/Etc/TODO_JGEditor.md`.

| 옮길 것 (GFW 에서 지운다) | JGEditor 에서 |
|---|---|
| `Widgets/WorldView.h/.cpp` (`JGWorldView`, 창 "World View") | **SceneViewport** 로 이름을 바꾼다(사용자 결정). 렌더는 `OnUpdate`(GUI 1-2 에 기댐), 표시 · 입력은 `OnGenerateGUI` 그대로 |
| `Widgets/GameplayDevView.h/.cpp` (`JGGameplayDevView` + `PGameplayDevViewObserver`, 창 "Gameplay DevView") | 이름은 이관하는 쪽이 정한다 |
| `Core/GameFrameWorksModule.cpp` 의 `registerWidgets()` · `closeWidgets()` 와 그 호출, `GUIModule.h` · `Widgets/*.h` include | 에디터가 메뉴를 등록한다. GFW 는 프로젝트 모드에서 게임 모듈이 에디터 `openDefaultWidgets` **뒤에** 연결하므로, 창은 GFW 가 없을 때 안내 문구만 그리게 두고 자동 열기는 `connectProjectModules` 뒤에 |
| `GameFrameWorks.module.json` 의 `"GUI"` 의존 | 위젯이 빠지면 GFW 에 GUI 사용처가 없다. `PSceneRenderer` 사용도 빠진다(GFW 는 `PScene` 데이터만) |
| `Files/tools/gfw_phase2_testgame/GfwViewModule.cpp` 의 `OpenWidget<JGGameplayDevView>()` | 게임 모듈이 에디터 위젯을 참조하면 안 된다 → 에디터가 열거나 검증 절차에서 메뉴로 연다. `imgui.ini` · `gfw_worldview_verify.ps1` 의 창 이름 · 좌표도 새 이름에 맞춘다 |

GFW 에 남는 것 (두 창이 부르는 순수 로직, 이관 뒤에도 그대로): `JGGameInstance::Get().GetWorld()`, `PWorld::GetScene()` · `GetActiveCamera()` · `PickActor()`, `JGCameraComponent::GetSceneCameraID()` · `ViewportPointToRay()`, `JGCameraActor::GetMode()` · `AddOrbitInput()`, `JGGameplayControllerActor::HandleClick()` · `HasLastPick()` · `GetLastPick()` · `HGameplayPickResult::ToString()`, `PGameMaster`(`GetState` · `Checksum` · `Undo` · `UndoCount` · `IsUndoEnabled` · `AddObserver/RemoveObserver`), `IGameplayObserver`, `IGameplayComponentTable::WriteEntity`. GUI 모듈에 남는 것: `HGUI::Button` · `Separator` · `CollapsingHeader` · `InteractiveImage`(범용 GUI 함수). 월드 자체 테스트(`gmtest` 64)는 위젯을 쓰지 않아 영향이 없다.

---

## 미완료 항목 (엔진 범위)

| ID | 항목 | 우선순위 | 상태 | 선행조건 | 완료 기준 | 비고 |
|---|---|---|---|---|---|---|
| C-1 | 게임 템플릿 `module.json` 의존성 또는 JGBuildTool include 경로 전이 (GameModule 트랙 조정) | 중간 | 대기 · `결정 필요` | 없음 | 위 Top 1 | 검증 게임은 `Files/tools/gfw_phase2_testgame/GfwView.module.json` 처럼 직접 추가했다. 10-01: GFW 안 `Data/DataTable.h`(DataTable 트랙)도 게임 모듈에 `Asset` 의존을 요구한다 — 같은 문제가 하나 더 |
| 2-4 | GameMasterActor 시퀀서 확장 — 인과 기반 병렬 묶음 · 페이싱 · 스킵 키 | 중간 | 미착수 | 연출 요구(게임 프로젝트 첫 연출)가 이벤트 정보량을 정한 뒤. 옛 1-4 "이벤트 묶음 번호"(`CauseSequence` · `Depth` 활용)를 여기서 함께 | 같은 원인의 이벤트가 한 묶음으로 병렬 재생, 묶음 사이 페이싱, 스킵 시 바인딩 결과가 순차 재생과 동일(월드 자체 테스트로 비교) | 백로그 R9(C17, `Start` 와 첫 명령의 `CauseSequence` 동일)를 이때 같이 고친다 |
| 2-6 | 프리팹(액터 서브트리 에셋 + 델타) | 낮음 | 미착수 | **Core 트랙**: `PJsonData::getJGObject` 미등록 타입이 실패 대신 `Allocate<JGObject>()` 반환(`Source/Runtime/Core/FileIO/Json.cpp:43-52`) → 클래스 개명 시 널 역참조(조사 §5-4-9). 메모리 A-2(빈 `PSharedPtr` 대입)는 완료 | `JGActor` 서브트리(컴포넌트 · 자식) JSON 왕복 + 인스턴스 델타 + 미등록 타입 로드가 오류로 끝남(크래시 아님) 테스트 | 다형 직렬화(`PSharedPtr<T>` `[JGType, 데이터]`)는 이미 동작 |
| B-1 | 백로그 R14 (C12) — 비 리플렉션 `JGGameplayCue` 파생이 기본 큐로 재생 | 낮음 | 미착수 | 게임이 `RegisterCue<T>()` 로 JGCLASS 아닌 큐를 등록할 때 | `CreateInstance` 를 팩토리 저장 또는 가상 `Clone` 으로, 재현 코드 R14 `[NOT REPRODUCED]` + `gmtest` 회귀 | `Actors/GameplayCue.cpp:26-28` |
| B-2 | 백로그 리뷰 잔여 C13–C27 · D6 · D9 · D10 · D12 | 낮음 | 미착수 | 쓸 곳이 생길 때 | 항목별 리뷰 "수정 방향" | 대표: C13 `Zone(name)` 자동 생성 · 참조 무효화, C14 관찰자 순회 중 변경, C15 상태 교체 시 `_bufferedCommands`, D9 `RegisterAllFromReflection` 범위 · NAME_NONE 거부 · 큐 프로토타입 정렬, D10 저장 헤더에 GameSchema · 규칙 지문(`RulesFingerprint` API 는 있음, 헤더에는 `SchemaVersion` 만), D12 육각 보드 마름모(오프셋 좌표/셀 마스크) |
| B-3 | 헤드리스 러너(옛 1-2) — 명령 스크립트 파일 실행, `-repeat=N` | 낮음 | 보류 | 게임 규칙을 올리기 시작할 때. JGConsole 명령은 `.cpp` 하나에 `HAutoConsoleCommand`(`Document/Memory/Etc/Files/DevConsole_TODO.md` "명령 등록 방법") | 스크립트 실행 → 체크섬 출력, 반복마다 `GMemoryGlobalSystem::Flush()`(JGConsole 은 GC 가 돌지 않음) | 명령 이름은 DevConsole 문서가 `simrun` 으로 가정 — 시스템 이름에 맞춰 정할 것 |
| B-4 | `IGameplayBoard` 시야 규칙 기본 구현(격자 브레젠험) | 낮음 | 미착수 | 없음 | Square · Hex 의 `HasLineOfSight` 기본 구현 + 자체 테스트 | 보드 추상화의 기본 제공물 |
| B-5 | 저장 버전 테스트(옛 1-5) | 낮음 | 보류 | 저장 파일이 생길 때 | 옛 스키마 파일을 `IGameplayMigrator` 로 읽는 테스트 | |

---

## 게임 프로젝트 영역 (엔진 범위 외 — 게임 프로젝트 트랙 · 게임 코드에서 다룬다)

| 출처 ID | 항목 | 이관 이유 · 엔진 측 접점 |
|---|---|---|
| 2-5 에서 분리 | 피킹 결과(엔티티 ID · 칸 좌표)를 어떤 명령으로 바꿀지, 무엇을 클릭 가능하게 할지(카드 · 말 등 구체 오브젝트), 고를 수 있는 칸 · 대상 규칙과 하이라이트 UI | 게임 규칙 · 콘텐츠. 엔진 접점: 컨트롤러의 클릭 가상 함수 + 명령 조립 API(2026-09-30 사용자 확인: 피킹은 엔진 피킹 시스템, 카드 등은 게임 영역) |
| 3-1 잔여 | 게임 모듈 안 `Sim/`(규칙) · `View/`(연출) 폴더 규약 | 템플릿 · StartupModule 등록 · 리플렉션 엔트리 액터는 게임 프로젝트 트랙이 구현(완료 이력). 폴더 규약은 게임 코드 구조 |
| 3-2 | ImGui 수직 슬라이스 — 한 전투를 끝까지, 시퀀서 이벤트 정보량 검증 | 특정 게임 규칙 · 연출이 필요. 엔진 접점: 2-3 DevView, 2-4 시퀀서(정보량 요구를 이 슬라이스가 정한다) |
| 3-3 | 헤드리스 자동 플레이 통계 | 게임 규칙 위에서만 의미. 엔진 접점: `PGameplayAgentRunner`(있음), B-3 헤드리스 러너 |
| 타이밍 창 모델 | 결과 적용 전 끼어들기("~할 때") · 역순(LIFO) 격발 해결 · 반응 선택 | 게임마다 방식이 달라 기반에 넣지 않음(2026-09-29 결정). 필요한 게임이 오면 효과 큐 확장 요청으로 |
| D3 | 트리거 인스턴스(소유자 · 부여 시각 정렬) | 현 구조(트리거 하나가 해당 컴포넌트를 가진 엔티티들을 순회)로 표현. 게임이 필요하면 컴포넌트 데이터로 |
| D12 일부 | 숨은 정보 모델(비공개 상태) | 목표 장르(협동)에서 불필요. 요구가 생기면 커널 확장 |
| 방향성 §5-2 ~ §5-4 | 카드 게임 · 보드 · cRPG 고유 시스템(덱 · 손패 · 상태 이상 · 대화 · 탐험 등) 및 정의 에셋(콘텐츠) | 콘텐츠 · 게임 규칙. 엔진은 `IJsonable` 컴포넌트 · 영역 · 보드 · 효과 프리미티브만 제공 |

---

## 완료 이력

| ID | 항목 | 완료일 | 검증 근거 |
|---|---|---|---|
| 0-1 | 폴더 · 파일 작성 (Core/ Simulation/ Components/ Actors/, 83 파일, JGConsole 진입) | 2026-09-28 | `Files/2026-09-28_턴제게임_프레임워크_방향성.md` "구현 착수" |
| 0-2 | 코드젠 · 프로젝트 재생성 (`Temp/CodeGen/GameFrameWorks/`, `module_system_info.json`) | 2026-09-28 | 같은 문서 |
| 0-3 · 0-4 | GameFrameWorks.vcxproj · JGConsole.vcxproj 빌드 오류 0 | 2026-09-28 | `Files/build_2026-09-28_gameframeworks.log` |
| 0-5 | `JGConsole.exe simtest` 종료 코드 0 (60/60) | 2026-09-28 | 같은 로그 |
| 0-6 | JGLauncher 회귀 (30초 · 종료 0 · 로그 오류 0) | 2026-09-28 17:40 | `Files/2026-09-28_gfw_launcher_capture.png` |
| 0-8 | 게임 인스턴스 · 엔트리 액터 (`JGGameInstance` · `JGGameEntryActor` · `PWorldSelfTest` 15) | 2026-09-28 18:00 | `Files/2026-09-28_턴제게임_프레임워크_방향성.md` 18:00 항목, 75/75 |
| 0-9 | 이름 변경 Simulation → GameMaster · Gameplay (타입 62 · 파일 72) | 2026-09-29 11:55 | `Files/2026-09-29_gfw_rename_verify.txt`, 대응표 `Files/GameFrameWorks_이름변경안_2026-09-29.md` |
| 0-7 | 1차분 커밋 (사용자, 메모리 Phase 2 와 함께) | 2026-09-29 14:57 | `git log` `d457b92` |
| 0-9 후속 | 별칭 `simtest` 제거 · `gmtest` 를 `Core/GameFrameWorksModule.cpp` 에 `HAutoConsoleCommand` 로 선언 (DevConsole 트랙, 사용자 결정) | 2026-09-30 | `Source/Runtime/GameFrameWorks/Core/GameFrameWorksModule.cpp`, `Document/Memory/Etc/Files/2026-09-29_DevConsole_현황분석.md` "2026-09-30 후속" (커밋 `4c8ac73`) |
| 1-1 | 죽은 ID 가드 (C1 · D5, R1–R3) — `Add<T>` → `T*`, `SetBoardPosition` · `MoveToZone` 거부 | 2026-09-30 00:15 | `Files/2026-09-29_gfw_phase1_verify.txt`, `gmtest` 9 절. 코드 `GameMaster/State/GameplayState.h:122-130, 168-169` |
| 1-2 | 스냅샷 스택 (C3 · D7, R16 · R10) — noexcept 이동, `HDeque` + `pop_front`, ×2.7 → ×1.0 | 2026-09-30 00:15 | 같은 파일. 코드 `Services/GameplaySnapshotStack.h`, `State/GameplayState.h:30-32` |
| 1-3 | 월드 · 연출 수명 (C4 R11 · R15, C8 R12, C5 R13) | 2026-09-30 00:15 | 같은 파일, 월드 자체 테스트 19. 코드 `Core/World.cpp flushPendingSpawn`, `Actors/Actor.cpp GetWorldPosition`, `Actors/GameMasterActor.h:101-104` |
| 1-4 | 턴 전이 순서 (C6 · D1, R6a · R6b) — `EGameplayPhaseStep` · `PendingStep` | 2026-09-30 00:15 | 같은 파일, 12 절. 코드 `GameMasterDefines.h:23-30`, `State/GameplayTurnState.h:13` |
| 1-5 | 트리거와 선택 요청 (C2 · C9 · C10 · D2, R4 · R5 · R8) — `Match`/`React(HGameplayTriggerContext&)`, `RequestChoice` bool, 한도 64 · 4096 | 2026-09-30 00:15 | 같은 파일, 12 절. 코드 `Rules/GameplayTrigger.h:20`, `Rules/GameplayContext.h:36, 55-72`, `GameMasterDefines.h:100-102` |
| 1-6 | 불러오기 후 트리거 순서 (C7, R7) — `ImportDocument` · `Replay` 가 `Finalize` | 2026-09-30 00:15 | 같은 파일, 10 절. 코드 `GameMaster/GameMaster.cpp:159, 324, 338`. 리슨 서버 트랙 0-1 과 동일 수정 병합 |
| 1-7 | 읽기 전용 경계 (C11 · D4, R17) — const 오버로드 | 2026-09-30 00:15 | 같은 파일, static_assert. 코드 `State/GameplayState.h:65, 93, 113` |
| 3-1 (엔진 측) | 게임 모듈 템플릿 — `Build/Templates/GameProject`: `ConnectModule("GameFrameWorks")` → `SetEntryClass<JG{PROJECT_NAME}EntryActor>()` → `LoadWorld`, 엔트리 액터 JGCLASS 리플렉션 등록 통과 (게임 프로젝트 트랙 구현, 커밋 `4c8ac73`) | 2026-09-30 | `Document/Memory/GameModule/Files/2026-09-29_게임프로젝트_구현검증.txt` §4 ("GameFrameWorks module startup → MyGame entry actor entered world", 종료 0 · 오류 0), `Document/Memory/GameModule/Files/게임프로젝트_생성가이드.md` |
| 1-8 | Phase 1 · 리슨 서버 병합 · `gmtest` 모듈 선언 커밋 + 커밋 뒤 회귀 | 2026-09-30 17:30 (커밋) · 21:08 (회귀) | 커밋 `4c8ac73`. 회귀는 그 뒤 다른 세션이 다시 빌드한 메인 트리 Bin 으로: `gmtest` 111/111 · `net.test all` 66/66 · 2 프로세스 OK · 런처 60초 종료 0 (`Files/2026-09-30_gfw_phase2_verify.txt` 0단계) |
| 2-2 | `JGStaticMeshComponent` + `PWorld::GetScene()` — 에셋 비동기 로드 · 렌더 메시 직접 지정 · 머터리얼 덮어쓰기, BeginPlay/틱/EndPlay · 컴포넌트 제거 수명 | 2026-09-30 21:4x (커밋 `736ddbe`) | 헤드리스 11 검사, 게임 프로젝트 에디터에서 X Bot 4 · 타일 36 그려짐 (`Files/2026-09-30_gfw_phase2_worldview_devview.png`) |
| 2-1 | `JGCameraComponent` · `JGCameraActor`(Fixed · Orbit) — 활성 카메라(`PWorld::SetActiveCamera`, 첫 카메라 자동), `ViewportPointToRay`, 궤도 입력 · 제한. 활성 카메라 → `PSceneRenderer::Render` 경로 = `JGWorldView` | 2026-09-30 21:4x (커밋 `736ddbe`) | 헤드리스 10 검사, 에디터에서 궤도 카메라로 그림 · 오른쪽 끌기 · 휠 입력 경로 |
| 2-3 | DevView = GUI 위젯 `JGGameplayDevView` (결정 2026-09-30 사용자) — 요약 · Undo · 체크섬 · 엔티티 → 컴포넌트 JSON(`IGameplayComponentTable::WriteEntity` 추가) · 이벤트 로그 · 마지막 피킹 | 2026-09-30 22:0x (커밋 `736ddbe`) | 캡처 2장(`Files/2026-09-30_gfw_phase2_worldview_devview.png` · `_after_undo.png`): E3.1 → `GfwViewUnit {Health 25, Team 1}`, EndTurn 이벤트 5줄, Undo 뒤 Sequence 1 → 0 · "state replaced" |
| 2-5 | 피킹 시스템 (엔진 범위, 2026-09-30 사용자 확인) — `JGPickShapeComponent`(상자 · 납작한 사각형 · 메시 경계 상자 대체) · `PWorld::PickActor` · `HGameplayBoardLayout`(사각 · 육각 칸 ↔ 월드, `PickCoord`) · `JGGameplayControllerActor::Pick/HandleClick/OnClick` · 월드 뷰 클릭 입력(`HGUI::InteractiveImage`) | 2026-09-30 22:0x (커밋 `736ddbe`) | 헤드리스 24 검사, 에디터에서 실제 클릭 → `entity E3.1 · cell (3, 3)`, 빈 칸 → `cell (5, 0)` (`Files/2026-09-30_gfw_phase2_verify.txt` run3) |
| 1-10 | 에디터에서 `gmtest` 가 게임 월드를 내리던 문제 — 월드 테스트 게임 인스턴스 절(14 검사)을 월드가 있으면 건너뜀, 모듈 주석 정정 | 2026-09-30 21:5x (커밋 `736ddbe`) | 에디터 안 `gmtest` → OK, "game world kept, actors 45 -> 45". JGConsole 은 여전히 64 검사 |
| C-2 | 월드 뷰 렌더를 `OnGenerateGUI` → `OnUpdate` 로 (같은 시각 GUI 세션의 GUI 1-2 로 위젯 `OnUpdate` 가 매 프레임 불리게 됨) | 2026-09-30 22:1x (커밋 `736ddbe`) | run4: 같은 클릭 결과 + 오른쪽 끌기 궤도 회전 캡처 `Files/2026-09-30_gfw_phase2_orbit_drag.png` |
| C-3 | 위젯 두 개를 JGEditor 로 이관(사용자 결정: 위젯은 JGEditor, GFW 는 순수 로직) — `JGWorldView` → JGEditor `JGSceneViewport`, `JGGameplayDevView` 이동, GFW 창 등록 · GUI 의존 삭제, 검증 게임은 DevView 를 메뉴로 연다 (JGEditor 세션) | 2026-09-30 22:4x (커밋 `736ddbe`) | `Document/Memory/Etc/Files/2026-09-30_jgeditor_C-3_verify.txt`: GFW · JGEditor 빌드 exit 0, `gmtest` 156/156, 검증 게임 에디터에서 클릭 → `entity E3.1 · cell (3, 3)` / `cell (5, 0)` (이관 전과 같은 값), DevView 선택 · Undo, 궤도 끌기 |
| 2-7 | GameMaster 흐름 교체 (ProjectAH ER-009, 사용자 결정 2026-10-01 "안 3 최소형") — `IGameplayFlow`(`Start` · `RunPendingStep` · `CanEndTurn` · `EndTurn` · `GetName`) + `PGameMaster::SetFlow`, 옛 페이즈 기계 → 기본 흐름 `PGameplayRoundTurnFlow`(동작 그대로), 턴 상태 공용 칸 `Actors` · `Step` · `NextStep`, 입력 행동자 한 곳(`HGameplayState::CollectInputActors` · `IsInputActor` · `FirstInputActor` — 컨트롤러 · 세션 · AI 러너 · `EnumerateLegal` · DevView 가 씀), `EGameplayPhase::Flow`, 이벤트 `StepChanged` · `FlowStepLimitExceeded`, 명령당 흐름 단계 한도 1024, 규칙 지문에 흐름 이름 | 2026-10-01 11:1x (커밋 `736ddbe`) | `Files/2026-10-01_ER-009_verify.txt`: GFW · JGEditor 빌드 exit 0 · 경고 0, `gmtest` 110/110(§13 흐름 18 새로) + 월드 64/64, `net.test all` 101/101, 2 프로세스 kernel · world OK(체크섬은 공용 칸이 늘어 바뀜, 호스트 = 클라), NetDuel 완주 OK(seq 13, desyncs 0), 에디터 DevView `Step TurnMain · Input E1.1` → Undo 뒤 `Input E0.1`(캡처 `Files/2026-10-01_er009_devview_*.png`). 넣지 않음: 데이터 기반 페이즈 정의, 숨은 정보 동시 선택, 캠페인 관리자 |
| 1-9 | Phase 2 · C-3 · 2-7 커밋 (사용자, 같은 파일의 Server Phase 5 · 게임 UI 와 한 커밋) | 2026-10-01 12:16 | 커밋 `736ddbe` "엔진 구현": `git show --name-status` 로 새 파일(`Components/*` · `CameraActor` · `GameplayBoardLayout` · `GameplayFlow.h` · `GameplayRoundTurnFlow.*` · JGEditor `Widgets/*`) 추적, `GameplayPhaseMachine.*` 삭제 반영 확인. 커밋 소스와 같은 Bin 의 회귀는 11:22 기록(`gmtest` 110 + 64 · `net.test all` 101, `Files/2026-10-01_ER-009_verify.txt` 끝). 커밋 뒤 메인 전체 빌드(12:58, DataTable 세션)에서도 `gmtest` OK · `net.test all` 101 — 그 세션 알림 |

---

## 제외 · 이관 항목

| 출처 | 항목 | 처리 | 이유 |
|---|---|---|---|
| 옛 1-1 | `getJGObject` 미등록 타입 처리 | **Core 트랙으로 이관**(2-6 의 선행 조건으로만 추적) | 커널 상태는 값 타입이라 이 경로를 타지 않음. 프리팹 · 에셋 쪽 문제 |
| 옛 1-3 | 트리거 정렬 키(소유자 · 부여 시각) | 하지 않음 | 트리거 하나가 해당 컴포넌트를 가진 엔티티들을 도는 현 구조로 표현됨(D3 도 같음) |
| 옛 1-4 | 이벤트 묶음 번호 | 2-4 에 합침 | 연출 병렬 재생을 만들 때 함께 |
| 옛 1-6 | 체크섬 직접 해시 | 삭제 | 측정된 병목이 아님 |
| 백로그 | 멀티플레이 대비 명령 스트림 직렬화 규약 문서화 | **Network 트랙으로 이관** | `Network/Messages/GameplayNetMessage.*` 로 구현됨(리슨 서버 Phase 0–4) |
| 백로그 | 사망 연출 뒤 파괴 "옵션" | 삭제 | 1-3 에서 기본 동작으로 처리(파괴 바인딩은 큐 완료 후) |
| 리뷰 D3 | 트리거를 상태 안 인스턴스로 | 하지 않음 → 게임 프로젝트 영역 표 | 위와 같음 |
| 리뷰 "커밋 전 필수 4건"(C1 · C3 · C4 · C8) | — | 완료(1-1 · 1-2 · 1-3) | Phase 1 에 흡수 |
