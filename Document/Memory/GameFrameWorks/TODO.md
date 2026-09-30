# GameFrameWorks TODO

갱신 2026-09-30. 구 TODO(`Files/GameFrameWorks_TODO.md`, 2026-09-28 작성 · 09-29 Phase 1 재구성)를 통합 · 최신화했다. **항목 번호는 구 TODO 그대로**(다른 트랙 문서가 참조). 현황은 `현황.md`.
기준: 엔진 기반 구축. "보드게임 · 카드게임 어느 것을 올려도 필요한 것"만 엔진 항목이고, 특정 게임 규칙에 맞춘 항목은 넣지 않는다. `결정 필요` 는 착수 전 사용자 확인. 한 항목이 끝나면 완료 이력 표로 옮기고 근거를 적는다.

---

## 다음에 할 일 Top 5 (권장 순서 — 착수 대상은 사용자가 정한다)

| 순서 | 항목 | 착수 조건 | 완료 기준 |
|---|---|---|---|
| 1 | **1-8. Phase 1 · 병합본 · `gmtest` 이동 커밋** (사용자) | 메인 트리 `gmtest` 111/111 · `net.test all` 66/66 · 2 프로세스 OK 가 그대로인지 재확인(`현황.md` §6). `tasklist` 에 다른 세션 빌드 없음 | `Source/Runtime/GameFrameWorks/**`(`Network/` 포함) + `Source/Runtime/Core/ConsoleCommand/` · `Core/Misc/Module.*` + `Source/Programs/JGConsole/*` + `Bin/DevelopEngine/{GameFrameWorks.dll,JGConsole.exe}` 가 한 커밋(또는 Core 변경이 먼저). `GameMasterSelfTest_*.json` 은 제외 |
| 2 | **2-2. `JGStaticMeshComponent`** + `PWorld::GetScene()` | 1 뒤. Graphics `PScene`(`Source/Runtime/Graphics/Classes/Scene.h`) 있음. 사용 예 `Document/Memory/Graphics/Files/Graphics_PScene_설계방안_2026-09-29.md` "GameFrameWorks에서 쓰는 모습" | `PWorld` 가 `Allocate<PScene>()` 로 장면을 소유 · `GetScene()`. 컴포넌트가 `OnBeginPlay` 에서 `CreateMesh`, `OnTick` 에서 `SetMeshWorldMatrix(GetOwner()->GetWorldMatrix())`, `OnEndPlay` 에서 `DestroyMesh`. 월드 자체 테스트(헤드리스 — `PScene` 은 백엔드 무관 데이터)에 스폰 · 파괴 후 `FindMesh` 검사 추가, `gmtest` 통과. 게임 프로젝트 런처에서 메시 액터가 그려진 캡처 |
| 3 | **2-1. `JGCameraComponent` · `JGCameraActor`** | 2 와 함께(같은 `GetScene`). `PCamera` 빈 클래스는 이미 없음 — `HSceneCamera` 사용 | `CreateCamera/SetCamera/DestroyCamera` 수명, 활성 카메라를 GFW 가 정해 `PSceneRenderer::Render` 에 넘기는 경로 하나. 보드 궤도 · 고정 두 모드. 월드 자체 테스트 + 게임 프로젝트 런처 캡처 |
| 4 | **2-3. `JGGameplayDevViewComponent`** `결정 필요`(부착 경로) | 위젯 경로면 GUI `HGUIModule::OpenWidget<T>` · `JGWidget` 사용 | 상태 인스펙터(엔티티 · 컴포넌트 값) · 이벤트 로그 · Undo 버튼 · 체크섬 표시. 게임 프로젝트 에디터 콘솔/창에서 확인 캡처 |
| 5 | **2-5. 피킹** | 3 뒤(카메라 필요). Core/Math `HRay` · `HPlane` 사용 | `HRay` 대 보드 평면(사각 · 육각 셀) · 카드 사각형 판정, `JGGameplayControllerActor` 에서 좌표 → `HGameplayCoord`/엔티티 ID. 헤드리스 판정 테스트 + 런처에서 클릭 → 명령 초안 확인 |

2-4(시퀀서 확장)와 2-6(프리팹)은 선행 조건(연출 요구 확정 · Core `getJGObject`)이 갖춰질 때 순서에 넣는다.

---

## 미완료 항목 (엔진 범위)

| ID | 항목 | 우선순위 | 상태 | 선행조건 | 완료 기준 | 비고 |
|---|---|---|---|---|---|---|
| 1-8 | Phase 1 + 리슨 서버 병합 + `gmtest` 모듈 선언 커밋 (사용자) | 높음 | 대기 | 검증 3종 재확인 | 위 Top 1 | 커밋 묶음 근거 `현황.md` §4 · §8. 메인 세션의 "GFW 는 DevConsole Core 변경에 기대지 않음" 은 옛말 — `GameFrameWorksModule.cpp` 가 `ConsoleCommandGlobalSystem.h` 포함 |
| 2-1 | `JGCameraComponent` · `JGCameraActor` — 보드 궤도 · 고정 카메라 | 높음 | 미착수 | Graphics `PScene::CreateCamera/SetCamera`(있음), 2-2 의 `PWorld::GetScene` | 위 Top 3 | 구 TODO 의 "`PCamera` 빈 클래스와 관계 정리" 는 소멸(소스에 `PCamera` 없음) |
| 2-2 | `JGStaticMeshComponent` — `PScene` 메시 ID 로 배치 | 높음 | 미착수 | `PScene::CreateMesh/SetMeshWorldMatrix/SetMeshMaterial/DestroyMesh`(있음) | 위 Top 2 | 리소스(메시 · 머터리얼)는 장면이 참조만. 메인 스레드 전용 |
| 2-3 | `JGGameplayDevViewComponent` — ImGui 상태 인스펙터 · 이벤트 로그 · 되돌리기 · 체크섬 | 중간 | 미착수 · `결정 필요` | 부착 경로 결정(위젯 vs 컴포넌트) | 위 Top 4 | 백로그 "리플렉션 인스펙터로 상태 편집" 은 이 항목의 확장 |
| 2-4 | GameMasterActor 시퀀서 확장 — 인과 기반 병렬 묶음 · 페이싱 · 스킵 키 | 중간 | 미착수 | 연출 요구(게임 프로젝트 첫 연출)가 이벤트 정보량을 정한 뒤. 옛 1-4 "이벤트 묶음 번호"(`CauseSequence` · `Depth` 활용)를 여기서 함께 | 같은 원인의 이벤트가 한 묶음으로 병렬 재생, 묶음 사이 페이싱, 스킵 시 바인딩 결과가 순차 재생과 동일(월드 자체 테스트로 비교) | 백로그 R9(C17, `Start` 와 첫 명령의 `CauseSequence` 동일)를 이때 같이 고친다 |
| 2-5 | 피킹 — `HRay` 대 보드 평면/헥스 · 카드 사각형, Controller 연결 | 중간 | 미착수 | 2-1 | 위 Top 5 | Core `Input/` 은 `Key.h` 하나 — 마우스 입력 경로는 GUI/플랫폼 쪽 확인 필요 |
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
| 0-9 후속 | 별칭 `simtest` 제거 · `gmtest` 를 `Core/GameFrameWorksModule.cpp` 에 `HAutoConsoleCommand` 로 선언 (DevConsole 트랙, 사용자 결정) | 2026-09-30 | `Source/Runtime/GameFrameWorks/Core/GameFrameWorksModule.cpp`, `Document/Memory/Etc/Files/2026-09-29_DevConsole_현황분석.md` "2026-09-30 후속" (미커밋) |
| 1-1 | 죽은 ID 가드 (C1 · D5, R1–R3) — `Add<T>` → `T*`, `SetBoardPosition` · `MoveToZone` 거부 | 2026-09-30 00:15 | `Files/2026-09-29_gfw_phase1_verify.txt`, `gmtest` 9 절. 코드 `GameMaster/State/GameplayState.h:122-130, 168-169` |
| 1-2 | 스냅샷 스택 (C3 · D7, R16 · R10) — noexcept 이동, `HDeque` + `pop_front`, ×2.7 → ×1.0 | 2026-09-30 00:15 | 같은 파일. 코드 `Services/GameplaySnapshotStack.h`, `State/GameplayState.h:30-32` |
| 1-3 | 월드 · 연출 수명 (C4 R11 · R15, C8 R12, C5 R13) | 2026-09-30 00:15 | 같은 파일, 월드 자체 테스트 19. 코드 `Core/World.cpp flushPendingSpawn`, `Actors/Actor.cpp GetWorldPosition`, `Actors/GameMasterActor.h:101-104` |
| 1-4 | 턴 전이 순서 (C6 · D1, R6a · R6b) — `EGameplayPhaseStep` · `PendingStep` | 2026-09-30 00:15 | 같은 파일, 12 절. 코드 `GameMasterDefines.h:23-30`, `State/GameplayTurnState.h:13` |
| 1-5 | 트리거와 선택 요청 (C2 · C9 · C10 · D2, R4 · R5 · R8) — `Match`/`React(HGameplayTriggerContext&)`, `RequestChoice` bool, 한도 64 · 4096 | 2026-09-30 00:15 | 같은 파일, 12 절. 코드 `Rules/GameplayTrigger.h:20`, `Rules/GameplayContext.h:36, 55-72`, `GameMasterDefines.h:100-102` |
| 1-6 | 불러오기 후 트리거 순서 (C7, R7) — `ImportDocument` · `Replay` 가 `Finalize` | 2026-09-30 00:15 | 같은 파일, 10 절. 코드 `GameMaster/GameMaster.cpp:159, 324, 338`. 리슨 서버 트랙 0-1 과 동일 수정 병합 |
| 1-7 | 읽기 전용 경계 (C11 · D4, R17) — const 오버로드 | 2026-09-30 00:15 | 같은 파일, static_assert. 코드 `State/GameplayState.h:65, 93, 113` |
| 3-1 (엔진 측) | 게임 모듈 템플릿 — `Build/Templates/GameProject`: `ConnectModule("GameFrameWorks")` → `SetEntryClass<JG{PROJECT_NAME}EntryActor>()` → `LoadWorld`, 엔트리 액터 JGCLASS 리플렉션 등록 통과 (게임 프로젝트 트랙 구현, 미커밋) | 2026-09-30 | `Document/Memory/GameModule/Files/2026-09-29_게임프로젝트_구현검증.txt` §4 ("GameFrameWorks module startup → MyGame entry actor entered world", 종료 0 · 오류 0), `Document/Memory/GameModule/Files/게임프로젝트_생성가이드.md` |

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
