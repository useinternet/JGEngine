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
- [x] **0-9. 이름 변경 Simulation → GameMaster · Gameplay** — 완료 2026-09-29 11:55. 규칙: 파사드 `PGameMaster` 와 그것을 든 월드 액터 `JGGameMasterActor` 및 시스템 단위 이름(폴더 `GameMaster/` · 정의 파일 · 자체 테스트 · 로그 카테고리)은 GameMaster, 그 밖의 타입은 Gameplay. 엔티티 액터 `JGGameplayEntityActor`, 콘솔 명령 `gmtest`(옛 이름 `simtest` 도 받음 → 2026-09-30 별칭 제거, `gmtest` 만 받는다). 타입 62 · 파일 72 이동, 리뷰 재현 코드 함께 치환. 변경 전후 자체 테스트 · 리뷰 재현 결과 줄 단위 동일, 런처 회귀 통과. 대응표 `Document/GameFrameWorks_이름변경안_2026-09-29.md`, 증거 `Document/Memory/2026-09-29_gfw_rename_verify.txt`.
- [x] **0-7. 커밋** — 사용자가 커밋함: `d457b92`(2026-09-29 14:57), 메모리 트랙 Phase 2(성장형 풀)와 함께.

## Phase 1. 커널 버그 수정 (리뷰 재현 항목)

2026-09-29 재구성. 기준은 "보드게임 · 카드게임 어느 것을 올려도 밟는 결함"이다. 특정 게임의 규칙에 맞춘 항목은 넣지 않는다.
근거와 재현 방법은 `Document/GameFrameWorks_설계코드리뷰_2026-09-28.md` 의 R · C · D 번호. 파일:행은 2026-09-29 이름 변경 후 기준.
완료 기준: 해당 R 항목이 리뷰 재현 코드에서 `[NOT REPRODUCED]` 가 되고, 그 항목을 `gmtest` 회귀 테스트로 옮긴다.

**Phase 1 완료 (2026-09-29 ~ 30).**
- 리뷰 재현 코드에서 R1–R8 · R10–R13 · R15–R17 이 `[NOT REPRODUCED]` 다. 남은 R9 · R14 는 백로그다.
- 회귀 테스트: `gmtest` GameMaster 9 · 10 · 12 절과 월드 자체 테스트.
- 리슨 서버 세션(Network Phase 0)이 1-1 · 1-6 을 같은 시기에 고쳐서 두 수정을 병합했다.
- 증거는 `Document/Memory/2026-09-29_gfw_phase1_verify.txt`.

- [x] **1-1. 죽은 ID 가드** (C1 · D5, R1–R3)
  - 상태 계층:
    - `HGameplayState::Add<T>` 는 `T*` 를 돌려주고, 죽은 ID 면 nullptr 이다.
    - 보드 · 영역 쓰기는 `HGameplayState::SetBoardPosition` · `MoveToZone` 이 죽은 ID 를 거부한다.
    - `Board` · `Zones` 를 직접 쓰는 함수는 생존을 모른다고 주석에 적었다.
  - 내장 `SetBoardPosition` · `MoveToZone` 은 이것을 써서, 죽은 대상이면 이벤트 없이 건너뛴다.
  - 회귀: 9 절.
- [x] **1-2. 스냅샷 스택** (C3 · D7, R16 · R10)
  - `HGameplayState` 에 noexcept 이동 생성 · 대입을 넣었다.
  - `PGameplaySnapshotStack` 은 `HDeque` + `pop_front` 를 쓰고, `Pop` 은 상태를 옮겨 준다. Undo 와 거부된 명령의 복원도 옮기기로 바꿨다.
  - 한도 40 · 엔티티 302 에서 한도 도달 전후 Submit 은 0.60 → 1.61 ms(×2.7)였다. 이제 0.37 → 0.36 ms(×1.0)다.
  - 회귀: 12 절 R16(한도를 넘겨 300 명령).
- [x] **1-3. 월드 · 연출 수명** (C4 R11 · R15, C8 R12, C5 R13)
  - `flushPendingSpawn` 은 파괴 예약된 액터를 월드에 넣지 않는다.
  - `GetWorldPosition` 은 `Get_C(n, 3)` 을 읽는다.
  - GameMasterActor 는 스폰 바인딩을 큐 시작 전에, 파괴 바인딩을 큐 완료 후에 적용한다(`applyBindingBeforeCue` · `applyBindingAfterCue` · `finishActiveCue`).
  - 회귀: 월드 자체 테스트 R11 · R12 · R13 · R15.
- [x] **1-4. 턴 전이 순서** (C6 · D1, R6a · R6b)
  - 전이를 `EGameplayPhaseStep` 단계로 나눴다: BeginRound · ResolveOrder · BuildOrder · NextTurn · EnterTurnMain.
  - 다음 단계는 `HGameplayTurnState::PendingStep` 에 예약하고(저장 필드 추가), 룰 엔진이 효과 큐가 빈 뒤에 실행한다.
  - 내장 `EndTurn` 효과는 TurnStart 에서도 받는다(턴 시작 효과로 턴 넘기기).
  - 회귀: R6a · R6b, 턴 넘기기, TurnEnd 도중 선택 대기의 문서 저장 · 재개.
- [x] **1-5. 트리거와 선택 요청** (C2 · C9 · C10 · D2, R4 · R5 · R8)
  - Emit 때는 매칭만 한다(`PGameplayTriggerDispatcher::Match`). 반응은 이벤트를 낸 단계가 끝난 뒤 `React` 로 부른다.
  - `JGGameplayTrigger::React` 는 `HGameplayTriggerContext`(Enqueue 만, 상태 const)를 받는다.
  - `RequestChoice` 는 bool 을 돌려준다. 효과 Resolve 안에서만 받고, 그 밖이면 오류 로그를 남기고 false 다.
  - 깊이 가드는 구조로 해결했다. React 가 Emit 을 부를 수 없으므로, 연쇄는 효과 큐의 깊이 한도(64)를 거친다.
  - 효과 총량 한도(4096)를 넘으면 남은 효과 · 반응을 버리고 페이즈 전이만 마친다.
  - 회귀: R4 · R5 · R8.
- [x] **1-6. 불러오기 후 트리거 순서** (C7, R7)
  - `ImportDocument`(Load 가 부른다)와 `Replay` 가 `Finalize` 를 부른다.
  - 회귀: 10 절.
- [x] **1-7. 읽기 전용 경계** (C11 · D4, R17)
  - `Table` · `Find` · `Get` 의 const 판은 const 를 돌려준다.
  - 회귀: 자체 테스트의 static_assert.

### 옛 Phase 1 항목과 타이밍 모델 (2026-09-29 판단)

| 항목 | 판단 | 이유 |
|---|---|---|
| 옛 1-1 `getJGObject` 미등록 타입 | 보류 → Core 트랙 | 커널 상태는 값 타입이라 이 경로를 타지 않는다. 프리팹 · 에셋 쪽 문제 |
| 옛 1-2 헤드리스 러너 | 보류 | 게임 규칙을 올리기 시작할 때 만든다. 지금은 C++ 자체 테스트로 충분. 만들 때 반복마다 GC Flush 필요(리뷰 D11). 만들 때 JGConsole 명령은 `.cpp` 하나에 `HAutoConsoleCommand`로 선언하고 인자는 `-repeat=N`(`Document/DevConsole_TODO.md` 0-3, 2026-09-29 완료) |
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

- 리뷰의 나머지 항목(C12–C27, D6 · D9 · D10 · D12) — 쓸 곳이 생길 때. 재현 코드에 아직 걸리는 것은 R9(C17, Start 와 첫 명령의 CauseSequence 가 같음) · R14(C12, 리플렉션 없는 큐 파생이 기본 큐로 재생). (사망 연출 뒤 파괴는 Phase 1-3 에서 처리)
- `IGameplayBoard` 시야 규칙 기본 구현(격자 브레젠험)
- 멀티플레이 대비: 명령 스트림 직렬화 규약 문서화
- 툴: 리플렉션 인스펙터로 상태 편집
