# GameFrameWorks 할 일 목록 (순차 진행용)

작성 2026-09-28, Phase 1 재구성 2026-09-29. 근거는 `Document/GameFrameWorks_구현현황_2026-09-28.md`, 리뷰 `Document/GameFrameWorks_설계코드리뷰_2026-09-28.md`, 설계 문서(`게임프레임워크_설계방안_요약`, `설계방안B_상세설계`, `설계방안B_인터페이스_구현가이드`).
위에서부터 순서대로 진행한다. 한 항목이 끝나면 `[x]`, 한 단계가 끝나면 `Document/Memory/` 에 진행 상황을 한 줄 추가한다.
`결정 필요` 표시는 착수 전 사용자 확인.

---

## Phase 0. 1차 구현 빌드 통과

- [x] **0-1. 폴더 · 파일 작성** — 완료 2026-09-28. Core/ GameMaster/ Components/ Actors/ 83개 파일(헤더 44 · cpp 39), JGConsole gmtest 진입.
- [x] **0-2. 코드젠 · 프로젝트 재생성** — 완료 2026-09-28. `Temp/CodeGen/GameFrameWorks/` 11 클래스, `module_system_info.json` 에 GameFrameWorks, JGConsole 이 GameFrameWorks 링크.
- [x] **0-3. GameFrameWorks.vcxproj 빌드 오류 0** — 격리 워크트리(HEAD `fcace8f` + 이 구현)에서 완료 2026-09-28. 첫 빌드 오류 17건(이벤트 한 인자 생성의 most vexing parse 5곳, `GameplayAgent.h` include 누락) 수정. 작업 트리 재확인은 0-6 과 함께.
- [x] **0-4. JGConsole.vcxproj 빌드 오류 0** — 격리 워크트리에서 완료.
- [x] **0-5. `JGConsole.exe gmtest` 종료 코드 0** — 격리 워크트리에서 60/60 통과, 로그 `[error]` 0. 작업 트리 재확인은 0-6 과 함께.
- [x] **0-6. JGLauncher 회귀** — 완료 2026-09-28 17:40. 작업 트리에서 GameFrameWorks · JGConsole 빌드 오류 0, `gmtest` 60/60, 런처 30초 실행 · 종료 코드 0 · 로그 오류 0(`2026-09-28_gfw_launcher_capture.png`, `build_2026-09-28_gameframeworks.log`). 런처는 GameFrameWorks 를 연결하지 않고 코드젠 DLL 로만 로드한다(클래스 목록에 JGActor 등 등장).
- [x] **0-8. 게임 인스턴스 · 엔트리 액터** — 완료 2026-09-28 18:00. 월드 소유를 게임 모듈에서 엔진으로: `Core/GameInstance`(`JGGameInstance`: LoadWorld · UnloadWorld · GetWorld · SetEntryClass · OnWorldLoaded/Unloading, 게임 파생 가능), `Actors/GameEntryActor`(`JGGameEntryActor`: OnEnterWorld/OnExitWorld), 모듈은 생명주기만(`ReplaceGameInstance<T>`). `Core/WorldSelfTest` 15개 헤드리스 검사 추가(`gmtest` 가 함께 실행). GameMasterActor 의 큐 없는 이벤트 소진이 프레임당 1개였던 것을 같은 프레임에 연속 소진으로 수정.
- [x] **0-9. 이름 변경 Simulation → GameMaster · Gameplay** — 완료 2026-09-29 11:55. 규칙: 파사드 `PGameMaster` 와 그것을 든 월드 액터 `JGGameMasterActor` 및 시스템 단위 이름(폴더 `GameMaster/` · 정의 파일 · 자체 테스트 · 로그 카테고리)은 GameMaster, 그 밖의 타입은 Gameplay. 엔티티 액터 `JGGameplayEntityActor`, 콘솔 명령 `gmtest`(옛 이름 `simtest` 도 받음). 타입 62 · 파일 72 이동, 리뷰 재현 코드 함께 치환. 변경 전후 자체 테스트 · 리뷰 재현 결과 줄 단위 동일, 런처 회귀 통과. 대응표 `Document/GameFrameWorks_이름변경안_2026-09-29.md`, 증거 `Document/Memory/2026-09-29_gfw_rename_verify.txt`.
- [ ] **0-7. 커밋** — 사용자가 직접. **순서: 리뷰 기록대로 메모리 트랙 Phase 2(성장형 풀)와 함께 또는 그 뒤** — HEAD 의 고정 풀에서는 스냅샷 스택이 약 95 명령에서 크래시한다(리뷰 R16). 이 트랙의 변경: `Source/Runtime/GameFrameWorks/**`(신규 89개, `Main.cpp` 삭제), `GameFrameWorks.module.json`, `Source/Programs/JGConsole/{Main.cpp,JGConsole.module.json}`, `Document/GameFrameWorks_*`, `Document/Memory/2026-09-28_*`, `Bin/DevelopEngine/GameFrameWorks.{dll,lib,exp}`. **`Source/Runtime/Core/Memory/*` 와 `Devkit/DevScene.cpp` 의 변경은 메모리 트랙(다른 세션)의 것이다.**

## Phase 1. 커널 버그 수정 (리뷰 재현 항목)

2026-09-29 재구성. 기준은 "보드게임 · 카드게임 어느 것을 올려도 밟는 결함"이다. 특정 게임의 규칙에 맞춘 항목은 넣지 않는다.
근거와 재현 방법은 `Document/GameFrameWorks_설계코드리뷰_2026-09-28.md` 의 R · C · D 번호. 파일:행은 2026-09-29 이름 변경 후 기준.
완료 기준: 해당 R 항목이 리뷰 재현 코드에서 `[NOT REPRODUCED]` 가 되고, 그 항목을 `gmtest` 회귀 테스트로 옮긴다.

- [ ] **1-1. 죽은 ID 가드** (C1 · D5, R1–R3) — `GameMaster/State/GameplayState.h:101` `Add<T>`, `GameplayComponentTable.h:171`, `GameplayBoardState.cpp:76` `SetPosition`, `GameplayZone.cpp:208` `MoveTo` 가 죽은 ID 를 검사하지 않아, 그 번호를 재사용한 살아 있는 엔티티의 데이터를 덮는다. 상태 계층에 `IsAlive` 가드(실패 반환), 내장 효과(`GameMaster/Rules/GameplayRuleEngine.cpp:355, 369`)는 죽은 대상이면 이벤트 없이 건너뛴다. 작음.
- [ ] **1-2. 스냅샷 스택** (C3 · D7, R16 · R10) — `GameMaster/Services/GameplaySnapshotStack.cpp:23` 의 `erase(begin)` 과 이동이 없는 `HGameplayState`(`GameplayState.h:28-29`) 때문에 명령마다 깊은 복사가 일어나고, 커밋된 고정 풀에서 약 95 명령에 크래시한다. noexcept 이동 생성 · 대입 + 링 버퍼(`HDeque` + `pop_front`). 작음. 0-7 커밋 전.
- [ ] **1-3. 월드 · 연출 수명** (C4 R11 · R15, C8 R12, C5 R13) — `Core/World.cpp:187` `flushPendingSpawn` 이 파괴 예약 액터를 거르지 않아 좀비가 남는다. `Actors/Actor.cpp:89` `GetWorldPosition` 이 `Get_C(3, n)`(4열)을 읽어 늘 0 이다 → `Get_C(n, 3)`. `Actors/GameMasterActor.cpp:364` 가 파괴 부수효과를 큐보다 먼저 적용해 사망 연출이 액터를 못 찾는다 → 스폰은 큐 시작 전, 파괴는 큐 완료 후. 작음.
- [ ] **1-4. 턴 전이 순서** (C6 · D1, R6a · R6b) — `GameMaster/Rules/GameplayPhaseMachine.cpp:25` `EndTurn` · `:97` `beginRound` 가 전이를 한 번에 끝내서, 턴 종료 · 라운드 시작 때 발동한 효과가 다음 차례 · 순서 결정 뒤에 해결된다. 전이를 내장 효과 단계로 큐에 넣는다. 중간.
- [ ] **1-5. 트리거와 선택 요청** (C2 · C9 · C10 · D2, R4 · R5 · R8) — `GameMaster/Rules/GameplayRuleEngine.cpp:210` 이 Emit 안에서 곧바로 React 를 부르고, `:298` 이 트리거의 선택 요청을 해결 중이던 효과에 붙여 선택 뒤 그 효과가 다시 실행된다. 명령 핸들러의 선택 요청은 큐 루프가 플래그를 지워 조용히 버려지고, Emit → React → Emit 재귀에는 깊이 제한이 없다. Emit 때는 매칭만 하고 React 는 현재 효과 해결 뒤에 부른다. React 에는 Enqueue 만 되는 제한 컨텍스트를 주고, 해결 중인 효과가 없을 때의 선택 요청은 오류 로그와 함께 거부(`ContextRequestChoice` `:223`), 깊이 가드 추가. 중간.
- [ ] **1-6. 불러오기 후 트리거 순서** (C7, R7) — `GameMaster/GameMaster.cpp:275` `Load` · `:296` `Replay` 가 `Finalize`(`:149`)를 부르지 않아 레지스트리 정렬이 빠진다. 호출 추가. 한 줄.
- [ ] **1-7. 읽기 전용 경계** (C11 · D4, R17) — `GameMaster/State/GameplayState.h:56, 81, 93` 의 const 함수가 쓰기 가능한 `T*` · `T&` 를 돌려준다. const 오버로드 분리. 작음.

### 옛 Phase 1 항목과 타이밍 모델 (2026-09-29 판단)

| 항목 | 판단 | 이유 |
|---|---|---|
| 옛 1-1 `getJGObject` 미등록 타입 | 보류 → Core 트랙 | 커널 상태는 값 타입이라 이 경로를 타지 않는다. 프리팹 · 에셋 쪽 문제 |
| 옛 1-2 헤드리스 러너 | 보류 | 게임 규칙을 올리기 시작할 때 만든다. 지금은 C++ 자체 테스트로 충분. 만들 때 반복마다 GC Flush 필요(리뷰 D11) |
| 옛 1-3 트리거 정렬 키 · 리뷰 D3 트리거 인스턴스 | 하지 않음 | 트리거 하나가 해당 컴포넌트를 가진 엔티티들을 도는 지금 구조로 표현된다 |
| 옛 1-4 이벤트 묶음 번호 | 보류 | 연출 병렬 재생(2-4)을 만들 때 |
| 옛 1-5 저장 버전 테스트 | 보류 | 저장 파일이 생길 때 |
| 옛 1-6 체크섬 직접 해시 | 삭제 | 측정된 병목이 아니다 |
| 타이밍 창 모델(결과 전 끼어들기 · 역순 스택) | 기반에 넣지 않음 | 게임마다 방식이 다르다. 지금 커널은 순서대로 쌓는 효과 큐와 수치 수정자로 동작하고, 끼어들기가 꼭 필요한 게임이 오면 그때 확장한다 |

## Phase 2. 월드 · 연출

- [ ] **2-1. `JGCameraComponent` · `JGCameraActor`** — 보드 궤도 · 고정 카메라. `PCamera` 빈 클래스와 관계 정리.
- [ ] **2-2. `JGStaticMeshComponent`** — Graphics 핸들 발급 API 필요(`PScene`/`PJGGraphicsAPI`). Graphics 트랙과 조율.
- [ ] **2-3. `JGGameplayDevViewComponent`** — ImGui 상태 인스펙터 · 이벤트 로그 · 되돌리기 · 체크섬. GUI 위젯 경로(`JGWidget`) 로 붙일지 결정 필요.
- [ ] **2-4. GameMasterActor 시퀀서 확장** — 인과 기반 병렬 묶음, 페이싱, 스킵 키.
- [ ] **2-5. 피킹** — `HRay` 대 보드 평면/헥스, 카드 사각형. Controller 에 연결.
- [ ] **2-6. 프리팹(액터 서브트리 에셋 + 델타)** — 지난 조사 §6-3. 다형 직렬화는 이미 동작.

## Phase 3. 게임 레이어 첫 검증

- [ ] **3-1. 게임 모듈 템플릿** — `Sim/` `View/` 폴더, StartupModule 등록, 리플렉션 자동 등록 통과 확인.
- [ ] **3-2. ImGui 수직 슬라이스** — 한 전투를 끝까지. 시퀀서 이벤트 정보량 검증.
- [ ] **3-3. 헤드리스 자동 플레이 통계**

## 백로그

- 리뷰의 나머지 항목(C12–C27, D6 · D9 · D10 · D12) — 쓸 곳이 생길 때. (사망 연출 뒤 파괴는 Phase 1-3 으로 옮김)
- `IGameplayBoard` 시야 규칙 기본 구현(격자 브레젠험)
- 멀티플레이 대비: 명령 스트림 직렬화 규약 문서화
- 툴: 리플렉션 인스펙터로 상태 편집
