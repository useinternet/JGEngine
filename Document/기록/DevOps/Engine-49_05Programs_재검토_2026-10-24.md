# 위키 05. Programs 재검토 — Engine-49 (2026-10-24 18:59, DevOps)

대상: `C:/obsidian/ProjectJG/02. 위키/JGEngine/05. Programs.md`(17:29 판, Document Writer 가 다듬은 글) · 그림 9장(`Programs_00 ~ 08`). 기준 코드: 메인 트리 `86eb96e`. 문서는 고치지 않았다(사용자 Q-004 답 · 상위 Engine-36).

## 결론

| 볼 것 | 판단 | 한 줄 |
|---|---|---|
| ① 코드와 맞는지 | 거의 맞음 — 고칠 곳 6 | 크게 틀린 흐름은 없다. "모든 모듈"처럼 예외를 빠뜨린 문장, 자체 검사 명령의 별칭 구분, "창 없이" 같은 표현이 어긋난다 |
| ② 게임 프로젝트가 이해하기 쉬운지 | 보통 — 고칠 곳 4 | 게임 프로젝트가 할 일과 엔진이 알아서 하는 일이 한데 섞였고, 게임과 상관없는 팀 도구(MainTree · Tools)가 절 두 개를 차지한다 |
| ③ API Reference 로 바로 갈 수 있는지 | 길이 없음 — 고칠 곳 3 | 게임이 쓰는 것은 C++ API 보다 명령줄 인자 · 명령 · 설명 파일(`.jgproject` · `module.json`)인데, 이를 모은 레퍼런스가 없다 |
| ④ 설명과 절차가 섞였는지 | 섞임 | 구조 설명에 명령줄 · "다시 실행한다" 같은 절차 문장과 인자 · 명령 표(레퍼런스)가 끼어 있다. 반대로 구조도(`JGEngine.md:114 · 135`)는 "절차는 05. Programs 에서" 라고 가리키는데 05 에는 차례대로 따라 할 절차가 없다 |

권고: 05. Programs 는 설계 설명으로 두고, 레퍼런스 1개 · 하우투 3개로 나눈다(4절 표, 약 1.5일). MainTree · Tools 절은 팀 운영 문서로 옮긴다.

## 1. 코드와 맞는지

| 어디를 | 지금 글 | 코드 | 어떻게 | 왜 |
|---|---|---|---|---|
| 0-1 마지막 문단 | 빌드 템플릿이 C++20 · 동적 CRT · 공용 PCH 를 "모든 모듈에 적용한다" | 단독 모듈(`"Standalone": true`, JGGameBootstrap)은 PCH · Core 없이, CRT 를 정적으로 링크한다(`BuildTemplate.lua:89-90`) | "단독 실행 파일(JGGameBootstrap)을 뺀 모든 모듈" | 6절의 "루트 실행 파일은 엔진을 링크하지 않는다"와 맞지 않는다 |
| 1-1 첫 문단 끝 | "소스 파일, 모듈 설명 파일, 리플렉션 표시가 바뀌면 다시 실행한다" | 소스는 더하기 · 지우기 · 옮기기 때만 프로젝트 파일이 바뀐다(내용만 고치면 빌드만) | "소스 파일을 더하거나 지우거나 옮길 때, 모듈 설명 파일 · 리플렉션 표시를 바꿀 때" — 이 문장은 하우투로 옮긴다(4절) | "소스 파일이 바뀌면"은 고칠 때마다 다시 생성하라는 뜻으로 읽힌다 |
| 1-1 넷째 점 | `jgengine.lua` 와 `JGEngine.sln` 이 저장소에 포함된 생성물 | `jgengine.bat` 도 저장소에 있고, 생성할 때 premake 의 절대 경로가 들어간다(`git ls-files jgengine.bat`) | 셋으로 적는다 | 작업 폴더(worktree)에서 다시 생성하면 `jgengine.bat` 만 경로가 바뀌어 보인다 |
| 4 첫 문단 | "JGConsole은 창 없이 콘솔 명령 하나를 실행하고 종료하는 명령줄 도구다" | `-graphics` 면 작은 창을 만들고 메인 루프를 돈다(4-1 · `JGConsole/Main.cpp:15-16`) | "기본은 창 없이" | 4-1 과 앞뒤가 다르다 |
| 4-2 GameFrameWork 행 | `gmtest` · `gfw.selftest` · `gameui.selftest` · `audio.selftest` · `datatable.selftest` · `datatable.validate` · `cookcheck.selftest` · `net.test` 를 한 줄에 "자체 검사" | `gfw.selftest` 가 묶음이고 `gmtest` · `gameui.selftest` · `datatable.selftest` · `cookcheck.selftest` · `net.test` · `audio.selftest` 6개는 같은 시험의 옛 이름 별칭이다(`GameFrameWorkSelfTest.cpp:14 · 178`). `datatable.validate` 는 자체 검사가 아니라 데이터 테이블 파일 검증 도구다(`DataTableSelfTest.cpp:15`) | 세 줄로: 묶음 `gfw.selftest` / 옛 이름 별칭 6개 / `datatable.validate`(게임 데이터 검증) | 별칭과 묶음을 구분해야 무엇을 돌릴지 안다. `datatable.validate` 는 게임 데이터에도 쓰는 도구다 |
| 4-2 표 | Core 명령 `help` 가 없다 | `help` 는 Core 가 등록한다(`ConsoleCommandGlobalSystem.cpp:59`), 명령 없이 실행하면 `help` 가 돈다 | `help` 한 줄 더함 | 명령 목록을 보는 첫 명령이다 |

함께 볼 것(지금은 맞지만 곧 바뀜): JGLauncher 는 `RequestExit` 종료 코드를 버리고 늘 0 을 돌려준다(`JGLauncher/Main.cpp:43`). Engine-43 에서 고치면 3절에 종료 코드 한 줄을 더한다.

옆에서 본 것(05 밖): 구조도 `JGEngine.md:133` "에디터 모듈과 GUI는 패키지에 포함되지 않는다" — GUI 는 런타임 모듈이 의존하면 경고와 함께 패키지에 들어간다(`BuildTool.cpp:14-15` 주석). Engine-36 담당에게 넘긴다.

## 2. 게임 프로젝트가 이해하기 쉬운지

| 어디를 | 지금 | 어떻게 | 왜 |
|---|---|---|---|
| 0절 | 프로그램 여섯 개를 만든 쪽(엔진) 눈으로 나눈다 | 0절 끝에 "게임 프로젝트가 만나는 것" 표 한 장 — 할 일(만들기 · 솔루션 다시 생성 · 에디터 실행 · 게임만 실행 · 게임 명령 시험 · 패키지)마다 쓰는 프로그램과 하우투 링크 | 게임 프로젝트는 "무엇을 하려면 무엇을 쓰나"부터 찾는다 |
| 절 제목 1 · 2 · 7 | `PBuildTool` · `PHeaderTool` · `PProjectCreator` — 내부 클래스 이름 | `JGBuildTool (프로젝트 파일 생성)` · `JGHeaderTool (리플렉션 코드 생성)` · `CreateGameProject (게임 프로젝트)` — 클래스 이름은 본문에 | 게임 프로젝트가 보고 쓰는 것은 실행 파일 · 배치 이름이다. 클래스는 게임 코드에서 쓰지 않는다(Engine-35) |
| 8 · 9절 | MainTree(세션 잠금) · Tools(엔진 팀 검증 도구) | 팀 운영 문서(「JGEngine 팀 구성 및 운영」)로 옮기고 05 에는 한 줄 링크 | 둘 다 엔진 팀 세션이 쓰는 도구라 게임 프로젝트와 상관없다. 05 를 게임 프로젝트 눈높이로 줄인다 |
| 0-1 · 1-2 · 4-1 | "실행 모듈" · "런타임 모듈 목록" · "PCH" · "SharedLib" 를 처음 나올 때 풀지 않는다 | 처음 나올 때 한 구절로 — 실행 모듈 = `GCoreSystem` 이 맨 먼저 연결하는 모듈(JGEditor · GameHost), 런타임 모듈 = 에디터 없이 게임을 돌릴 때 필요한 DLL | 이 낱말들이 Programs 의 흐름 전부를 잇는다. 게임 쪽이 처음 보는 낱말이다 |

## 3. API Reference 로 바로 갈 수 있는지

| 어디를 | 지금 | 어떻게 | 왜 |
|---|---|---|---|
| 3 · 4 · 5 · 6절 | 인자 · 명령 · 옵션 · 종료 코드가 일부만 본문과 표에 흩어져 있다. `cook.run` 옵션(`-config` · `-output` · `-skipbuild` · `-nosmoke` · `-smokeframes` · `-launchmodule`), JGGameLaunch 옵션(`-smoke=` · `-log` · `-logfile=` · `-launchmodule=`)과 종료 코드(0 · 2 · 3 · 4)는 빠졌다 | 「Programs Reference」(분류 레퍼런스)를 새로 두고, 절마다 끝에 "→ Programs Reference: JGConsole 옵션" 처럼 한 줄 링크 | 게임 프로젝트가 Programs 에서 쓰는 "API" 는 명령줄과 설명 파일이다. Engine-34 계획의 레퍼런스 · 설계 설명 나눔(계획 2절 53-54줄)과 같은 꼴이 된다 |
| 0-1 · 7절 | `module.json` · `.jgproject` · `JGPackage.json` 의 칸을 말로만 일부 적는다 | 같은 레퍼런스에 세 파일의 칸 표(이름 · 뜻 · 기본값 · 읽는 곳) | 게임 프로젝트가 직접 고치는 파일이다(특히 `Package.Window` · `ExcludeContent` · 구성별 `Config`) |
| 7절 끝 | 템플릿 게임 모듈이 하는 일을 말로 적는다 | `IModuleInterface` · `JG_MODULE_IMPL` · `JGGameInstance::SetEntryClass` · `LoadWorld` · `JGGameEntryActor` 를 Core · GameFrameWork API Reference 항목으로 잇는다(항목 이름이 정해진 뒤) | 새 게임이 처음 만나는 C++ API 가 바로 이것이다(Engine-35 1절) |

## 4. 설명 · 절차 섞임과 나눌 안

05 안의 절차 · 레퍼런스 성격 문장:

| 어디 | 문장 | 성격 |
|---|---|---|
| 1-1 | "… 바뀌면 다시 실행한다" | 하우투 |
| 3 | 인자 표(`-host` · `-windowed` …) | 레퍼런스 |
| 4-2 | 명령 표 | 레퍼런스 |
| 5 | "`JGConsole.exe -runCook -project=<프로젝트>`로 실행한다" | 하우투 · 레퍼런스 |
| 7-1 · 7 끝 | "엔진 솔루션을 먼저 다시 생성하고, 그다음 게임 솔루션" | 하우투 |
| 8 · 8-1 | MainTree 동사 · `-name=` · `-wait` | 하우투(팀 운영) |
| 9-1 | "`MainTree.cmd exec` 로 실행한다" · "`build.bat` 으로 만든다" | 하우투(팀 운영) |

나눌 안:

| 문서 | 분류 | 담는 것 | 옮겨 오는 곳 | 대략 |
|---|---|---|---|---|
| 05. Programs | 설계 설명 | 프로그램 구성 · 프로젝트 파일 생성 흐름 · 런타임 모듈 목록 · JGConsole · 쿠킹 · 패키지 실행 · 게임 프로젝트 구조(지금 0~7절의 그림 7장). 절차 문장 · 인자 · 명령 표는 빼고 링크 | 지금 문서 | 0.25일 |
| Programs Reference | 레퍼런스 | 프로그램별 명령줄 인자 · 옵션 · 종료 코드, JGConsole 명령 목록, `module.json` · `.jgproject` · `JGPackage.json` 칸, 출력 · 로그 자리 | 05 의 3 · 4-2절 표 + 코드 머리 주석(`JGConsole/Main.cpp` · `CookCommand.cpp:16-25` · `JGGameLaunch.cpp:14-21`) | 0.5일 |
| 하우투 — 게임 프로젝트 만들고 실행하기 | 하우투 | 만들기 → 솔루션 → 빌드 → 에디터 · 게임만 실행 → 게임 명령 시험, 다시 생성할 때 | 옛 「JGEngine 빌드와 실행」 7절 1~4 단계 · 05 의 7절 끝 | 0.25일 |
| 하우투 — 게임 패키지 만들기 | 하우투 | `-runCook` 실행 → 결과 폴더 → 옮겨 실행 · 로그 보기, Content 빼기 | 옛 「JGEngine 빌드와 실행」 7절 5~6 단계 · 05 의 5절 | 0.25일 |
| 하우투 — 엔진 처음 빌드하기 | 하우투 | 새 클론 → `GenerateProjectFiles.bat`(도구 자동 빌드) → 솔루션 빌드 → 에디터 실행, 다시 생성할 때 | 옛 「JGEngine 빌드와 실행」 2 · 5 · 6절 · 05 의 1-1 | 0.25일 |
| 팀 운영 문서에 더함 | (그 문서 분류) | MainTree 도우미 · Tools 폴더 · crashwalk | 05 의 8 · 9절 그림 2장 그대로 | 엔진 총괄 |

그 밖에 고칠 것: 머리말 `분류: 모듈` → Engine-34 계획대로 `설계 설명`. 구조도 `JGEngine.md:114 · 135` 의 "절차는 05. Programs 에서" 링크를 위 하우투 둘로 바꾼다.

## 5. 그림 9장

| 그림 | 코드와 | 고칠 것 |
|---|---|---|
| Programs_00 프로그램 구성 | 맞음 — 연결 모듈(JGConsole → GameFrameWork, JGLauncher → JGEditor · `-game` GameHost, JGGameLaunch → GameHost) | 없음 |
| Programs_01 프로젝트 파일 생성 | 맞음 | 없음(`jgengine.bat` 은 그림에 없어도 됨) |
| Programs_02 런타임 모듈 목록 | 맞음 — 뿌리 GameHost · GameModules, `EditorDependencies` 쿠킹 실패, GUI 경고 | 없음 |
| Programs_03 JGConsole 실행 | 맞음 — `ConnectModule("GameFrameWork")`, `-graphics` → `ConnectModule("Graphics")` | 없음 |
| Programs_04 쿠킹 단계 | 맞음 — 10단계 · staging · `-Symbols` | 없음 |
| Programs_05 패키지 실행 | 맞음 | 없음 |
| Programs_06 게임 프로젝트 | 맞음 — 템플릿 의존 Core · Graphics · GameFrameWork | 제목을 바꾸면 alt 첫머리도 맞춘다(`PProjectCreator` 상자는 그대로 둬도 됨) |
| Programs_07 메인 트리 도우미 | 맞음 | 팀 운영 문서로 옮길 때 문서 키만 바꿈 |
| Programs_08 Tools 구성 | 맞음 — 분야 폴더 이름 GameFrameWork | 위와 같음 |

## 6. 대조한 코드

`Source/Programs/JGBuildTool/Class/BuildTool.cpp`(Run · writeRuntimeModuleInfo · collectionModuleInfosInternal · 상수 14-15줄), `Template/BuildTemplate.lua`(구성 · SetStandaloneProjectConfig 89-90줄), `buildtool_arguments.json`, `Class/ProjectCreator.cpp`(이름 규칙), `JGHeaderTool/Class/HeaderTool.cpp`(collectSharedLibModules · writeIfChanged), `JGLauncher/Main.cpp`(43줄), `JGConsole/Main.cpp`(옵션 · 엔진 모듈 · `-graphics` 루프), `JGConsole/Cook/CookCommand.cpp`(16-25줄 · step 10개), `JGGameBootstrap/Main.cpp`, `JGGameLaunch/JGGameLaunch.cpp`(14-21줄), `Source/Runtime/Core/CoreDefines.h`(`JG_WITH_EDITOR`), `Core/ConsoleCommand/ConsoleCommandGlobalSystem.cpp`(59줄), `GameFrameWork/SelfTest/GameFrameWorkSelfTest.cpp`(14 · 172 · 178줄), `GameFrameWork/SelfTest/DataTableSelfTest.cpp`(15줄), `Build/BatchFiles/*`, `Build/Templates/GameProject/`, `Tools/README.md`, 구조도 `JGEngine.md`(114 · 133 · 135줄), `Document/기록/Core/Engine-34_APIReference_계획_2026-10-24.md`(2절).

## 변경 이력

| 날짜 | 무엇 |
|---|---|
| 2026-10-24 | 처음 씀(Engine-49) — 기준 86eb96e |
