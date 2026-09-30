# GameFrameWorks 구현 현황

작성 2026-09-28, **이름 변경 2026-09-29 반영**. 설계 방안 B(규칙 계층 / 월드 계층 2계층)를 `GameFrameWorks` 모듈 안에 구현한 1차분. 규칙 계층의 시스템 이름은 **GameMaster**(파사드 `PGameMaster` 와 그것을 든 월드 액터 `JGGameMasterActor`, 그 밖의 타입은 `Gameplay` 접두)다. 옛 이름과의 대응표는 `GameFrameWorks_이름변경안_2026-09-29.md`. 설계 근거는 `게임프레임워크_설계방안_요약_2026-09-28.md`, `설계방안B_상세설계_2026-09-28.md`(이름 · 폴더는 이 문서가 최신).
**Phase 1 커널 버그 수정 (2026-09-30 00:15)**: 리뷰 재현 R1–R8 · R10–R13 · R15–R17 을 고쳤다(남은 R9 · R14 는 백로그). 실행 모델이 바뀐 곳은 §2 의 명령 경로 · 선택 대기 · 트리거 · 페이즈 · 상태 쓰기 · 되돌리기 · GameMasterActor · 월드 스폰 행이다. 리슨 서버 세션의 Network 변경과 병합한 메인 트리에서 빌드 오류 0, `gmtest` GameMaster 92/92 · 월드 19/19, `net.test all` 66/66, 2 프로세스 OK, 런처 30초 종료 코드 0 · 로그 오류 0. 증거 `Document/Memory/2026-09-29_gfw_phase1_verify.txt`.
**이름 변경 후 재검증 (2026-09-29 11:55)**: 빌드 오류 0, `JGConsole.exe gmtest` 75/75 · 종료 코드 0 · 로그 오류 0, `JGLauncher` 30초 종료 코드 0 · 로그 오류 0 · 리플렉션 클래스 13개 전부 새 이름. 변경 전후 자체 테스트와 리뷰 재현 결과가 줄 단위로 동일(`Document/Memory/2026-09-29_gfw_rename_verify.txt`).
**검증 완료 (2026-09-28 17:40, GameInstance 추가 후 18:00 재검증)**: 실제 작업 트리에서 `GameFrameWorks.vcxproj`(의존 프로젝트 포함) · `JGConsole.vcxproj` 오류 0, **`JGConsole.exe gmtest` GameMaster 60/60 + 월드 15/15 통과 · 종료 코드 0 · 로그 오류 0**, `JGLauncher` 30초 실행 · WM_CLOSE 종료 코드 0 · 로그 오류 0(캡처 `Document/Memory/2026-09-28_gfw_launcher_capture.png`). 증거 로그 `Document/Memory/build_2026-09-28_gameframeworks.log`. 병렬 세션의 Core 편집(메모리 풀 재설계, 미커밋)이 포함된 상태에서의 결과다. 그 전에 커밋 HEAD(`fcace8f`) 격리 워크트리에서도 같은 결과를 확인했다.

---

## 1. 폴더와 파일

```
Source/Runtime/GameFrameWorks/              SharedLib (StaticLib 에서 전환) · GAMEFRAMEWORKS_API · 코드젠 대상
├─ Core/
│   GameFrameWorksDefines.h                 API 매크로
│   GameFrameWorksModule.h/.cpp             HGameFrameWorksModule. 생명주기만: 게임 인스턴스 생성 · 소유 · 프레임 틱 · 종료 정리. ReplaceGameInstance<T>()
│   GameInstance.h/.cpp                     JGGameInstance (JGCLASS). 월드 API 제공자: LoadWorld · UnloadWorld · GetWorld · SetEntryClass<T> · OnWorldLoaded/Unloading. 게임이 파생해 프로세스 수명 상태를 둔다
│   Transform.h/.cpp                        HTransform (위치 · 회전 · 스케일, IJsonable)
│   World.h/.cpp                            PWorld. 액터 소유 · 스폰 · 파괴 · BeginPlay/Tick/EndPlay. GameMaster 를 모름. 게임은 직접 만들지 않는다
│   WorldSelfTest.h/.cpp                    월드 · 게임 인스턴스 · GameMasterActor 바인딩 헤드리스 검증
├─ GameMaster/                              엔진 Core 헤더만 포함 (헤드리스 조건)
│   GameMasterDefines.h                     열거형 · 내장 명령/효과/이벤트 이름 · 한도
│   GameMaster.h/.cpp                       PGameMaster 파사드
│   GameMasterSelfTest.h/.cpp               자체 검증 (JGConsole gmtest)
│   State/      GameplayEntityId · EntityRegistry · ComponentTable · Zone · TurnState · BoardState · RandomStream · Choice · State
│   Messages/   GameplayCommand · GameplayEffectRequest · GameplayEvent
│   Rules/      GameplayContext · CommandHandler · Effect · Trigger · Modifier (JGCLASS 4종)
│               OrderPolicy · Registry · EffectQueue · TriggerDispatcher · ValuePipeline · PhaseMachine · RuleEngine
│   Boards/     GameplayBoard (IGameplayBoard + None · Square · Hex · Free)
│   Services/   SnapshotStack · CommandLog · Serializer · Observer(+Migrator)
│   Agents/     GameplayAgent (IGameplayAgent · IGameplayEvaluator · Random · Greedy · AgentRunner)
├─ Components/
│   ActorComponent.h/.cpp                   JGActorComponent 기본형
│   GameplayEntityComponent.h/.cpp          엔티티 ↔ 액터 바인딩의 액터 쪽 (엔티티 ID 를 든다)
└─ Actors/
    Actor.h/.cpp                            JGActor. 트랜스폼 · 부모자식 · 컴포넌트 목록 (JGPROPERTY)
    GameEntryActor.h/.cpp                   JGGameEntryActor. 월드 생성 직후 엔진이 스폰. 게임이 파생해 OnEnterWorld 에서 GameMasterActor · Controller 스폰
    GameplayEntityActor.h/.cpp              엔티티의 몸. 엔티티 하나에 하나, 그 엔티티 외형의 루트
    GameplayCue.h/.cpp                      연출 단위 (이벤트 종류마다 게임이 파생)
    GameMasterActor.h/.cpp                  연출자. PGameMaster 보유 · 이벤트 → Cue 재생 · 바인딩 표 · 입력 정책. 규칙은 돌리지 않는다
    GameplayControllerActor.h/.cpp          조작자. 명령 초안 → GameMasterActor 제출 · 선택 응답
```

그 밖의 변경: `Source/Runtime/GameFrameWorks/Main.cpp` 삭제, `Source/Programs/JGConsole/Main.cpp` 에 `gmtest` 진입 추가, `JGConsole.module.json` 의존에 GameFrameWorks 추가.

파일 수: 헤더 44 · cpp 39 (GameFrameWorks, 83개) + 콘솔 2. 코드젠 산출물 `Temp/CodeGen/GameFrameWorks/` 11개 클래스분 생성 확인.

---

## 2. 핵심 결정 (구현에 반영된 것)

| 항목 | 결정 |
|---|---|
| 상태 | `HGameplayState` 값 타입. 복사 = 스냅샷. 컴포넌트 테이블은 `IGameplayComponentTable::Clone` 으로 값 복사 |
| 엔티티 | 인덱스 + 세대. 파괴 시 세대 증가. 순회는 인덱스 오름차순 |
| 컴포넌트 | 게임이 `IJsonable` 구조체로 정의. `HGameplayComponentTable<T>` 가 보관. 헤더 툴은 구조체를 처리하지 않으므로 `WriteJson/ReadJson` 을 직접 쓴다 |
| 난수 | PCG32 자체 구현 `HGameplayRandomStream`. 용도별 8 스트림이 상태에 포함 |
| 명령 경로 | `Submit` → `Validate` → 스냅샷 push → `Execute` → 핸들러 → 효과 큐 루프 → 이벤트 목록. 시퀀스 번호 증가. 루프 한 바퀴 = 방금 끝난 단계가 낸 이벤트의 트리거 반응 → 효과 하나 해결. 큐가 비면 예약된 페이즈 전이 단계 |
| 선택 대기 | 효과가 `RequestChoice` → 남은 큐를 `HGameplayChoice::SavedQueue` 에 보존 → `PendingChoice` 반환. 내장 `ResolveChoice` 명령이 같은 효과를 `Choice` 채워 재진입. **효과 `Resolve` 안에서만 받는다.** 핸들러에서 부르면 오류 로그 후 `false`. 트리거는 선택을 요청하는 효과를 넣는다 |
| 트리거 | `Emit` 때는 매칭만 한다(발행 순간 상태로 `Matches`). `React` 는 이벤트를 낸 단계(핸들러 · 효과 · 페이즈 전이)가 끝난 뒤 발행 순 → 우선순위 → 등록 순으로 부른다. `React` 는 `HGameplayTriggerContext`(효과 적재만, 상태 읽기 전용)를 받는다. 반응이 넣은 효과의 깊이는 이벤트 깊이 + 1이다. 깊이 64 를 넘는 효과는 건너뛰고 `EffectDepthExceeded` 를 낸다. 한 명령에서 효과가 4096 개를 넘으면 남은 효과 · 반응을 버리고 페이즈 전이만 마친다 |
| 수치 | `DefineValueStages(kind, stages)` 후 `ctx.Compute` 가 단계 순서로 `JGGameplayModifier` 적용 |
| 페이즈 | RoundStart → OrderResolve → TurnStart → TurnMain → TurnEnd → … → RoundEnd. 순서는 `IGameplayOrderPolicy`(기본 2종). 전이는 `EGameplayPhaseStep` 단계로 나뉘고, 효과 큐가 빈 뒤에 진행한다. 그래서 턴 종료 효과는 다음 행동자가 정해지기 전에, 라운드 시작 효과는 순서가 정해지기 전에 해결된다. 예약 단계는 `HGameplayTurnState::PendingStep` 에 있어 선택 대기 · 저장 · 되돌리기에도 보존된다. 내장 `EndTurn` 효과는 TurnStart 에서도 받는다(턴 시작 효과로 턴 넘기기) |
| 상태 쓰기 | `Add<T>` 는 `T*` 를 돌려주고, 죽은 ID 면 nullptr 이다. 보드 · 영역 쓰기는 `HGameplayState::SetBoardPosition` · `MoveToZone` 이 죽은 ID 를 거부한다. 내장 효과도 이것을 써서 이벤트 없이 건너뛴다. `Board` · `Zones` 를 직접 쓰는 함수는 생존을 보지 않는다. const 상태에서 `Find` · `Get` · `Table` 은 const 를 돌려준다 |
| 되돌리기 | 스냅샷은 `HDeque` 에 담는다(한도 기본 256). 상태에 이동 생성 · 대입이 있어 명령당 상태 복사는 한 번이다 |
| 내장 | 명령 `EndTurn` `ResolveChoice`. 효과 `SpawnEntity` `DestroyEntity` `MoveToZone` `SetBoardPosition` `EndTurn`. 이벤트 15종 |
| 리플렉션 등록 | `RegisterAllFromReflection<T>()` — `JGClass::GetChildClasses` 재귀(중간 클래스 포함) → `AllocateByClass` → `Cast` |
| 결정론 고정 | 레지스트리를 종류 이름 사전순으로 정렬(`Finalize`, Start · Load(ImportDocument) · Replay 가 부른다). 상태 안에 `HHashMap` 없음 |
| 체크섬 | 직렬화 JSON 텍스트의 FNV-1a 64 |
| 저장 | 초기 상태 + 현재 상태 + 명령 로그. 버전 불일치 시 `IGameplayMigrator` |
| GameMasterActor | `JGActor` + `IGameplayObserver` 다중 상속은 `IMemoryObject` 뿌리가 둘이 되므로 별도 관찰자 객체(`PGameMasterActorObserver`)로 분리. 스폰된 엔티티의 액터는 그 이벤트의 큐가 시작하기 전에 만들고, 파괴된 엔티티의 액터는 큐가 끝난 뒤에 없앤다(사망 연출이 액터를 찾는다) |
| 월드 스폰 | 틱 중 스폰한 액터는 다음 틱 시작에 월드에 들어간다. 그 사이 파괴가 예약된 액터는 넣지 않는다 |
| 동적 캐스트 | `Cast<>` 는 정적 타입 관계만 보므로 컴포넌트 · 액터 탐색은 `RawDynamicCast<>` |
| 틱 | 모듈이 `ScheduleByFrame(Update)` 로 게임 인스턴스를 틱하고 게임 인스턴스가 활성 월드를 틱. Unschedule 부재는 GUI 와 같은 기지 이슈 |
| 월드 소유 | **엔진(`JGGameInstance`)이 소유.** 게임은 `SetEntryClass<T>()` 와 `LoadWorld()` 만 부르고 `PWorld` 를 만지지 않는다. 월드 생성 직후 엔트리 액터가 스폰되어 `OnEnterWorld` 에서 내용을 채운다 (언리얼 GameInstance + GameMode 자리) |
| 게임 인스턴스 교체 | 게임 모듈 StartupModule 에서 `ReplaceGameInstance<JGMyGameInstance>()`. 엔트리 클래스는 이어받고, 월드 로드 전이어야 한다 |

---

## 3. 게임 모듈 API 요약

```cpp
// 설정
PSharedPtr<PGameMaster> sim = Allocate<PGameMaster>();
sim->RegisterComponent<HMyData>(PName("MyData"));
sim->RegisterZone(PName("Units"));
sim->DefineValueStages(PName("MyValue"), { "Base", "Add", "Multiply", "Clamp" });
sim->RegisterAllFromReflection<JGGameplayCommandHandler>();   // Effect · Trigger · Modifier 도 같은 식
sim->SetOrderPolicy(Allocate<PGameplayZoneOrderPolicy>(PName("Units")));
sim->SetBoard(EGameplayBoardKind::Hex, 9, 9);
HGameplayState& initial = sim->EditInitialState();            // 엔티티 · 컴포넌트 · 영역 · 보드 배치
sim->Start(seed);

// 플레이
HList<HGameplayEvent> events; PString reason;
EGameplaySubmitResult r = sim->Submit(command, events, &reason);   // Rejected / Executed / PendingChoice
sim->EnumerateLegal(actor, out);  sim->GetPendingChoice();  sim->GetState();

// 되돌리기 · 저장 · 검증 · 에이전트
sim->Undo();  sim->Save(path);  sim->Load(path);  sim->Replay(log, events);  sim->Checksum();
sim->SetAgent(team, Allocate<PGameplayRandomAgent>(seed));  PGameplayAgentRunner::Run(*sim, steps);

// 규칙 (게임) — 효과는 상태를 바꾸고 이벤트를 낸다. 트리거는 효과를 넣기만 한다.
void JGMyEffect::Resolve(HGameplayContext& ctx, const HGameplayEffectRequest& request);   // ctx.State · Emit · Enqueue · RequestChoice · Rng · Compute
void JGMyTrigger::React(HGameplayTriggerContext& ctx, const HGameplayEvent& event);       // ctx.Enqueue / EnqueueFront 만. 상태는 const
HMyData* data = ctx.State.Add<HMyData>(id, value);                                        // 죽은 ID 면 nullptr
ctx.State.SetBoardPosition(id, coord);  ctx.State.MoveToZone(id, PName("Grave"));         // 죽은 ID 면 false

// 월드 쪽 — 게임 모듈 StartupModule
HGameFrameWorksModule* gfw = GModuleGlobalSystem::GetInstance().FindModule<HGameFrameWorksModule>();
gfw->ReplaceGameInstance<JGMyGameInstance>();                  // 선택. 프로세스 수명 상태가 필요할 때
JGGameInstance::Get().SetEntryClass<JGMyGameEntry>();          // JGGameEntryActor 파생
JGGameInstance::Get().LoadWorld(PName("Battle"));              // 엔진이 월드 생성 → 엔트리 스폰 → OnEnterWorld → BeginPlay

// 엔트리 액터 (게임) — OnEnterWorld
PSharedPtr<PWorld> world = GetWorld();
PSharedPtr<JGGameMasterActor> gameMasterActor = world->SpawnActor<JGGameMasterActor>();
gameMasterActor->SetGameMaster(sim);  gameMasterActor->RegisterCuesFromReflection();  gameMasterActor->SetInputPolicy(EGameplayInputPolicy::Buffer);
PSharedPtr<JGGameplayControllerActor> controller = world->SpawnActor<JGGameplayControllerActor>();
controller->SetGameMasterActor(gameMasterActor);  controller->BeginCommand(kind, actor); controller->AddTarget(id); controller->Commit();
```

---

## 4. 자체 테스트 (`JGConsole.exe gmtest`)

`GameMaster/GameMasterSelfTest.cpp` 가 테스트 전용 최소 규칙(TestAct · TestChange · TestHeal · TestPick · TestOnZero 트리거 · TestBonus 수정자)을 수동 등록하고 검사한다.

| 항목 | 검사 |
|---|---|
| 기본 | 시작 이벤트, 페이즈, 첫 행동자, 합법 수 4개, 명령 실행 결과값(10 - (3 + 2) = 5), 턴 외 명령 거부, EndTurn 전이 |
| 연쇄 | 치명 피해 → 파괴 이벤트 → 트리거 회복(깊이 ≥ 1) |
| 결정론 | 같은 시드 · 같은 명령열 3회 → 체크섬 동일 |
| 되돌리기 | 명령 후 Undo → 체크섬 · 로그 복원 |
| 저장/로드 | 로드 후 체크섬 동일, 명령 수용 |
| 리플레이 | 로그 재생 → 체크섬 동일 |
| 선택 대기 | PendingChoice 반환, 후보 2, 대기 중 타 명령 거부, 비후보 선택 거부, 대기 중 저장/로드 후 재개, 재진입 결과값 7 |
| 에이전트 | 무작위 40걸음 · 탐욕 12걸음, 같은 시드 재현 |
| 보드 | 육각 거리, 사각 경로(막힌 칸 우회 4걸음), 점유 · 차단, 범위 5칸 |
| 난수 · 영역 | 같은 시드 수열 동일, 호출 횟수, 셔플 순열성 |
| 9 · 10 · 11 절 (리슨 서버 세션) | 죽은 ID 쓰기 거부(R1–R3), 문서 왕복 · ImportDocument 뒤 트리거 순서(R7), 규칙 지문, Undo 가드, 에이전트 Choose |
| 12 절 리뷰 회귀 (Phase 1) | 트리거가 넣은 효과의 선택 재진입(R4), 핸들러 선택 요청 거부(R5), 턴 종료 효과 해결 시점(R6a), 라운드 시작 효과 → 순서 반영(R6b), 턴 시작 효과로 턴 넘기기, TurnEnd 도중 선택 대기의 문서 저장 · 재개, 트리거 연쇄 깊이 한도(R8), 스냅샷 한도를 넘긴 300 명령(R16), const 상태(R17, static_assert) |
| **월드(`PWorldSelfTest`, 19개)** | 게임 인스턴스 존재, LoadWorld · BeginPlay, GameMasterActor 스폰, 시작 이벤트 소진, 엔티티 3 → 액터 3 바인딩, ID ↔ 액터 왕복, 명령 → EntityDestroyed → 액터 제거, 연출 중 입력 버퍼링 후 실행, Undo → 바인딩 재구성, UnloadWorld. 리뷰 회귀: 한 틱 스폰 · 파괴 좀비(R11), 자식 월드 위치(R12), 사망 큐가 액터를 찾음(R13), 한 프레임 스폰 · 파괴(R15) |

---

## 5. 검증 절차 (Core 복구 후)

```
cd Build/BatchFiles && ../../Bin/DevelopEngine/JGHeaderTool.exe && ../../Bin/DevelopEngine/JGBuildTool.exe   (둘 다 종료 시 segfault 는 정상)
MSBuild Temp/ProjectFiles/GameFrameWorks.vcxproj -p:Configuration=DevelopEngine -p:Platform=x64 -p:SolutionDir="C:\JG\JGEngine\" -m
MSBuild Temp/ProjectFiles/JGConsole.vcxproj      -p:Configuration=DevelopEngine -p:Platform=x64 -p:SolutionDir="C:\JG\JGEngine\" -m
cd Bin/DevelopEngine && ./JGConsole.exe gmtest        → 종료 코드 0, "gmtest: OK"
```

솔루션 빌드에서 `-t:GameFrameWorks` 는 대상 이름을 못 찾는다(솔루션 폴더 경로 필요). vcxproj 를 직접 빌드한다.
격리 워크트리가 `%TEMP%`(세션 스크래치패드) 아래에 있으면 MSBuild 가 헤더 의존을 추적하지 않는다(경고 MSB8029). 헤더를 바꿨으면 `-t:Rebuild` 로 빌드한다.
리뷰 재현 코드(`Document/Memory/tools/gfw_review_repro/ReviewRepro.cpp`)는 워크트리에서만 JGConsole 에 넣어 `simreview` · `simreviewperf` · `simreview256` 으로 돌린다. Phase 1 이후 기대 결과는 R9 · R14 만 `[REPRODUCED]` 이다(백로그).

---

## 6. 남은 것

- 리뷰 재현 중 남은 것: R9(Start 와 첫 명령의 CauseSequence 가 같음, C17) · R14(리플렉션 없는 큐 파생이 기본 큐로 재생, C12). TODO 백로그.
- `JGCameraComponent` · `JGCameraActor` · `JGStaticMeshComponent`(렌더 핸들) · `JGGameplayDevViewComponent`(ImGui) — 미구현. Graphics 핸들 API 와 함께.
- `PGameplayHeadlessRunner`(명령 스크립트 파일 실행) — 미구현. 자체 테스트가 그 역할을 임시로 한다.
- 시퀀서의 인과 기반 병렬 묶음 — GameMasterActor 는 현재 순차 재생 + 스킵만.
- 지난 조사 선결 항목: 메모리 A-2 는 병렬 세션이 완료(`= nullptr` 해제). `getJGObject` 미등록 타입 처리(널 역참조)는 미수정.
