# GameFrameWorks 설계 · 코드 리뷰

> **이름 안내 (2026-09-29, 구현 세션)**: 이 보고서의 `Simulation` 이름은 이후 GameMaster · Gameplay 로 바뀌었다(대응표 `GameFrameWorks_이름변경안_2026-09-29.md`). 발견 내용은 그대로 유효하다. 재현 코드 `ReviewRepro.cpp` 도 새 이름으로 치환했고 결과는 변경 전과 같다(`Memory/2026-09-29_gfw_rename_verify.txt`). 콘솔 명령 `simtest` 는 `gmtest` 가 되었고 옛 이름도 받는다.

작성 2026-09-28. 대상: `Source/Runtime/GameFrameWorks/` 90개 파일(20:32 기준 메인 트리와 동일한 스냅샷) + `Source/Programs/JGConsole/Main.cpp`.
근거 문서: `게임프레임워크_설계방안_요약`, `설계방안B_상세설계`, `설계방안B_인터페이스_구현가이드`, `GameFrameWorks_구현현황`, `GameFrameWorks_TODO` (모두 2026-09-28).
경로는 따로 적지 않으면 `Source/Runtime/GameFrameWorks/` 기준이다. 소스는 고치지 않았다.
구현 세션에 `Simulation` 접두 이름 일괄 변경이 미결로 걸려 있다(`Document/Memory/2026-09-28_턴제게임_프레임워크_방향성.md` 18:00 항목). 이름이 바뀌어도 발견 내용은 그대로 적용되고, 줄 번호만 다시 찾으면 된다.

---

## 0. 결론

- **방향은 맞다.** 방안 B(값 타입 시뮬레이션 + 이벤트 · 명령으로만 연결되는 월드 계층)가 코드에 그대로 옮겨졌고, 결정론 기본기(PCG32 용도별 스트림 · 정수 · 순서 있는 컨테이너 · 상태 안의 선택 대기)가 좋다.
- **실행 모델에 설계 결함 둘**이 있다. (1) 페이즈 전이가 효과 큐 밖에서 동기로 끝난다. (2) 트리거가 `Emit` 안에서 즉시 반응하고, 반응 코드가 효과와 같은 권한을 가진다. 재현한 규칙 버그(R4 · R5 · R6 · R8)의 뿌리가 이 둘이다.
- **버그 17건을 실제로 재현했다**(아래 §1). 커밋 전에 막을 것: 죽은 엔티티 ID 가 살아 있는 엔티티 데이터를 덮어씀(R1–R3), 스냅샷 스택 — **HEAD Core 위에서는 약 95번째 명령에서 크래시**(R16. 메모리 트랙 Phase 2 와 함께 커밋해야 함), 월드 좀비 액터(R11 · R15), `GetWorldPosition` 오류(R12).

---

## 1. 검증 방법과 결과

메인 트리에서 다른 세션이 작업 중이라 **격리 워크트리**(`git worktree add --detach`, HEAD `fcace8f` Core + 현재 GameFrameWorks 스냅샷)에서 PreBuild 2회 → `GameFrameWorks.vcxproj` · `JGConsole.vcxproj` 빌드(오류 0) → 실행했다.

- 기준선: `JGConsole.exe simtest` → 시뮬레이션 60/60 · 월드 15/15 통과, 종료 코드 0.
- 재현: JGConsole 에 재현 코드(`Document/Memory/tools/gfw_review_repro/ReviewRepro.cpp`)를 넣어 `simreview` · `simreviewperf` · `simreview256` 실행. 결과 원문은 `Document/Memory/2026-09-28_gfw_review_repro_results.txt`.

| ID | 재현한 증상 | 결과 |
|---|---|---|
| R1 | B 파괴 → 새 엔티티가 B 의 인덱스 재사용 → 늦게 온 `SetBoardPosition(B)` 가 새 엔티티의 좌표 기록을 지우고 (3,3) 에 죽은 B 를 유령 점유자로 남김 | 재현 |
| R2 | 같은 상황에서 `Add<T>(B)` 가 새 엔티티의 컴포넌트를 덮어써 소실(값 50 → 없음), `Each<T>` 에 죽은 ID 가 나옴 | 재현 |
| R3 | `MoveToZone(B)` 가 죽은 B 를 묘지 영역에 넣고, EntityDestroyed 뒤에 ZoneChanged 를 냄 | 재현 |
| R4 | 트리거 `React` 의 `RequestChoice` 가 해결 중이던 **피해 효과**의 선택으로 저장 → 선택 후 피해 효과가 다시 실행(10 → 7 → 4) | 재현 |
| R5 | 핸들러 `Execute` 의 `RequestChoice` 가 조용히 버려짐(결과 Executed, 대기 없음, 로그 없음) | 재현 |
| R6a | `TurnEnded` 트리거 효과가 다음 행동자의 `TurnMain` 에서 해결(CurrentActor = 다음 행동자, phase = TurnMain) | 재현 |
| R6b | `RoundStarted` 트리거가 바꾼 이니셔티브가 이번 라운드 순서에 반영 안 됨(순서가 효과 해결 전에 만들어짐) | 재현 |
| R7 | Start 없이 `Load` 후 같은 명령 → 같은 우선순위 트리거 순서가 원래와 달라 결과 12 vs 21, 체크섬 불일치 | 재현 |
| R8 | `React` 안의 직접 `SpawnEntity` 연쇄가 깊이 80 까지 재귀, `MaxEffectDepth`(64) · `EffectDepthExceeded` 없음 | 재현 |
| R9 | Start 이벤트와 첫 명령 이벤트의 `CauseSequence` 가 둘 다 0 | 재현 |
| R10 | 스냅샷 한도 40 · 엔티티 302 에서 Submit 평균 0.86ms → 한도 도달 후 2.1ms(×2.4). 한도에 비례해 선형 증가 | 측정 |
| R11 | 월드 틱 중 스폰 + 같은 틱 파괴 → 다음 틱에 월드에 들어가 BeginPlay 되고 영구 잔류(`IsPendingDestroy`, `GetWorld()` null) | 재현 |
| R12 | 부모 (5,0,0) · 자식 로컬 (1,2,3) → `GetWorldPosition` = (0,0,0). 행렬 3행은 (6,2,3) | 재현 |
| R13 | Director 가 EntityDestroyed 큐를 부르는 시점에 액터가 이미 해제됨 → 사망 연출이 액터를 못 찾음 | 재현 |
| R14 | `RegisterCue` 로 넣은 비 리플렉션 큐 파생이 기본 `JGSimulationCue` 로 재생(파생 `Begin` 0회) | 재현 |
| R15 | Director 가 한 프레임에 EntitySpawned · EntityDestroyed 처리 → R11 좀비(살아 있는 엔티티 2, 액터 3) | 재현 |
| R16 | 기본 한도(256) · 엔티티 2개로 EndTurn 반복 → 75~100 명령 사이 크래시. 스택: `SnapshotStack::Push` → 벡터 재할당이 전 스냅샷 깊은 복사 → 16B 클래스(디버그 컨테이너 프록시) 고갈 → null 쓰기 | 재현(HEAD Core) |
| R17 | `const HSimulationState&` 로 받은 상태를 `Get/Find` 로 수정, 명령 없이 체크섬 변경 | 재현 |

메인 트리의 성장형 풀(메모리 트랙 Phase 2, 미커밋)에서는 R16 크래시가 나지 않을 것이다. 확인은 하지 않았다. R10 의 256 한도 수치는 HEAD 풀이 버티지 못해 재지 못했다.

---

## 2. 설계 리뷰

### 2-1. 잘 된 점

- 계층 경계가 문서대로다. `Simulation/` 은 Core 헤더만 포함하고, `PWorld` 는 시뮬레이션을 모르며, Director 만 둘을 잇는다.
- 결정론 기본기: 값 타입 상태 · 세대 ID · 인덱스 오름차순 순회 · 상태 안에 해시맵 없음 · PCG32(레퍼런스와 같은 구현) 용도별 8 스트림 · 레지스트리 이름순 정렬.
- 선택 대기의 남은 큐를 상태(`SavedQueue`)에 보존해서 대기 중 저장 · 로드 · 되돌리기가 된다(테스트로 확인).
- 입구가 `Submit` 하나이고 `Execute` 가 `Validate` 를 다시 부르므로 Replay · Simulate 도 검증을 거친다.
- 엔진 제약을 알고 피했다: Director 관찰자 분리(IMemoryObject 뿌리 둘 회피), `RawDynamicCast`, 구조체 수동 JSON. 코딩 규칙(한 줄 if 없음)도 지켰다.
- 헤드리스 자체 테스트 75개가 JGConsole 에서 돈다.

### 2-2. 설계 문제

**D1. 페이즈 전이가 효과 큐 밖에서 동기로 끝난다** — R6a · R6b
- `EndTurn` 은 TurnEnded 발행 → 다음 행동자 선택 → TurnStarted → TurnMain 을 한 번에 진행한다(`Simulation/Rules/SimulationPhaseMachine.cpp:37-44`). 트리거가 넣은 효과는 그 뒤 `runQueue` 에서 해결된다. 라운드 시작도 RoundStarted 발행 직후 `buildOrder` 를 부른다(`:109-112`).
- 결과: 턴 종료 효과가 다음 사람 차례에 해결된다. 턴 종료 효과의 선택 요청은 다음 사람 차례에 뜬다. 다음 행동자가 턴 종료 효과로 죽어도 그 턴은 TurnMain 에 머문다. RoundStart 와 OrderResolve 를 나눈 의미가 없다.
- 제안: 페이즈 전이를 **내장 효과 단계**로 큐에 넣는다(`EndTurn` → [TurnEnd 단계] → 발행 → 트리거 효과들 → [다음 턴 단계] …). 내장 `EndTurn` 효과가 이미 있고 큐는 상태에 보존되므로 선택 대기 · 저장 · 리플레이가 그대로 된다. 그러면 "RoundStart 에서 입력을 기다리는 계획 단계"(글룸헤이븐의 동시 카드 선택)도 선택 대기로 표현된다 — 지금 구조로는 불가능하다.

**D2. 트리거가 Emit 안에서 즉시 반응하고, 반응 코드가 효과와 같은 권한을 가진다** — R4 · R5 · R8
- `ContextEmit` 이 곧바로 `Dispatch` → `React` 를 부른다(`Simulation/Rules/SimulationRuleEngine.cpp:199-210`). `React` 는 효과와 같은 `HSimulationContext` 를 받아 Emit · Spawn · Destroy · RequestChoice · FinishGame 을 다 쓸 수 있다.
- 결과: 트리거의 선택 요청이 해결 중이던 효과의 선택으로 저장된다(R4, `runQueue` 가 `Waiting = request` 로 저장, `:290-298`). 핸들러의 선택 요청은 버려진다(R5). React 안의 직접 Emit 연쇄는 깊이 상한을 거치지 않는다(R8, 스택 오버플로 가능). 효과가 `Get<T>()` 참조를 쥔 채 Emit 하면 동기 트리거가 테이블을 키워 참조가 무효화될 수 있다 — 구현 가이드 §4-3 예제가 정확히 이 패턴이다.
- 제안: Emit 때는 **매칭만** 하고 `React` 는 현재 효과의 `Resolve` 가 끝난 뒤 큐 루프에서 부른다. React 에는 Enqueue 만 되는 제한 컨텍스트를 준다. `RequestChoice` 는 엔진이 "지금 해결 중인 효과" 를 알 때만 받고, 아니면 오류 로그 후 거부한다. `ContextEmit` 에 깊이 가드를 둔다.

**D3. 트리거 · 수정자가 클래스당 인스턴스 하나다** — 상세설계 §3-4 정렬 규칙과 충돌
- 디스패처 정렬 키는 우선순위 → 레지스트리 순서(종류 이름)뿐이다(`Simulation/Rules/SimulationTriggerDispatcher.cpp:25`). 트리거에 소유자 · 부여 시각이 없으니 문서의 "행동자 소유 먼저 → 부여 시각" 은 구현할 수 없다. TODO 1-3 은 지금 구조로는 할 수 없다.
- 같은 유물 3개, 상태 이상 스택처럼 인스턴스마다 따로 반응해야 하는 규칙은 트리거 하나가 안에서 엔티티를 돌며 처리해야 하고, 순서는 엔티티 인덱스 순으로 고정된다.
- 제안: 트리거 인스턴스를 **상태 데이터**로 둔다(`{ Kind, Owner, GrantSequence, Params }` 목록). 클래스는 행동만 맡는다. 정렬 = 우선순위 → 소유자(현재 행동자 먼저) → GrantSequence. 스냅샷 · 저장에도 자연히 들어간다.

**D4. 읽기 전용 경계를 컴파일러가 지키지 않는다** — R17
- `Get/Find/Table` 이 const 함수인데 비 const `T&` · `T*` 를 돌려준다(`Simulation/State/SimulationState.h:56, 81, 93`). 프레젠테이션 · `Validate` · `Matches` · `Evaluate` 가 받는 const 상태는 사실상 쓰기 가능하다.
- `HSimulationContext::State` 가 공개 참조라 `ctx.State.DestroyEntity()` 처럼 이벤트 없는 변경 길이 열려 있다. 수정자는 const 컨텍스트를 받지만 `Rng()` 가 const 라 난수를 소비할 수 있다.
- `Validate` · `Enumerate` 는 상태만 받아 수치 파이프라인을 쓸 수 없다. "비용 1 감소" 같은 수정자를 검증 · UI 미리보기에 반영할 길이 없다.
- 제안: const 오버로드 분리, 읽기 전용 계산 API(`Compute(const State&, query)`, 난수 금지), React · Apply 용 제한 컨텍스트.

**D5. 엔티티 수명 불변식을 상태가 지키지 않는다** — R1–R3
- `Add<T>`(`SimulationState.h:101` → `SimulationComponentTable.h:183`), `Board.SetPosition`(`SimulationBoardState.cpp:78, 104`), `Zones.MoveTo`(`SimulationZone.cpp:208`) 가 죽은 ID 를 검사하지 않는다. 내장 효과 `SetBoardPosition` · `MoveToZone` 도 대상 생존을 보지 않는다(`SimulationRuleEngine.cpp:355-385`).
- "죽인 뒤 넉백" 처럼 전술 게임에서 흔한 효과 순서에서 발생하고, 인덱스가 LIFO 로 즉시 재사용되므로 **살아 있는 엔티티의 데이터가 망가진다.**
- 제안: 상태 계층에서 `IsAlive` 가드(실패 반환), 내장 효과는 죽은 대상이면 건너뛰고 이벤트를 내지 않는다. 영역 소속을 배타(엔티티당 한 영역)로 할지 정해서 강제한다(지금은 `MoveTo` 만 배타, `PushBack` 은 중복 허용).

**D6. 룰 엔진의 실행 상태가 멤버다**
- `_queue` · `_outEvents` · `_bChoiceRequested` · `_requestedChoice` · `_resolvedCount` 가 `PSimulationRuleEngine` 멤버다(`Simulation/Rules/SimulationRuleEngine.h:29-37`). 그래서 `Execute` · `Simulate` 가 const 일 수 없고, 에이전트가 비 const `PSimulation&` 를 받아 Submit · Undo 까지 할 수 있다. 탐색을 워커 스레드에서 돌리거나 실행 중 재진입하면 깨진다.
- 제안: 호출마다 만드는 실행 구조체(`HSimulationExecution`)로 옮기고 컨텍스트가 그것을 가리키게 한다. `Simulate(const State&) const` · 병렬 탐색 · 재진입이 가능해진다.

**D7. 상태 복사 비용 구조** — R10 · R16
- `HSimulationState` 는 복사만 있고 이동이 없다(`SimulationState.h:28-29`). 스냅샷 스택은 `HList` + `erase(begin)` 이다(`Simulation/Services/SimulationSnapshotStack.cpp:23`).
- 한도에 닿으면 명령마다 (한도 − 1)개 상태를 복사 대입하고, 벡터가 커질 때마다 전 스냅샷을 깊은 복사한다(재할당 중 두 벌). HEAD 의 고정 풀에서는 이것만으로 약 95 명령에 크래시한다(R16).
- **커밋 순서: GameFrameWorks 는 메모리 트랙 Phase 2(성장형 풀)와 함께 또는 그 뒤에 커밋해야 한다.** TODO 0-7 은 둘을 따로 커밋하라고 적혀 있다.
- 제안: noexcept 이동 생성 · 대입, 스냅샷은 링 버퍼(`HDeque` + `pop_front`). 필요해지면 테이블 단위 공유(COW). 체크섬은 JSON 직렬화 대신 필드 해시(TODO 1-6).

**D8. 연출 계층의 타이밍 모델** — R11 · R13 · R14 · R15
- Director 는 이벤트를 꺼내면 스폰 · 파괴 부수효과를 먼저 적용하고 그다음 큐를 시작한다(`Actors/SimulationDirectorActor.cpp:364` → `:377`). **스폰은 큐 전에, 파괴는 큐가 끝난 뒤**여야 사망 연출이 된다. 백로그 "사망 연출 뒤로 미루는 옵션" 이 아니라 기본 동작이어야 한다.
- `PWorld::flushPendingSpawn` 이 파괴 예약된 액터를 거르지 않는다(`Core/World.cpp:187-199`). Director 는 큐 없는 이벤트를 한 프레임에 몰아 처리하므로 "생성 즉시 소멸하는 토큰" 에서 바로 좀비가 생긴다(R15).
- 액터 · UI 가 `GetState()` 를 읽으면 연출보다 앞선 최종 상태를 보여 준다(피격 연출 전에 HP 바가 줄어든다). "표시 값은 큐가 이벤트의 Before/After 로 갱신하는 뷰 모델에 둔다" 를 규칙으로 문서화할 것.
- 큐 인스턴스를 `GetClass()` 로 만들어서(`Actors/SimulationCue.cpp:28`) JGCLASS 가 아닌 파생은 기본 큐로 재생된다(R14). `RegisterCue<T>()` 템플릿(`SimulationDirectorActor.h:63`)이 있어 오해하기 쉽다 → 팩토리 저장이나 가상 Clone.

**D9. 등록 범위 · 이름 공간**
- `RegisterAllFromReflection<T>` 는 프로세스에 로드된 **모든** 파생 클래스를 등록한다. 게임 모듈이 둘이거나 한 게임에 시뮬레이션이 둘(전투 · 맵)이면 규칙이 섞인다. 큐도 같다.
- `Register` 는 NAME_NONE 종류를 중복 검사 없이 받는다(`Simulation/Rules/SimulationRegistry.h:21`). 내장 효과와 이름이 같은 게임 효과는 `resolveBuiltin` 이 먼저 잡아 조용히 가려진다.
- 큐 등록 순서는 `JGClass::ChildTypeSet`(HHashSet) 순회 순서라 클래스가 늘면 바뀐다. "먼저 등록된 큐가 먼저" 규칙이 의미가 없다.
- 제안: 등록 필터(메타 태그 · 모듈 이름), NAME_NONE · 내장 이름 거부(오류 로그), 큐 프로토타입도 정렬.

**D10. 저장 · 리플레이 호환성** — R7
- `Load` 가 `Finalize` 를 부르지 않는다(`Simulation/Simulation.cpp:275-290`). Start 없이 이어 하기를 하면 트리거 순서가 원래 세션과 달라진다.
- 저장 헤더에 엔진 `SchemaVersion` 만 있다(`Simulation/Services/SimulationSerializer.cpp:31`). 게임 컴포넌트 구조가 바뀌어도 버전이 안 바뀌어 마이그레이터가 불리지 않는다. 등록된 규칙 목록(지문)도 없어 다른 빌드의 리플레이가 조용히 어긋난다.
- 제안: Load · Replay 에서 Finalize(또는 등록할 때 정렬 삽입), 헤더에 `{ EngineSchema, GameSchema, RulesFingerprint }`.

**D11. 수명 · 모듈 · Core 의존**
- 모듈 틱을 `CreateRaw(this)` 로 스케줄하는데 스케줄러에 해제 API 가 없다(`Core/GameFrameWorksModule.cpp:20`). 실행 중 `DisconnectModule` 하면 해제된 모듈을 부른다. GC 티커 객체를 `CreateSP` 로 걸고 ShutdownModule 에서 Reset 하면 GFW 안에서 해결된다(SP 델리게이트는 대상이 죽으면 아무것도 하지 않는다).
- 역참조(액터의 부모 · 월드, 컴포넌트의 소유자, 시뮬레이션의 관찰자, Director 바인딩 표)가 전부 `PWeakPtr` 다. Core 약참조는 대상이 GC 되면 해제된 카운터를 읽는다(Memory_TODO 4-1 미해결). 방향은 맞지만 4-1 전까지는 대상보다 오래 사는 약참조를 남기지 않도록 끊어 줘야 한다(예: `~PWorld` 는 `_actors` 의 월드 참조만 끊고 `_pendingSpawn` 은 둔다).
- JGConsole 은 `GCoreSystem::Update` 를 부르지 않아 헤드리스 실행 중 GC 가 한 번도 돌지 않는다. 계획된 헤드리스 러너(`-repeat N`, TODO 1-2)는 반복마다 `GMemoryGlobalSystem::Flush()` 가 필요하다.

**D12. 장르 적합성 · 문서 정합성**
- 숨은 정보 모델이 없다(상태 전체가 공개) — 멀티플레이 · AI 공정성에 필요해진다.
- 육각 보드가 축 좌표 + 사각 범위라 실제 모양이 마름모다(`Simulation/State/SimulationBoardState.cpp:42`). 오프셋 좌표 변환이나 셀 마스크가 필요하다. 설계 문서의 탐색 에이전트는 없고 1수 탐욕만 있다.
- 상세설계 · 인터페이스 가이드는 아직 `Tabletop` 이름 · `Presentation/` 폴더 · `JGSTRUCT` 리플렉션 직렬화 · `Validate(ctx, …)` 시그니처로 적혀 있다. 구현(`Simulation` 접두 · 4폴더 · 수동 JSON · `Validate(state, …)` · Cue/Director)과 맞춰야 게임 작성자가 따라 할 수 있다.

---

## 3. 코드 리뷰 — 발견 목록

심각도: **높음** = 데이터 손상 · 크래시 · 규칙 오작동이 흔한 시나리오에서 발생. **중간** = 특정 사용에서 발생하거나 API 가 틀림. **낮음** = 성능 · 정리.

| # | 심각도 | 위치 | 문제 | 근거 | 수정 방향 |
|---|---|---|---|---|---|
| C1 | 높음 | `SimulationState.h:101` · `SimulationBoardState.cpp:78` · `SimulationZone.cpp:208` · `SimulationRuleEngine.cpp:355-385` | 죽은 ID 로 컴포넌트 · 좌표 · 영역을 쓰면 살아 있는 엔티티 데이터를 덮거나 유령을 남김 | R1–R3 | D5 |
| C2 | 높음 | `SimulationRuleEngine.cpp:290-298` | 트리거의 선택 요청이 해결 중 효과에 붙어 선택 후 효과 재실행 | R4 | D2 |
| C3 | 높음 | `SimulationSnapshotStack.cpp:23` · `SimulationState.h:28-29` | 이동 없는 상태 벡터: 재할당 · `erase(begin)` 마다 깊은 복사, HEAD 풀에서 약 95 명령 크래시 | R10 · R16 | D7 |
| C4 | 높음 | `Core/World.cpp:187-199` | 틱 중 스폰 + 파괴 예약 액터가 다음 틱에 월드에 들어가 BeginPlay 후 영구 잔류 | R11 · R15 | `_bPendingDestroy` 면 건너뛰고 `_world` 정리 |
| C5 | 높음 | `Actors/SimulationDirectorActor.cpp:364, 377` | 파괴 부수효과가 사망 큐보다 먼저 → 연출이 액터를 못 찾음 | R13 | 스폰은 Begin 전, 파괴는 큐 완료 후 |
| C6 | 높음 | `SimulationPhaseMachine.cpp:37-44, 109-112` | 턴 종료 효과가 다음 턴에, 라운드 시작 효과가 순서 결정 뒤에 해결 | R6a · R6b | D1 |
| C7 | 높음 | `Simulation.cpp:275-290` | `Load` 가 `Finalize` 를 안 불러 트리거 순서가 원래 세션과 다름 | R7 | Load · Replay 에서 Finalize |
| C8 | 중간 | `Actors/Actor.cpp:89` | `Get_C(3, n)` 은 4열(항상 0). 이동 성분은 `Get_C(n, 3)` | R12 | 인덱스 교정 |
| C9 | 중간 | `SimulationRuleEngine.cpp:290` | 핸들러(효과 밖)의 `RequestChoice` 가 경고 없이 버려짐 | R5 | 해결 중 효과가 없으면 오류 로그 · 거부 |
| C10 | 중간 | `SimulationRuleEngine.cpp:199-210` | Emit → React → Emit 재귀에 깊이 가드 없음 | R8 | D2, 깊이 초과 시 디스패치 중단 |
| C11 | 중간 | `SimulationState.h:56, 81, 93` | const 상태로 쓰기 가능 | R17 | const 오버로드 분리 |
| C12 | 중간 | `Actors/SimulationCue.cpp:28` · `SimulationDirectorActor.h:63` | 비 리플렉션 큐가 기본 큐로 재생 | R14 | 팩토리 저장 또는 Clone |
| C13 | 중간 | `SimulationZone.cpp:175` · `SimulationState.cpp:90` · `SimulationRuleEngine.cpp:235` | `Zone(name)` 이 없는 이름을 자동 생성(오타 = 새 영역), `push_back` 으로 앞서 받은 `HSimulationZone&` 무효화 | 코드 | Start 후 미등록 이름은 오류, 포인터 반환 |
| C14 | 중간 | `Simulation.cpp:403-411, 420` | 관찰자 콜백 안에서 Add/RemoveObserver · Submit 하면 순회 중인 반복자 무효화 | 코드 | 사본을 순회 |
| C15 | 중간 | `SimulationDirectorActor.cpp:211-224` | 바인딩 표를 순회하며 `Destroy` → 틱 밖이면 즉시 EndPlay → 게임 코드가 `UnbindActor` 하면 순회 중 삭제. 상태 교체 후에도 `_bufferedCommands` 가 남아 옛 상태 기준 명령이 실행됨 | 코드 | 표를 옮겨 놓고 순회, 교체 시 버퍼 비움 |
| C16 | 중간 | `Core/GameFrameWorksModule.cpp:20` | 해제 불가 raw 델리게이트 | 코드 | D11 |
| C17 | 중간 | `SimulationRuleEngine.cpp:19, 101, 136` · `SimulationSelfTest.cpp:431` | Start 와 첫 명령의 CauseSequence 가 같음(테스트가 이 값을 고정) | R9 | Start 를 별도 번호로 |
| C18 | 낮음 | `SimulationDirectorActor.cpp:305` | 엔티티마다 `"Entity_E<i>.<g>"` PName 생성 → 문자열 테이블 영구 인터닝, 세대마다 새 이름 | 코드 | 이름 없이 스폰하거나 디버그 전용 |
| C19 | 낮음 | `SimulationRegistry.h:21, 39` · `SimulationRuleEngine.cpp:343-387` | NAME_NONE 수용. `Find` 가 항목마다 가상 `GetKind()` 로 PName 을 만들고, 내장 효과 판정이 효과마다 PName 5개를 만든다(문자열 테이블 뮤텍스) | 코드 | 등록 시 종류 캐시, 내장 이름을 엔진 멤버로 |
| C20 | 낮음 | `Simulation.cpp:192` · `SimulationRuleEngine.cpp:93` | Submit 이 검증을 두 번 | 코드 | 한쪽 제거 |
| C21 | 낮음 | `SimulationValuePipeline.cpp:63` | 정의되지 않은 단계 이름의 수정자는 조용히 무시(오타 검출 불가) | 코드 | Finalize 에서 경고 |
| C22 | 낮음 | `SimulationRandomStream.cpp:42-43` | `-(int32)bound` 는 bound ≥ 2³¹ 에서 부호 오버플로(UB). 주석의 "Lemire" 는 실제로는 PCG threshold 방식 | 코드 | `(0u - bound) % bound` |
| C23 | 낮음 | `Simulation/Boards/SimulationBoard.cpp:67-80` | Free 보드 BFS: 방문 검사 선형, 경계 없음 → 도달 불가 목표 + 큰 maxSteps 에서 폭주 | 코드 | 방문 집합 · 탐색 상한 |
| C24 | 낮음 | `SimulationRuleEngine.cpp:267-288` | 효과 개수 상한 초과에 `EffectDepthExceeded` 재사용. 깊이 초과는 그 효과만 건너뛰고 계속(문서는 "중단") | 코드 | 이벤트 분리, 정책 문서와 맞춤 |
| C25 | 낮음 | `Simulation.cpp:142, 264` | Start 후 `EditInitialState` · `Restore` 가 "초기 상태 + 로그 = 현재" 불변식을 깸 | 코드 | Start 후 금지 또는 로그 정리 |
| C26 | 낮음 | `SimulationSelfTest.cpp:488, 539` | 테스트 저장 파일이 `Bin/DevelopEngine/` 에 남음(git 미추적 파일 2개) | 코드 | 임시 폴더 · 테스트 후 삭제 |
| C27 | 낮음 | `SimulationRuleEngine.cpp:396` | `isBuiltinCommand` 미사용 | 코드 | 제거 |

테스트 공백: 다중 트리거 순서 · `EnqueueFront` · 깊이/개수 상한 · 라운드 경계 · 죽은 ID · 선택 요청 위치 · Load 후 진행 · 스냅샷 한도 · **골든 체크섬(빌드 간 결정론)** · 월드 좀비 · 사망 연출. 런처 회귀는 GameFrameWorks 를 연결하지 않으므로 이 모듈을 검증하지 않는다.

---

## 4. 수정 순서 제안

| 시점 | 항목 |
|---|---|
| 커밋(TODO 0-7) 전 | C3(이동 + 링 버퍼. 또는 메모리 Phase 2 와 함께 커밋), C1, C4, C8 — 모두 작은 수정 |
| 게임 레이어 착수 전 | D1 페이즈 큐화(C6), D2 지연 반응 · 선택 요청 가드 · Emit 깊이 가드(C2 · C9 · C10), C7, C5, C11 |
| TODO Phase 1–2 중 | D3 트리거 인스턴스(TODO 1-3 대체), D6 실행 상태 분리, D9 등록 필터, D10 저장 헤더, C12–C17 |
| 여유 있을 때 | C18–C27, D12 문서 정합성 |

재현 코드의 R 항목을 `simtest` 회귀 테스트로 옮기고, 각 항목이 "재현 안 됨" 으로 바뀌는 것을 수정 완료 기준으로 삼기를 권한다.

---

## 5. 재현 코드 다시 돌리기

1. `Document/Memory/tools/gfw_review_repro/ReviewRepro.cpp` 를 `Source/Programs/JGConsole/` 에 복사한다.
2. `Main.cpp` 에 선언 3개와 분기 3개를 넣는다.
   ```cpp
   int32 GameFrameWorksReviewRepro();          // simreview      : R1–R9 · R11–R15 · R17
   int32 GameFrameWorksReviewSnapshotCost();   // simreviewperf  : R10 (새 프로세스에서 따로)
   int32 GameFrameWorksReviewSnapshotLimit();  // simreview256   : R16 (HEAD Core 에서는 크래시)
   // main 에서: GModuleGlobalSystem::GetInstance().ConnectModule("GameFrameWorks") 후 호출
   ```
3. PreBuild(HeaderTool → BuildTool) 후 JGConsole 빌드, `Bin/DevelopEngine` 에서 `JGConsole.exe simreview`.
4. 줄마다 `[REPRODUCED]` / `[NOT REPRODUCED]` 가 찍힌다. 수정 후에는 전부 `[NOT REPRODUCED]` 여야 한다.

메인 트리에서 다른 세션이 작업 중이면 격리 워크트리에서 돌린다(절차는 `Document/Memory/2026-09-28_GameFrameWorks_리뷰.md`). 이번 검증에 쓴 워크트리는 작업 후 지웠다.
