# GameModule TODO

갱신 2026-09-30. 현재 상태와 검증 절차는 `현황.md`. ID 규칙: `B`/`H`/`R`/`P` 는 `Files/게임모듈_사전작업_분석_2026-09-29.md` §3–§4 의 번호 그대로, `GM-` 는 이 문서에서 새로 붙인 것. 이 트랙은 이전에 별도 TODO 파일이 없었다(분석 문서 §7 "남은 것" + 인수인계 메모가 목록이었음).

---

## 다음에 할 일 Top 5

| 순서 | ID | 할 일 | 착수 조건 | 완료 기준 |
|---|---|---|---|---|
| 1 | GM-C1 | **커밋** (사용자) | 다른 세션의 MSBuild · cl 이 메인 트리에서 멈춘 상태 | 아래 체크리스트 파일 전부 한 커밋(또는 연속 커밋)에 포함. 커밋 뒤 `git status` 에 이 트랙 파일이 남지 않음. 커밋된 exe 로 `현황.md` §6 의 1–3 · 6 통과 |
| 2 | R8 | `JGConsole.exe modtest <Module>` — 헤드리스 모듈 로드 확인 명령 | 없음(DevConsole 1-3 완료로 `HAutoConsoleCommand` 사용 가능) | JGConsole 에 명령용 .cpp 1개 추가(`Main.cpp` 는 안 건드림). 엔진 Bin 에서 `modtest GameFrameWorks` 종료 코드 0, 없는 모듈은 1. 게임 Bin(`<Project>/Bin/DevelopEngine`)에서 `modtest MyGame` → `MyGame entry actor entered world` 로그 + 종료 코드 0 + `[error]` 0. `console.selftest` · `gmtest` 회귀 통과 |
| 3 | GM-H1 | 게임 단독 실행 호스트 결정 | **사용자 결정** | 결정이 분석 문서 §6 에 D8 로 기록되고 담당 파일이 정해짐 |
| 4 | R5 + B10 | 게임 실행에서 `<Name>Editor` 제외 | GM-H1 | 게임 실행 호스트에서 `<Name>Editor.dll` 로드 · 연결 0. ConfirmGame · ReleaseGame 솔루션 구성에 Editor 모듈 프로젝트 없음. 에디터 환경(JGEditor)은 지금과 동일, 엔진 단독 산출물 동일 |
| 5 | R7 | 게임 `Content` 마운트 | 게임이 에셋을 쓰기 시작할 때(사용자가 시점 결정) | `<Project>/Content` 의 에셋을 `/JGGame/` 경로로 로드. 엔진 단독 모드 · 엔진 Content 로드는 변화 0 |

---

## 미완료 항목

| ID | 항목 | 우선순위 | 상태 | 선행조건 | 완료 기준 | 비고 |
|---|---|---|---|---|---|---|
| GM-C1 | 커밋 | 높음 | 대기(사용자) | — | Top 1 | 체크리스트는 아래. `CoreSystem.cpp` · `Module.cpp` 는 DevConsole 변경과 한 파일 |
| R8 | `modtest <Module>` 명령 | 중 | 미착수 | — | Top 2 | `Document/Memory/Etc/Files/DevConsole_TODO.md` 표에 행이 있음. JGConsole 명령은 명령마다 .cpp 하나(여러 트랙이 `Main.cpp` 를 동시에 고치지 않게) |
| GM-H1 | 게임 단독 실행 호스트 | 중 | 사용자 결정 대기 | — | Top 3 | 창 · Graphics 를 누가 여나. 후보(예): 런처 인자/구성으로 게임 모드, 별도 런치 모듈, 프로젝트 템플릿에 호스트 모듈 추가 |
| R5 | 게임 실행에서 `<Name>Editor` 제외(런타임) | 중 | 미착수 | GM-H1 | Top 4 | `codeGen()` 이 `module_system_info.json` 의 모든 SharedLib 를 시작 시 로드(`ObjectGlobalSystem.cpp:253-255`) → 구성별 로드 목록이 필요 |
| B10 | 모듈 종류(Runtime/Editor) + ConfirmGame · ReleaseGame 에서 Editor 모듈 제외(BuildTool) | 중 | 미착수 | GM-H1 | Top 4 | `.jgproject` 의 `GameModules` / `EditorModules` 를 BuildTool 이 읽으면 module.json 새 필드 없이 가능 |
| R7 | 게임 Content 마운트 | 낮음(시점 미정) | 미착수 | 사용자 | Top 5 | `GameContentDirectory()` 빈 스텁(`FileHelper.cpp:623`), `/JGGame/` 토큰은 critical(`AssetPath.cpp`), `AssetDatabase` 는 엔진 Content 만 |
| GM-B1 | `ModuleInfo.h:65-66` WriteJson 복붙 — `ModuleInfoTemplate.json` 의 `ModuleName` · `ModulePath` 값이 다른 필드 값 | 낮음 | 미착수 | — | 템플릿 json 스키마가 맞음 | 09-16 Programs P2. 게임 템플릿은 이 파일을 쓰지 않음. 엔진 PreBuild 마다 재생성 |
| GM-B2 | 죽은 파일 정리 — `Source/Runtime/Asset/Core.module.json`(폴더명과 달라 무시됨), `Source/Dummy` | 낮음 | 미착수 | — | 삭제 뒤 PreBuild 산출물 동일 | 09-16 Programs P3 |
| GM-B3 | 게임 헤더의 생성 cpp `#include` 가 절대경로 | 낮음 | 미착수 | — | 프로젝트 `Source` 기준 상대경로 | 빌드는 된다. 프로젝트 폴더를 옮기면 배치 2 재실행으로 해결됨 |
| P4 | 프로젝트 경로 ASCII · 공백 검사 | 낮음 | 규칙만(가이드 §0) | — | 필요해지면 배치 1 에서 검사 | 지금 문제 없음 |

---

## 완료 이력

| ID | 항목 | 완료일 | 검증 근거 |
|---|---|---|---|
| G (09-16 블로커) | HeaderTool `Arguments.h` `ReadJson` const override — 인자 json 이 실제로 읽힘 | 2026-09-16 | `Files/2026-09-16_Programs_현황파악.md` |
| — | 09-16 구조 분석(블로커 A–I) · Programs 현황(P1–P3) | 2026-09-16 | `Files/엔진_게임분리_분석_2026-09-16.md`, `Files/Programs_현황분석_2026-09-16.md` |
| — | 옛 툴 + 외부 프로젝트 재현(Step 4 CRT 대화상자 멈춤, 게임 헤더 skip) | 2026-09-29 | `Files/2026-09-29_gameproject_repro_results.txt`, `Files/tools/gameproject_repro/` |
| D1–D7 | 결정(에디터 환경 · JGEditor 호스트 · 엔진 소스 포함 · 코드젠 2곳 · `<Name>` 이름 · ConfirmConfig · EngineRoot → 환경변수) | 2026-09-29 | 분석 §6 |
| B1–B9 | BuildTool: 실패 처리 · `-project=` · 루트 접두 · CodeGenPath · 생성물 프로젝트로 · 템플릿 변수 · PCH 바뀔 때만 · 3rd party 복사 · module_system_info | 2026-09-29 | `Files/2026-09-29_게임프로젝트_구현검증.txt` §1 · §3 |
| H1–H6 | HeaderTool: 실패 처리 · `-project=` · `*.module.json` 직접 스캔(09-16 P1 순환 의존 해소) · 게임 코드젠 + 등록 파일 · 프로젝트 CodeGen · 바뀐 파일만 | 2026-09-29 | 구현검증 §1 · §3 |
| R1 · R1a · R2 | 루트 2개(`resolveRootDirectories` · `HCoreSystemGlobalValues`) · `.jgproject`(`HProjectDescriptor`) · 런처 LaunchModule = JGEditor | 2026-09-29 | 구현검증 §2 · §4 |
| R3 | JGEditor 호스트(모듈 6개 · `Dev/DevAI` · 씬 뷰 · 프로젝트 모듈 연결/역순 해제) · JGDev_Graphics 삭제 — GUI_TODO 3-6 | 2026-09-30 | 구현검증 §2 · §5, `Files/2026-09-29_jgeditor_engine_capture.png` |
| R4 | 엔진 `Source/Runtime/Game` 삭제 — D5 로 불필요, **제외** | 2026-09-29 | 분석 §4-1 R4 |
| R6 | DevelopGame 의 게임 모듈 = ConfirmConfig (규칙) | 2026-09-29 | 템플릿 `*.module.json`, 가이드 §4 |
| — | 템플릿 13 파일 · `CreateGameProject.bat` · `GenerateGameProjectFiles.bat` · `-newproject=`(ProjectCreator) · 가이드 | 2026-09-29 | 구현검증 §3, `Files/게임프로젝트_생성가이드.md` |
| — | `PString::GetRawWString` 폴백(게임 프로젝트 창 제목 `(null)`) | 2026-09-29 | 구현검증 §4 |
| — | 엔진 단독 A/B 회귀(산출물 동일, 종료 코드 139 → 0, 2회차 컴파일 0) | 2026-09-29 | 구현검증 §1 |
| — | 게임 E2E: 생성 → 솔루션 2회 → `MyGame.sln` 17 프로젝트 오류 0 → 실행(두 모듈 로드, 종료 코드 0, live blocks 0) | 2026-09-29 | 구현검증 §3 · §4, `Files/2026-09-29_jgeditor_mygame_capture.png` |
| — | 메인 트리 반영(3-way 병합 충돌 0, PreBuild 0, 빌드 오류 0, 런처 0, 메인 배치 생성 0) | 2026-09-30 | 구현검증 §5 |
| P1 · P2 · P5 | HEAD `1308fd5` 위에서 worktree 격리 작업 · GameFrameWorks API 이름 경계 확인 | 2026-09-29 | 분석 §4-2 |

---

## 커밋 체크리스트 (GM-C1)

이 트랙 변경을 커밋할 때 **반드시 함께** 들어가야 하는 것. 하나라도 빠지면 배치가 옛 exe 를 부르거나 템플릿을 못 찾는다.

| 구분 | 파일 | 비고 |
|---|---|---|
| Core (수정) | `Source/Runtime/Core/CoreSystem.h`, `CoreSystem.cpp`, `FileIO/FileHelper.h`, `FileIO/FileHelper.cpp`, `Misc/Module.cpp`, `String/String.cpp` | `CoreSystem.cpp` · `Module.cpp` 에 DevConsole 트랙 변경(콘솔 명령 등록 훅) 혼재 — 파일 단위 분리 불가, 함께 커밋 |
| Core (신규) | `Source/Runtime/Core/FileIO/ProjectDescriptor.h`, `ProjectDescriptor.cpp` | |
| JGBuildTool | `Source/Programs/JGBuildTool/Main.cpp`, `Class/BuildTool.h`, `Class/BuildTool.cpp`, `Class/ModuleInfo.h`, `Class/ProjectCreator.h`(신규), `Class/ProjectCreator.cpp`(신규), `Template/BuildTemplate.lua`, `jgengine.lua`(사본, 재생성물) | |
| JGHeaderTool | `Source/Programs/JGHeaderTool/Main.cpp`, `Class/HeaderTool.h`, `Class/HeaderTool.cpp` | |
| 호스트 | `Source/Programs/JGLauncher/Main.cpp`, `Source/Editor/JGEditor/JGEditor.h`, `JGEditor.cpp`, `JGEditor.module.json` | |
| 삭제 | `Source/Editor/JGDev_Graphics/JGDev_Graphics.cpp`, `.h`, `.module.json`, `JGDevGraphicsTest.cpp`, `.h`; `Bin/DevelopEngine/JGDev_Graphics.dll`, `.exp`, `.lib` | `git rm` 상태로 이미 삭제됨 |
| 배치 (신규) | `Build/BatchFiles/CreateGameProject.bat`, `Build/BatchFiles/GenerateGameProjectFiles.bat` | CRLF 유지 |
| 템플릿 (신규, 13) | `Build/Templates/GameProject/**` — `{PROJECT_NAME}.jgproject`, `GenerateProjectFiles.bat`, `.gitignore`, `Source/{PROJECT_NAME}/*`(6), `Source/{PROJECT_NAME}Editor/*`(4) | 파일 · 폴더 이름에 `{PROJECT_NAME}` 토큰이 그대로 있어야 한다. 폴더는 `Source/` 밖 |
| **툴 exe (필수)** | `Bin/DevelopEngine/JGBuildTool.exe`, `Bin/DevelopEngine/JGHeaderTool.exe` | 배치가 이 exe 를 부른다. 옛 exe 는 `-project=` · `-newproject=` 를 모른다. 커밋 직전 현재 소스로 빌드된 것인지 확인(다른 세션 빌드가 계속 갱신함) |
| 생성물 (재생성) | `JGEngine.sln`, `jgengine.lua` | JGDev_Graphics 가 빠진 15 프로젝트 |
| 문서 | `Document/Memory/GameModule/현황.md`, `TODO.md`, `Files/**`(이동 후 경로: 분석 · 가이드 · 구현검증 · 캡처 2장 · 인수인계 · 재현 결과 · `Files/tools/gameproject_repro/`) | 다른 트랙 문서 수정분(GUI_TODO 3-6, Memory_TODO 3-2, DevConsole_TODO R8 행)은 그 트랙 커밋과 겹칠 수 있음 |

넣지 않는 것(다른 트랙): `Source/Programs/JGConsole/Main.cpp` · `NetCommands.cpp` · `ConsoleCommandSelfTest.cpp`(DevConsole · Network), `Source/Editor/DevConsole/**`(DevConsole), `Build/BatchFiles/jg_log.txt` · `Bin/DevelopEngine/jg_log.txt`(로그). `Bin/DevelopEngine` 의 다른 dll · lib 는 저장소 관례(추적 중)대로.

커밋 뒤 확인: 다른 폴더에 클론(또는 `Temp/` 를 지운 상태) → 엔진 `GenerateProjectFiles.bat` → 빌드 → `현황.md` §6 의 1–3 · 6. 첫 배치 2 실행 전 엔진 코드젠이 없으면 `engine code generation is missing` 으로 종료 코드 1 이 나와야 정상.
