# GameModule TODO

갱신 2026-09-30. 현재 상태와 검증 절차는 `현황.md`. ID 규칙: `B`/`H`/`R`/`P` 는 `Files/게임모듈_사전작업_분석_2026-09-29.md` §3–§4 의 번호 그대로, `GM-` 는 이 문서에서 새로 붙인 것. 이 트랙은 이전에 별도 TODO 파일이 없었다(분석 문서 §7 "남은 것" + 인수인계 메모가 목록이었음).

---

## 상태: 완료 (커밋 대기 → 휴면)

2026-09-30 사용자 완료 처리. 그 뒤 같은 날 밤 사용자가 "JGEngine 의 Game 모듈은 필요없을거같아" → **R4(엔진 `Source/Runtime/Game` 삭제) 재개 → 22:13 메인 트리 적용 완료(미커밋)**. worktree 검증 뒤, 첫 메인 트리 적용은 에이전트 자동 모드가 PreBuild 를 거부해 되돌렸고, 사용자 승인("a로 ㄱㄱ")으로 다시 적용했다. 증적 `Files/2026-09-30_R4_Game모듈_삭제_검증.txt` §12. 남은 것은 커밋(사용자)뿐이고, 아래 "보류" 표는 필요해질 때만 다시 연다.

| 순서 | ID | 할 일 | 착수 조건 | 완료 기준 |
|---|---|---|---|---|
| 1 | GM-C2 | **커밋** (사용자) — R7 · R8 · GM-A1 · 백로그 수정 · R4 | 다른 세션의 MSBuild · cl 이 메인 트리에서 멈춘 상태 | 아래 "커밋 체크리스트 (GM-C2)" 파일 전부 포함. 커밋 뒤 `git status` 에 이 트랙 파일이 남지 않음 |

R4 적용 뒤 알림: 메인 트리 엔진을 `EngineRoot` 로 쓰는 게임 프로젝트(다른 세션의 `GfwView` 등)는 각자 `GenerateProjectFiles.bat` 를 다시 돌려야 한다(안 하면 옛 `Game` 프로젝트가 `C1083`).

---

## 보류 (사용자 결정 2026-09-30: 필요할 때 진행)

| ID | 항목 | 다시 열 조건 | 완료 기준 | 비고 |
|---|---|---|---|---|
| GM-H1 | 게임 단독 실행 호스트 — 창 · Graphics 를 누가 여나 | 게임을 에디터 없이 실행해야 할 때 | 결정이 분석 문서 §6 에 D8 로 기록되고 담당 파일이 정해짐 | 후보(예): 런처 인자/구성으로 게임 모드, 별도 런치 모듈, 프로젝트 템플릿에 호스트 모듈 추가. 헤드리스 확인은 지금도 `JGConsole module.test <Name>` 으로 된다 |
| R5 | 게임 실행에서 `<Name>Editor` 제외(런타임) | GM-H1 | 게임 실행 호스트에서 `<Name>Editor.dll` 로드 · 연결 0. 에디터 환경은 지금과 동일 | `codeGen()` 이 `module_system_info.json` 의 모든 SharedLib 를 시작 시 로드(`ObjectGlobalSystem.cpp:253-255`) → 구성별 로드 목록이 필요 |
| B10 | 모듈 종류(Runtime/Editor) + ConfirmGame · ReleaseGame 에서 Editor 모듈 제외(BuildTool) | GM-H1 | ConfirmGame · ReleaseGame 솔루션 구성에 Editor 모듈 프로젝트 없음, 엔진 단독 산출물 동일 | `.jgproject` 의 `GameModules` / `EditorModules` 를 BuildTool 이 읽으면 module.json 새 필드 없이 가능 |

## 닫음 (하지 않음)

| ID | 항목 | 이유 | 근거 |
|---|---|---|---|
| GM-B3 | 게임 생성 cpp 의 `#include` 가 절대경로 | 생성물은 `<Project>/Temp/CodeGen`(git 미추적, PC 마다 배치 2 가 다시 만듦). 프로젝트를 옮기면 sln · vcxproj 때문에도 배치 2 재실행이 필요. 절대경로는 이름이 같은 엔진 헤더를 잘못 집는 문제도 피한다 | `Files/2026-09-30_GameModule_마무리_검증.txt` §8 |
| GM-B2 (일부) | `Source/Dummy` 삭제 | 죽은 폴더가 아니다 — 툴의 기본 `UserWorkDirectory`(`JGBuildTool/Class/Arguments.h:61`, `buildtool_arguments.json:10`, `JGHeaderTool/Class/Arguments.h:22`, `headertool_arguments.json:9`) | 마무리 검증 §1 |

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
| R4 | 엔진 `Source/Runtime/Game`(더미, `int dummy()` 뿐) 삭제 — 09-29 에는 D5 로 불필요해 제외, 09-30 밤 사용자 결정으로 재개. `Bin/DevelopEngine/Game.*` 삭제 · 생성물 재생성(14 프로젝트) + `ProjectCreator.cpp` 에 `Game` 프로젝트 이름 예약(폴더 검사가 사라져도 `JGGameEntryActor` 충돌을 배치 1 에서 막음) | 2026-09-30 22:13 (미커밋 → GM-C2) | `Files/2026-09-30_R4_Game모듈_삭제_검증.txt` — worktree(엔진 · `MyGame` 빌드 · 실행) + 메인 트리 §12(PreBuild 차이 `Game` 3곳뿐, 빌드 오류 0, `Game` 이름 종료 코드 1, JGConsole · 런처 60초 종료 0 · error 0) |
| R6 | DevelopGame 의 게임 모듈 = ConfirmConfig (규칙) | 2026-09-29 | 템플릿 `*.module.json`, 가이드 §4 |
| — | 템플릿 13 파일 · `CreateGameProject.bat` · `GenerateGameProjectFiles.bat` · `-newproject=`(ProjectCreator) · 가이드 | 2026-09-29 | 구현검증 §3, `Files/게임프로젝트_생성가이드.md` |
| — | `PString::GetRawWString` 폴백(게임 프로젝트 창 제목 `(null)`) | 2026-09-29 | 구현검증 §4 |
| — | 엔진 단독 A/B 회귀(산출물 동일, 종료 코드 139 → 0, 2회차 컴파일 0) | 2026-09-29 | 구현검증 §1 |
| — | 게임 E2E: 생성 → 솔루션 2회 → `MyGame.sln` 17 프로젝트 오류 0 → 실행(두 모듈 로드, 종료 코드 0, live blocks 0) | 2026-09-29 | 구현검증 §3 · §4, `Files/2026-09-29_jgeditor_mygame_capture.png` |
| — | 메인 트리 반영(3-way 병합 충돌 0, PreBuild 0, 빌드 오류 0, 런처 0, 메인 배치 생성 0) | 2026-09-30 | 구현검증 §5 |
| P1 · P2 · P5 | HEAD `1308fd5` 위에서 worktree 격리 작업 · GameFrameWorks API 이름 경계 확인 | 2026-09-29 | 분석 §4-2 |
| GM-C1 | 커밋(`4c8ac73`, 사용자, 체크리스트 전부 포함 확인) + 커밋 뒤 회귀(`C:\Develop\JGEngine`: PreBuild 2회 · 엔진 빌드 · 엔진 단독 실행 · 생성 · 솔루션 2회 · 게임 빌드 · 실행 — 전부 종료 코드 0, 오류 0, 엔진 2회차 컴파일 0) | 2026-09-30 | `Files/2026-09-30_R7_게임Content_검증.txt` §2–§5 |
| GM-H1 · R5 · B10 | 보류 결정(사용자: 게임 단독 실행은 필요할 때 진행) | 2026-09-30 | 사용자 지시 |
| R7 | 게임 Content 마운트: `GameContentDirectory()` = `<Project>/Content`(엔진 단독은 빈 값), `/JGGame/` 해석, 전체 경로 → 두 Content 판별, 시작 시 게임 Content 로드. 캐시는 로드 작업 예약 전에 메인 스레드에서 채움. 시점 조건은 사용자 지시로 제거 | 2026-09-30 (미커밋 → GM-C2) | 검증 파일 §3 · §4 — `/JGGame/TempAsset/GameSample.jgasset : Success Load Asset`, 엔진 단독 변화 0 |
| R8 | `JGConsole module.test <Module> [<Module>...]` (`Source/Programs/JGConsole/ModuleCommands.cpp`, 신규). 연결 → 역순 해제, 그 사이 Error/Critical 로그가 있으면 실패. 이름은 명령 규칙(area.action)대로 `modtest` → `module.test` | 2026-09-30 (미커밋) | 마무리 검증 §4 · §5 — 게임 Bin `module.test MyGame` 종료 코드 0 · `MyGame entry actor entered world` · error 0, 엔진 Bin `module.test GameFrameWorks` 0 · `NoSuchModule` 1, `console.selftest` 57/57 · `gmtest` · `net.test all` 66 통과 |
| GM-A1 | 같은 GUID 에셋: 먼저 올라온 것을 지키고 나중 것은 Error(두 경로 · GUID) + 실패 | 2026-09-30 (미커밋) | 마무리 검증 §6 — 엔진 Sample 복사본을 게임 Content 에 두고 런처 실행, Error 1줄 · 엔진 Sample 로드 유지 · 종료 코드 0 |
| GM-B1 | `ModuleInfo.h` `WriteJson` 복붙 수정 → `ModuleInfoTemplate.json` 이 `ModuleName: "Name"` · `ModulePath: "Source/[Category]/[ModuleName]"` | 2026-09-30 (미커밋) | 마무리 검증 §4 |
| GM-B2 | `Source/Runtime/Asset/Core.module.json` 삭제(툴이 무시하던 옛 사본). `Source/Dummy` 는 유지(닫음 표) | 2026-09-30 (미커밋) | 마무리 검증 §3 — 삭제 전후 PreBuild 산출물(vcxproj · sln · lua · 코드젠) 동일 |
| GM-B4 | 엔진 루트 `GenerateProjectFiles.bat`: `cd /d "%~dp0Build\BatchFiles"` + `call .\PreBuild.bat` | 2026-09-30 (미커밋) | 마무리 검증 §3 — `NoDefaultCurrentDirectoryInExePath=1`, 작업 폴더 `C:\` 에서 종료 코드 0 |
| P4 | 프로젝트 경로 검사: ASCII 가 아니면 배치 1 · 2 가 처음에 Critical + 종료 코드 1(`PProjectCreator::IsSupportedProjectPath`). 조사 결과 공백은 문제없어 허용 | 2026-09-30 (미커밋) | 마무리 검증 §2 — 공백 경로 생성 · 빌드 · 실행 통과, 한글 경로는 검사 전 premake 실패 → 검사 후 배치 1 · 2 모두 명확한 메시지로 1 |

---

## 커밋 체크리스트 (GM-C2 — 2026-09-30 저녁 추가분 전부)

| 구분 | 파일 | 비고 |
|---|---|---|
| 소스 (수정) | `Source/Runtime/Core/FileIO/FileHelper.cpp`, `Source/Runtime/Asset/AssetPath.cpp`, `Source/Runtime/Asset/AssetDatabase.cpp`, `Source/Programs/JGBuildTool/Main.cpp`, `Source/Programs/JGBuildTool/Class/ModuleInfo.h`, `Source/Programs/JGBuildTool/Class/ProjectCreator.h`, `Source/Programs/JGBuildTool/Class/ProjectCreator.cpp` | R7 · GM-A1 · GM-B1 · P4 |
| 소스 (신규) | `Source/Programs/JGConsole/ModuleCommands.cpp` | R8. **미추적 파일 — `git add` 필요** |
| 삭제 | `Source/Runtime/Asset/Core.module.json` | GM-B2 |
| 배치 | `GenerateProjectFiles.bat`(엔진 루트) | GM-B4 |
| 재빌드된 Bin | `Bin/DevelopEngine/*.dll` · `*.exe` · `*.lib` · `*.exp` · `Core.idb`(git status 의 Bin 수정분 전부) | `Core.lib`(FileHelper.cpp) 이 바뀌어 모든 DLL · exe 가 다시 링크됐다. **`JGBuildTool.exe` 는 P4 검사가 든 새 exe** — 배치가 이 exe 를 부른다 |
| 생성물 | `jgengine.bat`(엔진 절대경로만 `C:/Develop/JGEngine`), `Source/Programs/JGBuildTool/ModuleInfoTemplate.json`(GM-B1 로 재생성) | |
| 문서 | `Document/Memory/GameModule/현황.md`, `TODO.md`, `Files/2026-09-30_R7_게임Content_검증.txt`, `Files/2026-09-30_GameModule_마무리_검증.txt`, `Files/2026-09-30_R4_Game모듈_삭제_검증.txt`, `Files/게임프로젝트_생성가이드.md`, `Files/게임모듈_사전작업_분석_2026-09-29.md`(R4 · D5 행), `Document/Memory/진행현황.md` · `README.md`(GameModule 행 · 변경 로그), `Document/Memory/Etc/Files/DevConsole_TODO.md` · `Etc/TODO_DevConsole.md`(R8 행 완료 표시) | 진행현황 · README 는 다른 트랙도 고친다 |
| R4 | 삭제 `Source/Runtime/Game/Game.module.json` · `Main.cpp`, `Bin/DevelopEngine/Game.dll` · `.exp` · `.lib` / 재생성 `JGEngine.sln` · `jgengine.lua` · `Source/Programs/JGBuildTool/jgengine.lua` / 소스 `Source/Programs/JGBuildTool/Class/ProjectCreator.cpp`(P4 와 같은 파일, `Game` 예약 검사) / 재빌드 `Bin/DevelopEngine/JGBuildTool.exe`(22:09) | 삭제 3건은 `git add -A` 로 들어간다. `Bin/` 루트 · `Bin/ConfirmGame/` 의 옛 `Game.*` 사본은 JGDev_Graphics 때처럼 그대로 둔다(옛 빌드 잔재, 다른 옛 DLL 도 함께 남아 있음) |

넣지 않아도 되는 것: `Bin/DevelopEngine/jg_log.txt` · `Build/BatchFiles/jg_log.txt` · `Bin/DevelopEngine/imgui.ini`(실행 부산물, 진행현황 D-9). 다른 트랙 문서 수정분은 그 트랙 몫.

---

## 커밋 체크리스트 (GM-C1 — 완료, `4c8ac73`)

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
