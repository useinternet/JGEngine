# API Reference 계획 · 꼴 견본 (Engine-34)

- 일감: Engine-34(상위 Engine-33), 담당 Core, 2026-10-24 18:56
- 요청: 사용자 대시보드 요청 r-20261024-183950-a5cc(18:39) — 게임 프로젝트가 엔진 코드를 보지 않고 쓸 수 있는 공개 C++ API Reference, 공개 API 문서 주석, 모듈 README, 변경 이력
- 견본: `Document/기록/Core/Engine-34_견본/` — 코드는 고치지 않았다

## 0. 결론

| 정할 것 | 안 |
|---|---|
| 대상 | 게임이 쓰는 이름(Engine-35): Core 45 · GameFrameWork 84 · Graphics 2 · GUI 2 · GameHost 0 · JGEditor 0. Reference 는 Core · GameFrameWork 가 대부분(아래 1) |
| 공개 기준 | 게임 모듈 · 게임 에디터 모듈 · 게임 프로젝트 틀이 쓰는 것 + 그것을 쓰려면 알아야 하는 형 + 엔진이 열어 둔 확장 지점. 엔진 내부 · 엔진 프로그램 전용 · 플랫폼 구현은 뺀다 |
| 위키 자리 | **결정 1** — 안 A 모듈 폴더 아래 「Core API Reference」(권고) / 안 B 모듈 문서 옆 「00. Core API Reference」 |
| 꼴 | 항목 = 개요 · 선언 · 매개변수/반환값 · 사용 예시 · 주의사항(스레드 · 수명 · 순서 · 오류 4줄). 문서 주석 = `///` + `@param` · `@returns`(Visual Studio 툴팁에 보임). README = 역할 · 의존 · 주요 클래스 · 예시. 변경 이력 = 날짜 · API · 내용 · 게임 영향 · 일감 |
| 예시 확인 | **결정 2** — 안 1 예시를 엔진 빌드에 넣음(JGApiSamples, 권고) / 안 2 문서를 쓸 때만 떼어 둔 폴더에서 |
| 문서 주석 작업 | **결정 3** — 이 계획 확인을 주석 작업의 설계도 확인으로 봄(권고) / 모듈마다 설계도를 다시 올림 |
| 일수 | 모두 약 14일(세션 · 일). Core 를 먼저 1일 써서 꼴을 굳힌 뒤 나머지를 나란히 하면 달력으로 약 7일 |

그림: `Saved/UIShots/engine34_wiki_layout.png`(위키 자리 비교) · `Saved/UIShots/engine34_doc_layers.png`(한 API 가 놓이는 세 곳 · 예시 확인 방법). 원본 HTML 은 `Engine-34_견본/그림/`.

## 1. 대상 모듈과 경계

게임 모듈이 링크하는 엔진 모듈: ProjectAH = Core · Graphics · GameFrameWork, ProjectAHEditor = Core · GUI · GameFrameWork(각 `*.module.json`). 게임 쪽 코드가 쓰는 엔진 이름은 DevOps 가 모은 게임 프로젝트가 쓰는 엔진 API 목록(Engine-35, `Document/기록/DevOps/Engine-35_게임이쓰는엔진API_2026-10-24.md` — ProjectAH 231 파일 · 게임 프로젝트 틀 · 검증 게임 4개)을 바닥으로 쓴다.

| 모듈 | 게임이 쓰는 방식 | 게임이 쓰는 이름(Engine-35) | API Reference 범위 | 일감 |
|---|---|---|---|---|
| Core | 모든 게임 모듈 | 45 + 정적 함수 38 · 상속 3(`IJsonable` · `IMemoryObject` · `IModuleInterface`) | 기본 형 · 문자열 · 컨테이너, 메모리, 로그, 파일, JSON, 수학, 모듈, 콘솔 명령, 리플렉션 표시, 델리게이트 · 스케줄러 · 시간 · 입력 | Engine-38 |
| GameFrameWork | 게임 로직 · UI · 네트워크 · 에셋 · 오디오 | 84 + 정적 함수 88 · 상속 11 | 규칙 커널 · 게임 UI · 세션 · 게임 인스턴스 · 액터 · 오디오 + 쓰지는 않지만 공개할 것(DataTable · 컴포넌트 · 규칙 커널 확장 · 에셋) | Engine-40 |
| Graphics | 게임 UI 텍스처 핸들 | 2 (`IRawTexture` · `HVertex`) | `IRawTexture` 를 "게임 UI 텍스처 핸들"로. 나머지는 GameFrameWork 가 감싼다 | Engine-39 |
| GUI | 게임 에디터 모듈의 메뉴 | 2 (`HGUIModule` · `HMainMenuItem`) | 메뉴 등록 + 도구 창 기반(`HGUI` · `JGWidget`) 후보 | Engine-41 |
| GameHost | `.jgproject` 의 `Package.Window` · 명령줄 인자 | 0 | C++ API 없음 → README 만(설정 · 인자는 하우투 문서 몫). GameFrameWork 세션이 Engine-40 과 함께 | Engine-40 |
| JGEditor | `.jgproject` 의 EditorModules 를 올림. 게임 에디터 모듈은 JGEditor 를 링크하지 않고 GUI 로 확장한다 | 0 | 공개 C++ API 없음 → README 와 "확장 지점 = GUI 모듈" 한 줄. Reference 는 지금 만들지 않음 | Engine-42 |

- 게임은 Core 를 거의 include 없이 쓴다. 게임 프로젝트 틀이 만드는 `<이름>Defines.h` 가 `Core.h`(Core 헤더 대부분)를 include 하고 게임 헤더가 그것을 include 하기 때문이다(`Build/Templates/GameProject/Source/{PROJECT_NAME}/{PROJECT_NAME}Defines.h`). 엔진 PCH(`Source/PCH/PCH.h`)는 표준 · DX12 · spdlog · rapidjson 만 넣고 `Core.h` 는 넣지 않는다 — 그래서 PCH 만 쓰는 엔진 프로그램(JGConsole)에서는 견본 예시에 `Core.h` 가 필요했다(4절). 따라서 Core Reference 는 헤더보다 기능 묶음으로 나누고, 항목마다 "`Core.h` 에 들어 있음 / 따로 include" 를 적는다.
- 게임이 상속해 쓰는 확장점(Engine-35 1절 — `IModuleInterface` · `IJsonable` · `JGGameInstance` · `JGGameEntryActor` · `JGGameWidget` · 규칙 커널 핸들러 · 효과 · 흐름 · 에이전트 …)은 Reference 에 "상속해 쓰는 클래스" 묶음으로 따로 둔다 — 재정의할 virtual 함수와 엔진이 부르는 차례를 적는다.

### Core 공개 판단 (Engine-35 판단 거리)

| 이름 | 게임 쓰임 | 판단 | 까닭 |
|---|---|---|---|
| `HRawString` | 231 | 공개 | `PString` 과 표준 문자열 사이에서 게임이 실제로 쓴다. "`PString` 이 감싼 `std::string`"으로 적는다 |
| rapidjson 직접 사용 | `ArkhamJson.*` | 공개 API 밖 | 외부 라이브러리 API 는 쓰지 않는다. 공개 범위는 `PJson` · `PJsonData` · `IJsonable`. `PJson::GetDocument` · `GetAllocator`(rapidjson 형을 돌려줌)는 "외부 라이브러리 원본으로 나가는 출구"로만 적는다 |
| `GCoreSystem::RequestExit` | 1(검증 게임) | 공개 | 게임이 스스로 끝내는 길(종료 버튼 등). `GCoreSystem` 의 나머지 · `GetGlobalValues` · `PJWindow::GetHandle` 은 내부 |
| `GConsoleCommandGlobalSystem` | 1(검증 게임 `Execute`) | `Execute` · `Submit` · `Register` · `Unregister` 공개 | 게임 UI 에서 명령 실행, 객체에 묶인 핸들러 등록(헤더 주석이 이미 이 쓰임을 안내). `ExecutePending` · `FindCommands` 는 내부(에디터 · JGConsole 용) |
| `JGPROPERTY` · `JGFUNCTION` · `JGENUM` · `JGENUMMETA` | 0 | 공개 | 게임 클래스 속성을 JSON 으로 저장하는 길. `JGCLASS` 와 한 묶음 |
| 델리게이트 · `HDelegateHandle` · `JG_DECLARE_EVENT` | `AddLambda` 274 등 | 공개 | GameFrameWork API 가 이벤트를 델리게이트로 내준다 |
| `PDateTime` · `PTimespan` · `HGuid` | 0 | 공개 | 저장 · 식별 · 시간의 기본 형. 짧게 |
| 스케줄러(`GScheduleGlobalSystem` 예약 함수 · `EMainThreadExecutionOrder` · `HTaskHandle`) | 0 | 공개 | 엔진이 주는 지연 · 주기 호출과 작업 스레드 |
| `EKeyCode` · `HInputEvent` | 0 | 지금은 공개 밖 | GameFrameWork 안에서도 `Viewport/GameViewport.h`(공개 밖)만 쓴다. 게임이 키 입력을 직접 받는 API 가 생기면 그때 공개 |

### 공개 기준

| 넣는다 | 뺀다 |
|---|---|
| 게임 모듈 · 게임 에디터 모듈 · 게임 프로젝트 틀(`Build/Templates/GameProject`)이 include · 호출 · 상속 · 매크로로 쓰는 것 | 엔진 내부 구현 — 메모리 풀(`HMemoryPool` …), 리플렉션 등록 · codeGen, 명령 레지스트리 등록 루틴(`HAutoConsoleCommand::RegisterAll`), `*Private` 네임스페이스, `JG_MODULE_IMPL` 이 만드는 진입 함수 |
| 그것을 쓰려면 알아야 하는 형 — 매개변수 · 반환 형(`HConsoleCommandArgs` · `PSharedPtr` · `PString` …) | 엔진 프로그램 전용 — `HModuleSystemInfo` · `HRuntimeModuleInfo` · 서술자 쓰기(JGHeaderTool · JGBuildTool · 쿠킹) |
| 엔진이 게임에 열어 둔 확장 지점 — 상속하라고 만든 기반 클래스의 virtual 함수(`IModuleInterface::StartupModule` …), 등록 매크로(`JG_MODULE_IMPL` · `JGCLASS`) | 플랫폼 구현 — `PWindowsJWindow` · `HWindowsInputTranslator` |
| | public 이지만 게임이 부르지 않는 멤버 — 헤더에 "엔진 내부용" 한 줄만 달고 Reference 에서 뺀다 |
| | 외부 라이브러리 원본 API(ImGui · spdlog · rapidjson) |

## 2. 위키 자리 · 문서 이름 (결정 1)

대시보드 위키 목록은 폴더와 같은 이름의 문서를 그 폴더 줄로 보여 주고(`03. 대시보드/server.js` wikiMain), 위키 전체에서 같은 문서 이름을 막는다(`/api/wiki/new` clash). 그래서 이름은 `<모듈> API Reference` 로 위키 전체에서 하나가 되게 한다.

| | 안 A — 모듈 폴더 아래 (권고) | 안 B — 모듈 문서 옆 |
|---|---|---|
| 모양 | `00. Core/00. Core.md`(폴더 줄) 아래 `Core API Reference`. GameFrameWork 는 지금 폴더에 `GameFrameWork API Reference` 를 더함 | `00. Core` 바로 아래 줄에 `00. Core API Reference`(같은 번호라 붙어 선다) |
| 좋은 점 | 모듈 문서를 누르면 펼쳐져 Reference 가 바로 보임. 다섯 모듈이 모두 GameFrameWork 와 같은 꼴 | 옮기는 문서가 없음. 바로 쓸 수 있음 |
| 대가 | 모듈 문서 4개(Core · Graphics · GUI · JGEditor)를 같은 이름 폴더로 옮김 — 그림 폴더 · 그리개(`Tools/Ops/wiki_svg/pages.js`) 경로도 같이 옮기고 다시 그림. 링크(`[[00. Core]]`)는 이름이라 그대로. 약 0.5일 | 맨 위 목록이 두 배로 길어짐. GameFrameWork 만 폴더 안에 들어가 모양이 다름 |

- 문서 분류: 머리말 `분류:` 를 네 가지(튜토리얼 · 하우투 · 레퍼런스 · 설계 설명) 가운데 하나로 둔다. API Reference = `레퍼런스`, 지금 모듈 문서(`00. Core` …) = `설계 설명`.
- 서로 잇기: Reference 첫 줄에 설계 설명 문서 링크, 설계 설명 문서 첫머리에 Reference 링크 한 줄. 구조도(JGEngine.md)의 모듈 표에서 Reference 로 잇는 칸은 Engine-36 이 이 이름을 쓴다.

## 3. 꼴

### 3-1. API Reference 문서

```
---
팀: JGEngine
분류: 레퍼런스
모듈: Core
갱신: 2026-10-24
---
# Core API Reference
<무엇 · include · 설계 설명 문서 링크 한 줄>
<영역 표 — 영역 · 항목 링크>
## <영역>
### <클래스 이름> (<한글>)
  개요 · 선언 · 매개변수/반환값 · 사용 예시 · 주의사항 · 관련
## 변경 이력
```

### 3-2. 항목

| 칸 | 쓰는 것 | 규칙 |
|---|---|---|
| 개요 | 역할 · 쓰는 상황 2~3줄 | 문서 주석 첫 두 문단과 같게 |
| 선언 | 헤더 위치(`Source/Runtime/<모듈>/…` 와 include 줄) + 공개 선언만 줄인 코드 상자 | 따라 include 할 헤더가 더 있으면 적는다(견본: `JG_LOG` 는 `Core.h` 필요) |
| 매개변수 / 반환값 | 표(이름 · 형 · 뜻). 함수가 많으면 함수 표 | 범위 밖 · 실패 때 돌려주는 값까지 |
| 사용 예시 | 예시 원본 `.cpp` 를 그대로, 확인 날짜, 실행 결과 표(실행되는 것만) | 원본과 한 글자도 다르지 않게 |
| 주의사항 | 4행 표 — 스레드 · 소유권/수명 · 호출 순서 · 에러 처리 | 해당 없으면 "제약 없음"(빈칸 금지) |
| 관련 | 같은 문서 · 다른 Reference 항목 링크 | |

### 3-3. 공개 헤더 문서 주석

```cpp
/// <역할 한두 줄>
///
/// <쓰는 상황 · 동작>
///
/// 스레드: …
/// 수명: …
/// 순서: …
/// 오류: …
/// @param InName  …
/// @returns …
```

- 선언 바로 위 `///`. Visual Studio 는 Quick Info 툴팁에 `///` 문단과 Doxygen 태그 `param` · `tparam` · `returns` · `deprecated` · `exception` 을 기본으로 보여 준다(Microsoft Learn「Configure Languages > C/C++ > IntelliSense options」). 그래서 이 다섯 태그만 쓰고, 주의 네 줄은 태그 없이 본문 줄로 둔다 — 게임 개발자는 엔진 헤더를 열지 않고 툴팁에서 같은 내용을 본다.
- 게임이 부르지 않는 public 멤버: `/// 엔진 내부용 — <누가 부르는지>.` 한 줄.
- 주석이 다루는 것은 "무엇을 · 언제 · 지키지 않으면 무슨 일"이다. 구현 줄의 주석은 지금처럼 "왜"만 쓴다. 공개 헤더에 있는 이력 주석(날짜 · TODO 번호 · "이전에는 …")은 그 헤더에 문서 주석을 달 때 변경 이력 · 커밋으로 옮기고, 손대지 않는 헤더는 그대로 둔다.

### 3-4. 모듈 README.md

`Source/Runtime/<모듈>/README.md`(JGEditor 는 `Source/Editor/JGEditor/README.md`) — 역할 · 의존 모듈 · 주요 클래스(헤더 · 하는 일 표) · 사용 예시 하나 · 문서(위키 이름). 저장소가 공개라 위키는 경로가 아니라 이름으로 가리킨다.

### 3-5. 변경 이력

Reference · 구조도 문서 맨 아래 표. 새 줄은 표 맨 위(최신이 위).

| 날짜 | 변경된 API | 변경 내용 | 게임 코드 영향 | 일감 |
|---|---|---|---|---|
| 2026-10-24 | HAutoConsoleCommand · HConsoleCommandArgs | 처음 씀 | 없음 | Engine-34 |

API 가 바뀐 작업의 보고에는 같은 세 가지(추가 · 변경 · 삭제된 API, 고친 문서, 게임 코드 수정이 필요한지)를 쓴다 — 유지 규칙은 Engine-37 이 팀 지침에 넣는다.

## 4. 견본 (Core — HAutoConsoleCommand · HConsoleCommandArgs)

| 파일 | 내용 |
|---|---|
| `Engine-34_견본/문서주석_견본.h` | 두 헤더 공개 부분에 문서 주석을 단 모습(본 헤더는 그대로) |
| `Engine-34_견본/Core API Reference_견본.md` | Reference 문서 틀 + 두 항목 + 변경 이력 |
| `Engine-34_견본/README_견본.md` | Core README 일부(역할 · 의존 · 주요 클래스 · 예시 · 문서) |
| `Engine-34_견본/ApiSample_ConsoleCommand.cpp` | 예시 원본 |
| `Engine-34_견본/compile_run_2026-10-24.txt` | 컴파일 · 실행 기록 |
| `Engine-34_견본/그림/*.html` | 보고함 그림 원본 |

컴파일 · 실행 확인 — 떼어 둔 작업 폴더 `Temp/wt/engine34`(main `c6dd99d`)의 JGConsole 에 예시를 넣어 빌드(DevelopEngine x64, 오류 0)하고 실행했다.

| 실행 | 결과 |
|---|---|
| `JGConsole.exe sample.greet World -count=2` | `Hello, World` 두 줄, 종료 코드 0 |
| `JGConsole.exe sample.greet` | 사용법 경고, 종료 코드 1 |
| `JGConsole.exe sample.greet World -count=abc` | 사용법 경고, 종료 코드 1 |
| `JGConsole.exe help sample.greet` · `help` | 사용법 줄 · 목록에 `sample.greet` |

견본에서 찾은 것: 첫 빌드는 `ConsoleCommand/ConsoleCommandGlobalSystem.h` 만 include 해서 `JG_LOG` 가 없다는 오류(C2065 · C3861)가 났다 — 이 헤더는 로그를 포함하지 않는다. 그래서 Reference 선언 칸에 "`JG_LOG` 를 쓰려면 `Core.h` 도"를 적었다. 예시를 실제로 빌드하지 않았다면 문서에 틀린 예시가 들어갔을 것이다(결정 2 의 까닭).

## 5. 예시 컴파일 확인 방법 (결정 2)

| | 안 1 — 엔진 빌드에 넣는다 (권고) | 안 2 — 문서를 쓸 때만 떼어 둔 폴더에서 |
|---|---|---|
| 하는 일 | 예시 원본을 시험 프로그램 `Source/Programs/JGApiSamples`(콘솔, 의존 Core · Graphics · GameFrameWork · GUI)로 묶어 매 빌드 컴파일. 헤드리스로 돌 수 있는 예시(Core · GameFrameWork)는 명령으로 실행까지 | `Temp/wt` 작업 폴더에 예시를 넣어 빌드 · 실행(이번 견본 방식) |
| 좋은 점 | API 가 바뀌면 빌드가 깨져, 같은 작업 안에서 문서를 고치라는 유지 규칙을 놓치지 않음 | 엔진 코드에 아무것도 들어가지 않음 |
| 대가 | 예시 파일이 공개 저장소에 들어감 · 처음 만들기 0.5일(Core 또는 DevOps) · 빌드에 수십 파일 | API 가 바뀐 뒤 예시가 깨져도 다음 확인 전까지 모름 · 확인마다 작업 폴더 빌드 |

## 6. 문서 주석 작업과 설계도 (결정 3)

공개 헤더에 주석을 다는 것도 코드 수정이라 팀 지침 7절(설계도 먼저)에 걸린다. 주석만 더하고 선언 · 동작은 바꾸지 않는다.

| | 안 가 — 이 계획 확인을 설계도 확인으로 (권고) | 안 나 — 모듈마다 설계도 |
|---|---|---|
| 하는 일 | 사용자가 이 계획(꼴 · 견본)을 확인하면 모듈 세션이 대상 헤더에 주석을 달고, 빌드 통과와 diff(주석 줄만)로 확인 | 모듈마다 대상 헤더 목록과 주석 견본을 보고함에 올려 확인받은 뒤 작업 |
| 대가 | 주석 내용은 Reference 확인 때 함께 봄 | 확인 5번, 달력 1~2일 늘어남 |

## 7. 일수 · 차례 · 담당

어림 근거: 항목(클래스 · 매크로 묶음) 하나에 문서 주석 + Reference + 예시 컴파일이 들고, 하루 10개 안팎. 항목 수는 Engine-35 의 쓰는 이름 · 상속 확장점 · "공개할 것 같은 것"을 클래스로 묶어 센 어림이다.

| 차례 | 일감 · 담당 | 범위(어림) | 일수 |
|---|---|---|---|
| 1 | Engine-38 Core | 약 35 항목 + README. 처음 하루로 꼴의 세부를 굳히고 다른 세션이 볼 수 있게 먼저 올림 | 4 |
| 1 | (결정 2 안 1이면) JGApiSamples 시험 프로그램 — Core 또는 DevOps | 프로그램 틀 · 빌드 · 명령 실행 | 0.5 |
| 2 | Engine-40 GameFrameWork | 약 60 항목(열거형은 묶음, 상속 확장점 11 · 공개할 것 포함) + README, GameHost README | 7 |
| 2 | Engine-39 Graphics | `IRawTexture` · `HVertex` + README | 0.5 |
| 2 | Engine-41 GUI | 메뉴 · 위젯 확장 지점 + README | 1 |
| 2 | Engine-42 Editor | README(공개 C++ API 없음, 확장은 GUI) | 0.5 |
| — | (결정 1 안 A면) 모듈 문서 4개를 폴더로 · 그림 경로 | 각 모듈 세션이 자기 문서를 첫 작업으로 | 0.5 |

- 합계 약 14일(세션 · 일). 차례 2는 Core 첫날 뒤 나란히 — 달력으로 약 7일(GameFrameWork 가 가장 김).
- 모듈 문서끼리의 링크(GameFrameWork 항목이 `PSharedPtr` 를 가리키는 것 등)는 Core Reference 의 항목 이름이 정해진 뒤 단다.

## 8. 남는 것 · 기대는 것

- Engine-35(DevOps) 목록 — 받아서 1 · 7 에 반영했다(19:0x). 그 문서 1절의 "PCH 가 `Core.h` 를 넣는다"는 실제로는 게임 `<이름>Defines.h` 가 넣는 것이라 DevOps 에 알렸다.
- 팀 지침 9절(문서 — API Reference · 구조도, 커밋 008cf58 · Engine-37) — 이 계획의 항목 꼴 · 문서 주석 · README · 문서 종류 · 변경 이력은 9절과 같다. 8절 위키 글 줄(주의 사항은 쓰지 않음)은 이제 설계 설명 문서에만 적용된다.
- Engine-36(구조도) — 모듈 표의 Reference 링크 이름은 결정 1 을 따른다.
- 작업 폴더 `Temp/wt/engine34` — 견본 확인용. 결정 2 가 안 2 면 다음 확인에 다시 쓰고, 아니면 지운다(엔진 총괄 판단).
