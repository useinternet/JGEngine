# GameFrameWork 위키 문서 재검토 — Engine-46

기록: 2026-10-24 19:01, GameFrameWork 세션. 상위 일감: 구조도 재검토(Engine-36). 사용자 Q-004 답 "JGEngine.md 하위 문서까지 싹 다 검토".
대상: `C:/obsidian/ProjectJG/02. 위키/JGEngine/02. GameFrameWork/` 의 문서 10개(개요 + 분야 9). 문서는 고치지 않았다. 코드는 메인 트리(`86eb96e`)를 읽었다.
본 것: ① 코드와 맞는지 ② 게임 프로젝트가 읽고 이해하기 쉬운지 ③ 나중에 API Reference 로 바로 갈 수 있는지 ④ 한 문서에 설명과 절차(하우투)가 섞였는지.

## 결론

| 본 것 | 결과 |
|---|---|
| ① 코드와 맞는지 | 틀린 설명은 없다. 빠진 것이 넷이다: 위젯 훅 `OnBackAction`, 게임 인스턴스 훅 `OnWorldReady` · `OnWorldUnload`, 실행 인자 `-bind`, 보드 종류 None(보드 없음). JGConsole 의 에셋 DB 설명 한 줄은 `-graphics` 일 때를 빠뜨렸다 |
| ② 게임 프로젝트 눈높이 | 엔진 안 구조는 잘 보인다. 하지만 **게임이 무엇을 상속하고 무엇을 부르는지**가 거의 없다. 규칙 커널 문서는 게임이 상속하는 클래스(`JGGameplayCommandHandler` 등) 이름을 하나도 적지 않았다. 게임이 가장 많이 쓰는 타입(`HGameplayEntityId` · `HGameplayContext`)도 어느 문서에도 없다 |
| ③ API Reference 로 이어지는지 | 제목은 "클래스 이름 (한글)" 규칙을 지켜 링크 자리가 있다. 그러나 ② 의 이름들이 본문에 없어서 Reference 로 이어 줄 곳이 없다. 08 의 "Window" 제목은 클래스 이름이 아니다 |
| ④ 설명과 절차가 섞였는지 | 대부분 설명만 있다. 07(자체 시험)은 절반이 "돌리는 법"이고, 03 의 실행 인자 줄과 08 의 창 설정 JSON 은 설정 참고 자료다. 06 의 짧은 코드 예는 API 예라서 괜찮다 |

## 모든 문서에 공통

| 번호 | 어디를 | 어떻게 | 왜 |
|---|---|---|---|
| 공-1 | 개요 0절 끝 | "게임이 상속하는 클래스" 표를 하나 둔다: 부모 클래스 · 게임이 재정의하는 함수 · 분야 문서 · API Reference. 줄은 아래 "표 초안" | 게임 프로젝트가 처음 찾는 것이 "무엇을 상속하나"다. 지금은 분야 문서 9개를 다 읽어야 알 수 있고, 그래도 규칙 커널 쪽은 이름이 안 나온다 |
| 공-2 | 개요 1절 분야 문서 표 | "API Reference" 칸을 더한다(Engine-36 수정안 2 와 같은 꼴) | 개요 → 분야 → Reference 로 바로 간다 |
| 공-3 | 분야 문서마다, 본문 안 | 게임이 부르는 함수 이름을 개념 옆 괄호에 적는다(예: "위젯을 띄운다(`PushWidget<T>`)"). 따로 절을 만들지 않는다 | 이름이 본문에 있어야 Reference 링크를 걸 수 있다. 절을 따로 만들면 설명과 Reference 가 겹친다 |
| 공-4 | 10개 문서 맨 아래 | "변경 이력" 절(날짜 · 바뀐 것 · 까닭) | 새 문서 유지 규칙(Engine-36 수정안 6) |
| 공-5 | 겹치는 설명 셋 | 한 곳에만 두고 다른 곳은 링크로 바꾼다: "에셋 DB 는 Graphics 가 있을 때만"(개요 0-1 · 05 0절), "Travel 을 엔트리 액터보다 먼저"(01 0-2 · 03 그림), "에디터와 GameHost 가 같은 뷰포트"(01 2절 · 08 0-1) | 위키 규칙: 중복을 쓰지 않는다. 한쪽만 고쳐져 어긋나는 일을 막는다 |

표 초안(공-1, Engine-35 확장점 표에서 GameFrameWork 것만, 코드에서 확인):

| 부모 클래스 | 게임이 재정의하는 함수 | 분야 문서 |
|---|---|---|
| `JGGameInstance` | `OnInit` · `OnWorldReady` · `OnWorldUnload` · `OnShutdown` | 01 |
| `JGGameEntryActor` | `OnEnterWorld` · `OnExitWorld` | 01 |
| `JGActor` · `JGActorComponent` | `OnSpawned` · `OnBeginPlay` · `OnTick` · `OnEndPlay` | 01 |
| `JGGameMasterActor` | `SpawnActorForEntity` · `DestroyActorForEntity` | 02 |
| `JGGameplayControllerActor` | `OnClick` | 02 |
| `JGGameplayCue` | `Accepts` · `Begin` · `Tick` · `IsDone` · `Skip` | 02 |
| `JGGameplayCommandHandler` | `GetKind` · `Enumerate` · `Validate` · `Execute` | 02 |
| `JGGameplayEffect` · `JGGameplayTrigger` · `JGGameplayModifier` | `GetKind` · `Resolve` 등 | 02 |
| `IGameplayFlow` · `IGameplayAgent` · `IGameplayObserver` | 흐름 진행 · 명령 고르기 · 이벤트 받기 | 02 |
| `JGGameWidget` | `OnInitialize` · `OnActivated` · `OnUpdate` · `OnDeactivated` · `OnBackAction` · `OnShutdown` | 00 |

## 문서별 수정안

### GameFrameWork 개요

| 번호 | 어디를 | 어떻게 | 왜 |
|---|---|---|---|
| 개-1 | 0-1 마지막 줄 "JGConsole은 GameFrameWork만 연결한다" | "JGConsole 은 시작할 때 GameFrameWork 만 연결하고, `-graphics` 이면 Graphics 를 그 뒤에 붙인다. 그래서 JGConsole 에는 `-graphics` 여도 에셋 DB 가 없다" | ① GameFrameWork 는 시작할 때 Graphics 가 있어야 에셋 DB 를 등록한다. JGConsole 은 Graphics 를 나중에 붙이므로 DB 가 없다. Engine-30 시험(`JGConsole -graphics gfw.selftest asset`)에서도 DB 검사가 건너뜀이었다 |
| 개-2 | 0절 | 공-1 표 | ② · ③ |

### 00. Game UI

| 번호 | 어디를 | 어떻게 | 왜 |
|---|---|---|---|
| UI-1 | 0-1 위젯 훅 줄 | `OnBackAction`(뒤로가기, `SetBackHandler(true)` 위젯만)을 더한다 | ① 코드(`GameWidget.h:94`)에 있는 훅인데 빠졌다. 바로 위 줄이 뒤로가기를 설명하면서 훅 이름이 없다 |
| UI-2 | 0-1 | 게임이 부르는 곳을 괄호로: 위젯 띄우기 `JGGameInstance::Get().GetUI()->PushWidget<T>(EGameUILayer::…)` · 닫기 `RemoveWidget`, 입력 모드 `SetInputMode`, 게임 코드가 넣는 헤더 `UI/GameUI.h` | ② 개념만 있고 첫 줄을 어떻게 쓰는지가 없다. `UI/GameUI.h` 에는 사용 예까지 있다 |
| UI-3 | 1절 이미지 · 2절 글꼴 | 이미지 텍스처는 `PGameUIManager::LoadTexture` 로 읽어 `PGameUIImage::SetTexture` 에 넣고, 글꼴은 `LoadFont` · 기본 글꼴 지정으로 바꾼다는 한 줄 | ② 게임 UI 에 그림을 넣는 길이 문서에 없다. ProjectAH 가 `IRawTexture` 를 138번 쓰는 까닭이 이 길이다(Engine-35) |
| UI-4 | 1절 | 파생 요소(보호 가상 함수 `onBuildDraw` · `onPointer*`)는 Engine-35 의 공개 여부 결정 뒤에 넣는다 | ③ 공개 API 로 정해지면 Reference 대상이다. 지금 적으면 결정을 앞질러 간다 |

### 01. 월드와 액터

| 번호 | 어디를 | 어떻게 | 왜 |
|---|---|---|---|
| 월-1 | 0-2 | 엔트리 액터 클래스 이름 `JGGameEntryActor`(`OnEnterWorld` · `OnExitWorld`)와 게임 인스턴스 훅 `OnWorldReady`(엔트리 액터 스폰 뒤 · BeginPlay 전) · `OnWorldUnload` 를 적는다 | ① · ② `LoadWorld` 는 엔트리 액터 → `OnWorldReady` → BeginPlay 순서다(`GameInstance.cpp:135-150`). 그림 설명은 `OnWorldLoaded`(방송)만 말하고, 게임이 재정의하는 `OnWorldReady` 가 없다 |
| 월-2 | 1-1 | 게임이 부르는 곳: `PWorld::SpawnActor<T>` · `DestroyActor`, `JGActor::AddComponent<T>` · `FindOrAddComponent<T>`, 메시 넣기 `JGStaticMeshComponent::LoadStaticMesh(경로)` | ② · ③ 액터를 만들고 컴포넌트를 붙이는 첫 줄이 없다 |
| 월-3 | 1-1 | `JGCameraActor` 이름을 적는다(지금은 "카메라 액터"로만) | ③ Reference 링크 자리 |
| 월-4 | 새 줄(0-1 또는 02 의 보드 줄) | `HGameplayBoardLayout`(보드 칸 좌표 → 월드 위치. 사각 · 육각 · 자유, 칸 크기 · 원점)을 한 줄로 | ① `Core/GameplayBoardLayout.h` 가 어느 문서에도 없다. 규칙은 칸 좌표만 알고 월드 위치는 이 값이 정한다 — 보드 게임이 처음 만나는 다리다 |

### 02. 규칙 커널(GameMaster)

| 번호 | 어디를 | 어떻게 | 왜 |
|---|---|---|---|
| 규-1 | 0-1 표 "준비" 줄 | "명령 핸들러 · 효과 · 트리거 · 수정자" 옆에 클래스 이름 `JGGameplayCommandHandler` · `JGGameplayEffect` · `JGGameplayTrigger` · `JGGameplayModifier` 를 적는다. 관찰자는 `IGameplayObserver` | ② · ③ 게임 규칙 코드의 대부분이 이 클래스들의 파생인데, 문서에 이름이 하나도 없다 |
| 규-2 | 0절 첫 문단 · 1절 | 게임 코드가 매일 쓰는 값 타입을 적는다: 엔티티 번호 `HGameplayEntityId`, 명령 `HGameplayCommand`, 이벤트 `HGameplayEvent`, 핸들러 · 효과가 받는 실행 문맥 `HGameplayContext`(되감기 `MarkRewindPoint` · `RewindTo` 가 여기 있다) | ② · ③ 게임 쪽에서 가장 많이 쓰는 이름이 `HGameplayEntityId`(14,677번)와 `HGameplayContext`(2,325번)다(Engine-35). 지금 1-1 은 "효과는 되감기 지점을 표시"라고만 해서 어디에 있는 함수인지 모른다 |
| 규-3 | 0-1 보드 줄 | 보드 종류에 None(보드 없음 — 카드 게임)을 더한다 | ① `EGameplayBoardKind` 는 None · Square · Hex · Free 다(`GameMasterDefines.h:61`). 카드 게임은 None 을 쓴다 |
| 규-4 | 0-1 에이전트 줄 | 이름 `PGameplayRandomAgent` · `PGameplayGreedyAgent`(평가 함수) | ③ |
| 규-5 | 2-1 | 게임이 할 일 두 가지를 적는다: 엔티티를 보여 줄 액터는 `JGGameMasterActor::SpawnActorForEntity` 재정의로 만들고, 연출은 `JGGameplayCue` 파생(`Accepts` → `Begin` → `Tick` → `IsDone`)으로 더한다. 컨트롤러의 `Cancel` 도 명령 조립 줄에 넣는다 | ② 지금은 "엔티티 ↔ 액터 표를 관리한다", "연출 큐로 재생한다"까지만 있고, 게임이 어디를 채우는지가 없다 |

### 03. 네트워크

| 번호 | 어디를 | 어떻게 | 왜 |
|---|---|---|---|
| 네-1 | 0-1 마지막 줄 실행 인자 | 둘 중 하나: (가) `-host[=포트]` · `-join=주소[:포트]` · `-bind=주소` · `-name=이름` 으로 고친다 (나) 이 줄을 하우투("멀티플레이 시험하는 법")로 옮기고 여기는 `HostSession` · `JoinSession` · `LeaveSession` 만 적는다 — 권고 (나) | ① `-bind` 가 빠졌다(`GameInstance.cpp:417`). ④ 실행 인자는 절차다. 게임 코드는 인자가 아니라 `JGGameInstance::HostSession` · `JoinSession` 을 부른다 |
| 네-2 | 1-2 표 | 게임이 부르는 곳: 세션 공유 데이터 `PGameplayHostSession::SetSessionData`, 준비 `SetLocalReady`, 설정 `HGameplaySessionConfig` | ② · ③ 대기방을 만드는 게임이 부를 함수 이름이 없다 |

### 04. 데이터 테이블

| 번호 | 어디를 | 어떻게 | 왜 |
|---|---|---|---|
| 데-1 | 0-1 | 게임 구조체 묶기 예 3~5줄(`BindColumns` · `HDataTableView<T>`, 에디터 저장 뒤 `IsUpToDate` 가 false 면 다시 묶기) — `DataTableView.h` 머리 주석의 예를 줄여 옮긴다 | ② 문장만으로는 `BindColumns` 의 꼴(정적 함수, 묶개를 받음)을 알 수 없다. 코드와 맞는지는 확인함 — 틀린 곳 없음 |

### 05. 에셋

| 번호 | 어디를 | 어떻게 | 왜 |
|---|---|---|---|
| 에-1 | 0-1 | 게임이 부르는 곳: `GAssetDatabase::GetInstance().LoadAssetAsync(경로, 콜백)` · `GetLoadedAsset(경로)`, 메시는 `JGStaticMeshComponent::LoadStaticMesh` | ② 지금은 그림 설명에만 `LoadAssetAsync` 가 있다 |
| 에-2 | 0-1 "JGConsole처럼 Graphics가 없는 프로세스" | 개-1 과 같은 뜻으로 고친다(또는 개요 쪽만 두고 링크 — 공-5) | ① `-graphics` 여도 DB 가 없다 |
| 에-3 | 0절 또는 새 줄 | Engine-30 이 메인에 들어간 뒤: 컨텐츠 브라우저용 `ListAssetFolder`(폴더 → 하위 폴더 · 에셋 · 원본 파일) · `ImportAssetFile`(원본 → 저장 폴더, 확장자로 가져오기 클래스 고름) | ① 새 공개 함수. 패치는 `Temp/ContentBrowser/GameFrameWork.patch`(아직 메인 밖) |

### 06. 오디오

| 번호 | 어디를 | 어떻게 | 왜 |
|---|---|---|---|
| 오-1 | — | 고칠 것 없음. Reference 칸만(공-2) | ① 예제의 `HAudio::Play` · `PlayMusic` · `SetBusVolume` · `SetBackgroundMode`, 묶음 Master · Music · Effects, 출력 세 가지 모두 코드와 같다. ② 짧은 예가 있어 게임이 바로 쓴다 |

### 07. 자체 시험

| 번호 | 어디를 | 어떻게 | 왜 |
|---|---|---|---|
| 시-1 | 0-1 그룹 표 · 그림 | Engine-30 이 메인에 들어간 뒤 `asset` 그룹(컨텐츠 브라우저 폴더 읽기 · 가져오기)을 더하고 "8개" → "9개" | ① 새 그룹 |
| 시-2 | 문서 전체 | (판단 요청) 이 문서는 엔진 개발자가 시험을 돌리는 법이 중심이다. 구조(등록 파일 · `HSelfTestResult`)만 남기고 명령 쓰는 법 · 종료 코드는 하우투로 옮길지 | ④ 새 규칙: 위키 문서는 한 종류로만. 게임 프로젝트는 이 문서를 읽을 일이 거의 없다 |

### 08. GameHost 모듈

| 번호 | 어디를 | 어떻게 | 왜 |
|---|---|---|---|
| 호-1 | 1절 제목 "Window (창 설정)" | `HGameWindowSettings (창 설정)` | ③ 제목 규칙 "클래스 이름 (한글)". `Window` 는 JSON 키라 Reference 로 갈 곳이 없다 |
| 호-2 | 1절 JSON · 명령줄 인자 | 나중에 `.jgproject` 설정 참고 자료(Reference)가 생기면 그리로 옮기고 여기는 링크 | ④ 설정 값 목록은 참고 자료다. 지금은 옮길 곳이 없으니 그대로 둔다 |

## 확인한 것(코드)

| 문서 주장 | 근거 |
|---|---|
| 모듈 시작 · 종료 순서, Graphics 있을 때만 에셋 DB | `Core/GameFrameWorkModule.cpp:15-50` |
| `ReplaceGameInstance<T>`, Update 단계 `tick`(간격 0.25 초 상한) | `GameFrameWorkModule.h:31` · `.cpp:33, 85-105` |
| 규칙 커널 · 네트워크 · 데이터 테이블에 Graphics include 없음 | `GameMaster/` · `Network/` · `Data/` grep — 없음. `Data/DataTable.h` → `Asset/Asset.h` → Core 만 |
| JGConsole 은 GameFrameWork 먼저, `-graphics` 는 뒤 | `Programs/JGConsole/Main.cpp:24, 141-148` |
| 게임 UI 레이어 4개 · 위젯 훅 · 요소 5종 · 대체 글꼴 · 렌더러가 타깃을 지우지 않음 | `UI/GameUIDefines.h:51`, `UI/GameWidget.h:88-94`, `UI/` 파일, `GameUIFont.h:106`, `GameUIRenderer.h:9` |
| 월드 로드 순서(언로드 → Travel → 엔트리 → `OnWorldReady` → `OnWorldLoaded` → BeginPlay), 스폰 · 파괴는 틱 끝 | `Core/GameInstance.cpp:120-150`, `Core/World.h:17-18, 84-85` |
| 액터 훅 · `SpawnActor<T>` · `AddComponent<T>` · `PickActor` | `Actors/Actor.h:67-109`, `Core/World.h:37-80` |
| 뷰포트 입력: UI 먼저 · 컨트롤러 `HandleClick` · Esc 뒤로가기 | `Viewport/GameViewport.cpp:177, 361-532` |
| `PGameMaster` 함수 이름 14개 · `MarkRewindPoint` · `RewindTo` | `GameMaster/GameMaster.h`, `Rules/GameplayContext.h:54, 63` |
| 컨트롤러 명령 조립 · 입력 정책 Block · Buffer · SkipAndExecute | `Actors/GameplayControllerActor.h:23-79`, `Actors/GameMasterActor.h:12-14` |
| 메시지 종류 · 프로토콜 버전 · 실행 인자 bind · host · join · name | `Network/Messages/GameplayNetMessage.h:9-30`, `Core/GameInstance.cpp:362-420` |
| 데이터 테이블 열 7종 · `ApplyContent` · `OnChanged` | `Data/DataTableTypes.h:6-16`, `Data/DataTable.h:11-53` |
| 패키지 Content 경로 · `.jgasset` 붙이기 · 텍스처 리드백 · `GetMesh` | `Core/FileIO/FileHelper.cpp:635, 791`, `Asset/AssetPath.cpp:127`, `Asset/TextureAsset.cpp:44-48`, `Asset/StaticMeshAsset.cpp:340-364` |
| `HAudio` 함수 · 묶음 · 비활성 모드 | `Audio/Audio.h:30-57`, `Audio/AudioDefines.h:6-20` |
| GameHost 인자 · 창 모드 · 크기 변경 뒤 맞추기 · 최소화 쉬기 | `GameHost/GameHostModule.cpp:35-75, 245-281, 320-372` |

## 가 반영 — Engine-62 (2026-10-24, 보고함 Q-011 답 "가 · 나 · 다 차례로 모두 고침")

위키를 직접 고쳤다. 고치기 전 사본: `Temp/Light/wiki_backup_Engine-62/`. 끊긴 링크 0, 10개 문서 모두 변경 이력 절이 있다.

| 문서 | 고친 곳 | 수정안 |
|---|---|---|
| 00. Game UI | 0-1 뒤로가기 줄에 `OnBackAction` | UI-1 |
| 01. 월드와 액터 | 0-2 에 `OnWorldReady` · `OnWorldUnload` | 월-1(훅만) |
| 02. 규칙 커널 | 0-1 보드 줄에 보드 종류 None | 규-3 |
| 03. 네트워크 | 실행 인자 줄에 `-bind=주소` | 네-1(가) — 줄을 하우투로 옮기는 것은 다(Engine-66) |
| 05. 에셋 | JGConsole 줄: `-graphics` 여도 에셋 DB 없음 + 개요 링크 | 에-2 · 공-5 |
| 08. GameHost 모듈 | 1절 제목 `HGameWindowSettings (창 설정)` · 번호 표 | 호-1 |
| GameFrameWork 개요 | JGConsole 연결 순서 · 에셋 DB 없음, 에셋 DB 줄에 05 링크 | 개-1 · 공-5 |
| 04 · 06 (+ 위 문서들) | 변경 이력 절 | 공-4 |

공-5 의 나머지 둘은 고치지 않았다: "Travel 을 엔트리 액터보다 먼저"는 03 본문에 겹치는 글이 없고(그림의 차례뿐), "에디터와 GameHost 가 같은 뷰포트"는 08 이 이미 짧게 쓰고 01 로 링크한다.
나로 남긴 것(Engine-40): 공-1 · 공-2 · 공-3, UI-2 · 3 · 4, 월-2 · 3 · 4, 규-1 · 2 · 4 · 5, 네-2, 데-1, 에-1. 다로 남긴 것(Engine-66): 시-2, 호-2, 네-1 의 옮기기.
