# GameFrameWorks 이름 변경안 — Simulation → GameMaster · Gameplay

작성 2026-09-29. **적용 완료 2026-09-29 11:55** (검증 기록 `Document/Memory/2026-09-29_gfw_rename_verify.txt`). 대상은 `Source/Runtime/GameFrameWorks/` 와 `Source/Programs/JGConsole/`.
근거: 사용자 결정 "파사드는 GameMaster, 타입 접두어는 Gameplay", 이어서 "Director 는 `JGGameMasterActor`, 엔티티 액터는 `JGGameplayEntityActor`, Actor 접미어 유지" (2026-09-29).

## 최종 결정 (적용된 것)

| 항목 | 결정 |
|---|---|
| 규칙 | 파사드 `PGameMaster` 와 그것을 든 월드 액터 `JGGameMasterActor`, 시스템 단위 이름은 GameMaster. 그 밖의 타입은 Gameplay |
| ① 엔티티 액터 | `JGSimulationActor` → `JGGameplayEntityActor` |
| Director | `JGSimulationDirectorActor` → `JGGameMasterActor`. 딸린 이름 `PGameMasterActorObserver` · `EGameMasterActorSubmit` · `SetGameMasterActor` · `GetGameMasterActor` |
| 조작자 | `JGSimulationControllerActor` → `JGGameplayControllerActor` (엔티티 액터 이름에 Entity 가 있어 파생 오해 없음) |
| ② 콘솔 명령 | `simtest` → `gmtest`. 옛 이름 `simtest` 도 받는다 (리뷰 기록의 검증 절차 호환) |
| ③ 폴더 | `Simulation/` → `GameMaster/` |
| 결과 | 변경 전후 자체 테스트 75개와 리뷰 재현 결과가 줄 단위로 동일. 메인 트리 빌드 오류 0, `gmtest` 75/75, 런처 30초 종료 코드 0 |

아래 §1 이후는 적용 전 검토안이다. 월드 쪽 표(§3)와 메서드 표(§4)는 위 최종 결정으로 갱신했다.

## 0. 결론

- **규칙.** 파사드 하나와 시스템 단위 이름(폴더 · 정의 파일 · 자체 테스트 · 로그 카테고리)은 `GameMaster`, 그 밖의 모든 타입은 `Gameplay`. (최종: 파사드를 든 월드 액터 `JGGameMasterActor` 도 GameMaster)
- **규모.** 타입 62 · 메서드와 멤버 13 · 파일 72 와 폴더 1 · include 경로 33 · 로그 카테고리 2 · 주석 18곳. 모듈 밖 소스 참조는 없음. 리뷰 재현 코드 1개와 문서 6건이 따라 바뀜.
- **결정 필요 3개** (§2): 엔티티 액터 이름, 콘솔 명령 이름, 폴더 이름.
- **순서.** 리뷰(`GameFrameWorks_설계코드리뷰_2026-09-28.md`) 수정보다 먼저 한다. 기계적 치환이라 `simtest` 75/75 와 리뷰 재현 결과(R1–R17) 동일로 "동작 변화 없음"을 증명할 수 있다.
- GameFrameWorks 는 아직 커밋 전이다(추적 파일은 `module.json` 하나). 파일 이동에 보존할 git 이력이 없다.

---

## 1. 규칙

| 대상 | 이름 | 예 |
|---|---|---|
| 파사드 | `GameMaster` | `PGameMaster` |
| 시스템 단위: 폴더 · 정의 파일 · 자체 테스트 · 로그 카테고리 | `GameMaster` | `GameMaster/` · `GameMasterDefines.h` · `PGameMasterSelfTest` · `JG_LOG(GameMaster, …)` |
| 그 밖의 타입 (상태 · 메시지 · 규칙 · 보드 · 서비스 · 에이전트 · 월드 쪽) | `Gameplay` | `HGameplayState` · `JGGameplayEffect` · `IGameplayBoard` · `JGGameplayCue` |
| 파사드를 가리키는 메서드 · 멤버 | `GameMaster` | `SetGameMaster` · `_gameMaster` |
| 파일 이름 | 담은 타입 이름에서 접두 글자를 뺀 것 (기존 규칙 그대로) | `HGameplayState` → `GameplayState.h` |

---

## 2. 결정 필요

| # | 항목 | 권장 | 대안 | 이유 |
|---|---|---|---|---|
| ① | `JGSimulationActor` | **`JGGameplayEntityActor`** | `JGGameplayActor` | "Gameplay 액터"는 모든 액터로 읽힌다. 엔티티에 묶인 액터라는 뜻을 이 한 곳에서 지키고, `JGGameplayEntityComponent` 와 짝이 맞는다 |
| ② | 콘솔 명령 `simtest` | **`gmtest`** | 유지 | 시스템 이름과 맞춘다. 문서의 명령 예시도 함께 바뀐다 |
| ③ | 폴더 `Simulation/` | **`GameMaster/`** | 유지 | 앞서 정한 4폴더 중 하나를 바꾸는 것이라 확인이 필요하다. 시스템 이름과 폴더가 같아야 include 경로만 보고 찾아간다 |

---

## 3. 타입 (62)

### 파사드 · 시스템 (2)

| 지금 | 변경 |
|---|---|
| `PSimulation` | `PGameMaster` |
| `PSimulationSelfTest` | `PGameMasterSelfTest` |

### 상태 (12)

| 지금 | 변경 |
|---|---|
| `HSimulationState` | `HGameplayState` |
| `HSimulationEntityId` | `HGameplayEntityId` |
| `HSimulationCoord` | `HGameplayCoord` |
| `HSimulationEntityRegistry` | `HGameplayEntityRegistry` |
| `ISimulationComponentTable` | `IGameplayComponentTable` |
| `HSimulationComponentTable<T>` | `HGameplayComponentTable<T>` |
| `HSimulationZone` | `HGameplayZone` |
| `HSimulationZoneSet` | `HGameplayZoneSet` |
| `HSimulationTurnState` | `HGameplayTurnState` |
| `HSimulationBoardState` | `HGameplayBoardState` |
| `HSimulationRandomStream` | `HGameplayRandomStream` |
| `HSimulationChoice` | `HGameplayChoice` |

### 메시지 (3)

| 지금 | 변경 |
|---|---|
| `HSimulationCommand` | `HGameplayCommand` |
| `HSimulationEffectRequest` | `HGameplayEffectRequest` |
| `HSimulationEvent` | `HGameplayEvent` |

### 규칙 (15)

| 지금 | 변경 |
|---|---|
| `HSimulationContext` | `HGameplayContext` |
| `JGSimulationCommandHandler` | `JGGameplayCommandHandler` |
| `JGSimulationEffect` | `JGGameplayEffect` |
| `JGSimulationTrigger` | `JGGameplayTrigger` |
| `JGSimulationModifier` | `JGGameplayModifier` |
| `HSimulationValueQuery` | `HGameplayValueQuery` |
| `ISimulationOrderPolicy` | `IGameplayOrderPolicy` |
| `PSimulationZoneOrderPolicy` | `PGameplayZoneOrderPolicy` |
| `PSimulationKeyedOrderPolicy` | `PGameplayKeyedOrderPolicy` |
| `PSimulationRegistry<T>` | `PGameplayRegistry<T>` |
| `PSimulationEffectQueue` | `PGameplayEffectQueue` |
| `PSimulationValuePipeline` | `PGameplayValuePipeline` |
| `PSimulationTriggerDispatcher` | `PGameplayTriggerDispatcher` |
| `PSimulationPhaseMachine` | `PGameplayPhaseMachine` |
| `PSimulationRuleEngine` | `PGameplayRuleEngine` |

### 보드 (5)

| 지금 | 변경 |
|---|---|
| `ISimulationBoard` | `IGameplayBoard` |
| `PSimulationBoardNone` | `PGameplayBoardNone` |
| `PSimulationBoardSquare` | `PGameplayBoardSquare` |
| `PSimulationBoardHex` | `PGameplayBoardHex` |
| `PSimulationBoardFree` | `PGameplayBoardFree` |

### 서비스 (5)

| 지금 | 변경 |
|---|---|
| `PSimulationSnapshotStack` | `PGameplaySnapshotStack` |
| `PSimulationCommandLog` | `PGameplayCommandLog` |
| `PSimulationSerializer` | `PGameplaySerializer` |
| `ISimulationObserver` | `IGameplayObserver` |
| `ISimulationMigrator` | `IGameplayMigrator` |

### 에이전트 (5)

| 지금 | 변경 |
|---|---|
| `ISimulationAgent` | `IGameplayAgent` |
| `ISimulationEvaluator` | `IGameplayEvaluator` |
| `PSimulationRandomAgent` | `PGameplayRandomAgent` |
| `PSimulationGreedyAgent` | `PGameplayGreedyAgent` |
| `PSimulationAgentRunner` | `PGameplayAgentRunner` |

### 열거형 · 상수 (6)

| 지금 | 변경 |
|---|---|
| `ESimulationPhase` | `EGameplayPhase` |
| `ESimulationSubmitResult` | `EGameplaySubmitResult` |
| `ESimulationBoardKind` | `EGameplayBoardKind` |
| `ESimulationRandomStream` | `EGameplayRandomStream` |
| `HSimulationBuiltin` | `HGameplayBuiltin` |
| `HSimulationLimits` | `HGameplayLimits` |

내장 명령 · 효과 · 이벤트의 문자열 값(`EndTurn`, `ResolveChoice`, `EntitySpawned` …)은 그대로다.

### 월드 쪽 (8)

| 지금 | 변경 |
|---|---|
| `JGSimulationEntityComponent` | `JGGameplayEntityComponent` |
| `JGSimulationActor` | `JGGameplayEntityActor` (결정 ①) |
| `JGSimulationCue` | `JGGameplayCue` |
| `JGSimulationDirectorActor` | `JGGameMasterActor` (최종 결정) |
| `PSimulationDirectorObserver` | `PGameMasterActorObserver` |
| `JGSimulationControllerActor` | `JGGameplayControllerActor` |
| `ESimulationInputPolicy` | `EGameplayInputPolicy` |
| `ESimulationDirectorSubmit` | `EGameMasterActorSubmit` |

### 테스트 전용 (1)

| 지금 | 변경 |
|---|---|
| `HSimulationTestValue` | `HGameplayTestValue` |

바뀌지 않는 것: `PWorld` · `JGActor` · `JGActorComponent` · `HTransform` · `JGGameInstance` · `JGGameEntryActor` · `HGameFrameWorksModule` · `PWorldSelfTest`.

---

## 4. 메서드 · 멤버 · 지역 이름 (13)

| 지금 | 변경 | 위치 |
|---|---|---|
| `SetSimulation` | `SetGameMaster` | Director |
| `GetSimulation` | `GetGameMaster` | Director · Controller |
| `GetOrCreateSimulation` | `GetOrCreateGameMaster` | Director |
| `_simulation` | `_gameMaster` | Director |
| `simulation` | `gameMaster` | 매개변수 · 지역 변수 (에이전트 · Controller · Director) |
| `OnSimulationEvents` | `OnGameplayEvents` | `IGameplayObserver` |
| `OnSimulationStateReplaced` | `OnGameplayStateReplaced` | `IGameplayObserver` |
| `onSimulationEvents` | `onGameplayEvents` | Director 내부 |
| `onSimulationStateReplaced` | `onGameplayStateReplaced` | Director 내부 |
| `RunSimulationSelfTest` | `RunGameMasterSelfTest` | 모듈 |
| `runSimulationSelfTest` | `runGameMasterSelfTest` | JGConsole |
| `setupSimulation` | `setupGameMaster` | 자체 테스트 |
| `countSimulationActors` | `countEntityActors` | 월드 자체 테스트 |
| `SetDirector` · `GetDirector` | `SetGameMasterActor` · `GetGameMasterActor` | 조작자 (최종 결정) |
| `_director` · `director` | `_gameMasterActor` · `gameMasterActor` | 조작자 · 관찰자 · 큐 `Begin` 인자 (최종 결정) |

지역 변수 `sim` 은 그대로 둔다.

---

## 5. 파일 · 폴더 (72 + 폴더 1)

| 지금 | 변경 | 수 |
|---|---|---|
| `Simulation/` | `GameMaster/` (결정 ③) | 폴더 |
| `Simulation/Simulation.h/.cpp` | `GameMaster/GameMaster.h/.cpp` | 2 |
| `Simulation/SimulationDefines.h` | `GameMaster/GameMasterDefines.h` | 1 |
| `Simulation/SimulationSelfTest.h/.cpp` | `GameMaster/GameMasterSelfTest.h/.cpp` | 2 |
| `Simulation/<하위>/Simulation<X>.*` | `GameMaster/<하위>/Gameplay<X>.*` | 57 |
| `Actors/SimulationActor.*` | `Actors/GameplayEntityActor.*` (결정 ①) | 2 |
| `Actors/SimulationDirectorActor.*` | `Actors/GameMasterActor.*` (최종 결정) | 2 |
| `Actors/Simulation{Cue,ControllerActor}.*` | `Actors/Gameplay{Cue,ControllerActor}.*` | 4 |
| `Components/SimulationEntityComponent.*` | `Components/GameplayEntityComponent.*` | 2 |

하위 폴더 `State/ Messages/ Rules/ Boards/ Services/ Agents/` 는 이름 그대로 `GameMaster/` 아래로 옮긴다.

**코드젠.** JGCLASS 헤더 9개의 이름이 바뀌어 `*.generation.h` 이름도 바뀐다. 프로젝트가 `Temp/CodeGen/GameFrameWorks/` 를 통째로 포함하므로, 옛 `Simulation*.generation.cpp` 가 남으면 지워진 헤더를 include 해서 빌드가 깨진다. **PreBuild 전에 그 폴더를 비운다.**

---

## 6. 문자열 · 로그 · 주석

| 대상 | 지금 | 변경 |
|---|---|---|
| 로그 카테고리 | `Simulation` (9곳) | `GameMaster` |
| 로그 카테고리 | `SimulationSelfTest` (2곳) | `GameMasterSelfTest` |
| 로그 메시지 안 클래스 이름 | `PSimulationSerializer: …` 등 | 타입 변경을 따라감 |
| 테스트 출력 | `== Simulation self test ==` | `== GameMaster self test ==` |
| 테스트 저장 파일 | `SimulationSelfTest_{Save,Pending}.json` | `GameMasterSelfTest_{Save,Pending}.json` |
| 콘솔 출력 | `simtest: OK` | `gmtest: OK` (결정 ②) |
| 주석 `시뮬레이션` (15파일 18곳) | 시스템을 가리키는 곳 | `GameMaster` 로. 일반 개념으로 쓴 곳은 유지 |

---

## 7. 같이 바뀌는 것

| 대상 | 내용 |
|---|---|
| `Source/Programs/JGConsole/Main.cpp` | include 경로 · 호출 · 명령 이름 |
| `Document/Memory/tools/gfw_review_repro/ReviewRepro.cpp` | 리뷰 세션의 재현 코드. 261곳. 같은 표로 치환해야 계속 컴파일된다 |
| 문서 | `설계방안B_상세설계` · `설계방안B_인터페이스_구현가이드` · `게임프레임워크_설계방안_요약` · `GameFrameWorks_구현현황` · `GameFrameWorks_TODO` · `Memory/2026-09-28_턴제게임_프레임워크_방향성` |
| 리뷰 보고서 | 다른 세션의 기록이라 본문은 고치지 않고, 맨 위에 이 문서를 가리키는 한 줄만 넣는다 |
| 지속 메모리 | `jgengine-tool-quirks.md` 의 `HSimulationEvent` 예시 |

---

## 8. 적용 절차와 검증

1. 기준선. 격리 워크트리에서 지금 이름으로 리뷰 재현 코드를 돌려 R1–R17 결과를 저장한다.
2. 치환. perl 스크립트로 파일 이동 후 토큰 치환. 긴 이름부터 바꾸고, 예외(§2 ① · §4)를 일반 규칙보다 먼저 적용한다. 이 환경의 python 은 스토어 스텁이라 쓰지 않는다.
3. 잔여 확인. `Simulation` 과 `simulation` 이 소스에 0건인지 grep.
4. 빌드. `Temp/CodeGen/GameFrameWorks/` 비움 → PreBuild → `GameFrameWorks.vcxproj` · `JGConsole.vcxproj`.
5. 검증. `gmtest` 75/75 · 종료 코드 0 · 로그 오류 0, 런처 30초 회귀, 재현 코드 결과가 1번과 같음.
6. 문서 갱신 (§7).
