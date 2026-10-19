# DevOps TODO

갱신 2026-10-19. **분야 이름 GameModule → DevOps(2026-10-02, 사용자 지시)** — 빌드 시스템 · 툴 · 게임 프로젝트 환경 · 빌드 성능 · **게임 빌드 파이프라인(쿠킹 · 패키징)**. 현재 상태와 검증 절차는 `현황.md`. ID 규칙: `B`/`H`/`R`/`P` 는 `Files/게임모듈_사전작업_분석_2026-09-29.md` §3–§4 의 번호 그대로, `GM-` 는 엔진-게임 분리 트랙에서 붙인 것, `BS-` 는 빌드 속도(2026-10-02~), `GP-` 는 게임 빌드 파이프라인(2026-10-02~, 글자 = 담당 분야: D DevOps · C Core 입력 · G Graphics · F GameFrameWorks · E JGEditor · A Data · S Server · P 게임 구현 · H 후속), `MT-` 는 메인 트리 동시 빌드 · 실행 겹침(2026-10-03~).

---

## 상태: 마무리 — 게임 빌드 파이프라인(GP) DevOps 몫 전부 완료 · 메인 반영(10-03 10:1x). M1(게임만 실행) · M2(쿠킹 · 패키지) 메인에서 끝까지 확인. MT-1 메인 트리 잠금 도우미 완료(10-03 10:45). 오디오 재생(D-37) 뒤처리 AU-1 ~ 3 · 게임 템플릿 배치 종료 코드 GM-B5 완료(10-11 14:16). 같은 패키지 여러 실행의 로그 파일 따로 쓰기 ER-024 완료(10-11 23:02). 대시보드 에디터 실행 스크립트 MT-2(`RunEditor.cmd`) 완료(10-17 14:43). **모두 커밋 `7b8ee6e`**(10-17 15:03 엔진 총괄 묶음 — GP-D1 · D3 · D5 의 Core 파일은 `ee36bb1`). 새 클론 첫 빌드 GM-B6(`BootstrapTools.bat`, D-40 후속) 완료(10-19 21:03, 미커밋)

10-02 저녁 사용자 요청: "`JGConsole.exe -runCook -project=<경로>` 로 쿠킹 · 패키징 → `Bin/ Content/ <Name>.exe`, exe 실행 → 게임만(ImGui 아님) — 설계를 꼼꼼히 보고하고 다른 세션 배정도 계획". 조사 + ReleaseGame 프로브(14 프로젝트 오류 0 · 경고 0, 3rd-party 손 복사 뒤) + 설계 + 배정안 작성, **코드 변경 없음**. 설계 `Files/게임빌드파이프라인_설계_2026-10-02.md`, 보고서 `Document/DevOps_게임빌드파이프라인_설계_2026-10-02.html`, 결정 진행현황 §3 D-23 ①~④. 보류였던 GM-H1 · R5 · B10 이 이 설계로 다시 열림(아래 "재개" 표).

**구현 (10-02 밤):** GP-D0 · D1 · D2 완료 → 메인 반영 · 회귀 통과(증적 `Files/2026-10-02_GP-D0-D2_구현_검증.txt`). 구현 중 사용자 지시: **패키지 구분은 `JG_WITH_EDITOR` 디파인 하나**(CoreDefines.h — DevelopEngine · DevelopGame = 1 에디터 빌드, ConfirmGame · ReleaseGame = 0 패키지 전용 빌드), **`IsPackageMode()` 제거**(설계 4-3). 그래서 패키지는 ConfirmGame · ReleaseGame 바이너리로만 만든다(에디터 빌드의 `JGGameLaunch` 는 종료 코드 4 로 거절).

**사용자 결정 D-23 (10-02 밤):** ① (a) 한 프로세스 · ② (a) 패키지 폴더 안 `Saved/` · ③ **(b) Content 만 패키징** — 엔진 · 게임이 Content 밖 데이터를 쓰면 Content 안으로 옮기라고 요청(엔진 셰이더 → Graphics GP-G4, ProjectAH `AHData` · `CardTemplates` → 게임 구현 GP-P0, 직접 전달함) · ④ (a) 배정대로. **기본 게임 화면 1920×1080.** 총괄 · 다른 세션이 떠 있지 않아 진행현황 D-23 행을 작업 지시서로 적었다.

빌드 속도(BS-0~3)는 커밋됨: **BS-C1 = `0bd6356`**(10-02 20:10, 사용자 요청으로 이 세션이 커밋 — `git commit -- <경로>` 로 DevOps 파일만, `Bin/` 은 다른 세션 코드가 섞여 제외 → 10-17 부터는 D-40 으로 `Bin/` 을 추적하지 않음).

**완료 (10-03 오전, 사용자 "끝까지 진행" · "이어서 진행"):** GP-D3 `GameHost` + `JGLauncher -game` · GP-D4 쿠킹 `-runCook` / `cook.run` · GP-D5 런타임 모듈 닫힘 · GP-D6 템플릿 · 가이드. 메인 엔진으로 검증 게임 GameUIDemo 를 쿠킹(85.6초 exit 0, 경고 assimp 1) → `GameUIDemo.exe` 가 게임만 1920×1080(월드 + 게임 UI) · 클릭 입력 · 종료 0 · live 0. 증적 `Files/2026-10-03_GP-D3-D6_구현_검증.txt`, 검증 도구 `Files/tools/gamehost_e2e.ps1`. 다음은 다른 세션 GP-S1(Server) · GP-P1(게임 구현) — 총괄 배정.

**MT-1 (10-03 10:45, 엔진 총괄 배정 — 대시보드 제안 "메인 트리 동시 빌드 겹침 줄이기"):** `Build/BatchFiles/MainTree.cmd`(+ `MainTree.ps1`) — 메인 트리 빌드 · 메인 Bin 실행을 잠금으로 차례 세움. `status` · `build` · `exec` · `hold`/`release`. 다른 세션의 잠금, 도우미 밖 빌드(MSBuild · cl · link 등 — 작업 폴더로 판정, VS · Rider 포함), 최근 빌드 출력, (build · hold 는) 메인 Bin 에서 도는 프로세스를 기다린다. 엔진 코드 변경 없음. 가짜 트리 + 실제 worktree 빌드로 검증(증적 `Files/2026-10-03_MainTree_도우미_검증.txt`). 쓰는 법 한 단락은 총괄에 보냄 → 지침서 §3(총괄 몫).

**새 클론 첫 빌드 GM-B6 (10-19 21:03, 엔진 총괄 배정 — 사용자 결정 D-40 "빼고 소스에서 빌드"의 후속):** `Bin/` 이 git 에 없어(`6b9d212`) 새로 받은 저장소에서는 `GenerateProjectFiles.bat` 가 빌드 도구를 못 찾아 실패했다(솔루션 빌드도 프로젝트 파일이 없어 MSB3202). 새 `Build/BatchFiles/BootstrapTools.bat` — 툴이 없을 때만 `PreBuild.bat` 가 불러 premake + VS 2022 MSBuild 로 Core · JGHeaderTool · JGBuildTool 을 먼저 빌드(약 20초), `CreateGameProject.bat` 는 툴이 없으면 안내 + 1. 새 클론에서 생성 · 전체 빌드 · 에디터 실행 · 2회차 생성 · 게임 배치 1 · 2 확인, 메인 반영(MainTree). 증적 `Files/2026-10-19_새클론_첫빌드_검증.txt`.

**대시보드 에디터 실행 스크립트 MT-2 (10-17 14:43, 엔진 총괄 배정 — 사용자 대시보드 명령 14:29 "대시보드에 에디터 실행할 수 있는 버튼", 버튼은 프로젝트 총괄):** `Build/BatchFiles/RunEditor.cmd` = `MainTree.ps1` 새 동작 `editor`. 메인 트리가 빌드 중(잠금 · 도우미 밖 MSBuild · cl · link · 10초 안 빌드 출력)이면 띄우지 않고 한 줄 · 종료 90(`-wait=<초>` 만큼 기다림), 같은 Bin 에디터가 떠 있으면 89, 아니면 `Bin/DevelopEngine/JGLauncher.exe` 를 따로 띄우고(셸 실행 — 파이프를 물려주지 않음) 0. 잠금은 띄우는 2~3초 동안만. 게임 프로젝트 `-project=` 도 넣음(그 프로젝트의 빌드만 봄). 엔진 코드 변경 없음. Node 에서 파이프로 불러 8가지(띄우기 · 이미 떠 있음 · 에디터가 떠 있으면 hold 가 기다림 · 다른 세션 hold 로 빌드 중 · 게임 프로젝트 · `-wait` · 빌드 출력 · 바로 끝남) 확인, 띄운 에디터는 모두 종료 0 으로 닫음. 증적 `Files/2026-10-17_RunEditor_에디터실행_검증.txt`, 도구 `Files/tools/runeditor_check.js`.

**같은 패키지에서 여러 개 띄울 때 로그 파일 따로 쓰기 ER-024 (10-11 22:51 엔진 총괄 배정, 사용자 결정 없이 — 게임 쪽 요청):** `JGGameLaunch.cpp` 가 로그 파일에 바이트 범위 잠금을 걸어 이름을 잡고, 쓰는 중이면 `<Name>_2.log` … , `-logfile=<경로>` 도 넣음. 혼자 띄우면 이름 그대로. 검증 게임 `Temp/Games/LogCheck` 패키지로 3개 동시 · 혼자 · `-logfile` · 서술자 없음(종료 2) · 죽은 실행 모두 확인. 증적 `Files/2026-10-11_ER-024_패키지_로그파일_검증.txt`, 도구 `Files/tools/package_multi_launch.ps1`.

**오디오 재생(D-37) 뒤처리 (10-11, Core 부탁):** Core 가 hold 13:54 ~ 14:02 에 Audio 모듈과 JGConsole 연결 줄을 메인에 넣음 → DevOps 가 생성 가이드 9절 "소리 쓰기" · 2절 표 · 5절(다른 엔진으로 옮기기) · 6절, 쿠킹은 Core 검증 게임 AudioCheck 보고서로 확인(Audio.dll · 스모크), Core 가 알려 준 "엔진 불일치인데 배치 종료 0" 은 게임 템플릿 배치 마지막 줄 탓 → 템플릿 고침(GM-B5). 증적 `Files/2026-10-11_Audio_가이드_템플릿배치_검증.txt`.

### GP — DevOps 담당 (전부 완료 — 완료 이력 표)

(남은 DevOps GP 항목 없음. 보류 후보는 아래 보류 표 GP-H*)

다른 세션 GP 항목(설계 5 · 7절, 지시서 = 진행현황 D-23 행): GP-C1 입력 계층(GUI) · GP-G1 크기 변경 · G2 `PSceneRenderer::Resize` · G3 출력 다듬기 · **G4 셰이더 → `<Engine>/Content/Shaders`**(Graphics) · GP-F1 게임 뷰포트(GameFrameWorks) · GP-E1 씬 뷰포트 공용화 · E2 패키징 메뉴(선택)(JGEditor) · GP-A1 쿠킹 검사(Data) · GP-S1 패키지 네트워크(Server) · **GP-P0 ProjectAH 데이터 → Content**(10-02 밤 직접 전달) · GP-P1 ProjectAH 패키지(게임 구현).

회귀는 모두 통과(BS-V1 11:52 닫음). D-20 적용 뒤 알림: 메인 트리 엔진을 쓰는 게임 프로젝트(ProjectAH · 검증 게임)는 배치 2 를 다시 돌려야 `/MP` 가 들어간다(가이드 표). `PCH.h` 가 바뀌어 다음 빌드는 한 번 전체 다시 컴파일(이제 약 1분).

R4 적용 뒤 알림(09-30): 메인 트리 엔진을 `EngineRoot` 로 쓰는 게임 프로젝트(다른 세션의 `GfwView` 등)는 각자 `GenerateProjectFiles.bat` 를 다시 돌려야 한다(안 하면 옛 `Game` 프로젝트가 `C1083`).

다른 트랙이 넘긴 빌드 도구 항목: GFW C-1 → **완료(10-02, D-19 ②)** — 아래 완료 이력. JGEditor E-H4(JGHeaderTool 이 지운 JGCLASS 헤더의 생성 파일을 남김)는 그 트랙 보류 표(`Etc/TODO_JGEditor.md`)에 있다.

---

## 재개 (09-30 보류 → 10-02 사용자 요청 "게임 빌드 파이프라인"으로 다시 열림 → D-23 결정 뒤 GP-D3 · D5 로 완료 10-03, 커밋 `7b8ee6e`)

| ID | 항목 | 어디로 | 비고 |
|---|---|---|---|
| GM-H1 | 게임 단독 실행 호스트 — 창 · Graphics 를 누가 여나 | **GP-D3 `GameHost` 런타임 모듈**(+ GP-D2 부트스트랩 · 진입 DLL). 실행 구조는 D-23 ① | 09-29 분석 §6 D8 자리. 후보 셋(런처 인자 · 별도 런치 모듈 · 템플릿 호스트) 중 "별도 모듈 + 런처 `-game`" |
| R5 | 게임 실행에서 `<Name>Editor` 제외(런타임) | **GP-D5**(모듈 닫힘 → 패키지 `RuntimeModules` · 개발 `runtime_module_info.json`) + GP-D1(`HModuleSystemInfo::Get` 이 패키지 목록) | `codeGen()` 이 `module_system_info.json` 의 모든 SharedLib 를 시작 시 로드(`ObjectGlobalSystem.cpp:249-279`) |
| B10 | 모듈 종류(Runtime/Editor) + Release 구성에서 Editor 모듈 제외 | 판별은 **GP-D5**(폴더 `Source/Editor` + `.jgproject` EditorModules, module.json 새 필드 없음), 솔루션 구성에서 빼기는 **GP-H4**(빌드 시간 — 보류) | 패키징은 닫힘 목록만 복사하므로 H4 없이도 패키지에 에디터 바이너리 0 |

## 보류 (사용자 결정 2026-09-30 등: 필요할 때 진행)

| ID | 항목 | 다시 열 조건 | 완료 기준 | 비고 |
|---|---|---|---|---|
| MT-H1 | 실행 파일 로그 경로 옵션(`JGLauncher` · `JGConsole` `-log=<경로>`) — 패키지 실행은 ER-024(10-11)로 해결(`-logfile=` · `_2` …), 개발 실행만 남음 — 동시 실행 때 `Bin/<구성>/jg_log.txt` 덮어쓰기 근본 해결 | 도우미 밖 동시 실행으로 로그가 계속 섞일 때 | 실행마다 다른 로그 파일 | 엔진 코드(Core 는 `HCoreSystemArguments::LogFilePath` 가 이미 있음 — 런처 · 콘솔 인자만). MT-1 의 exec 는 도우미끼리만 차례로 돌려 덮어쓰기를 줄인다. 실제 사례 10-03 19:25: 메인 `-game` 확인 중 다른 프로세스(pid 57264)의 줄이 같은 로그에 섞임 |
| GP-H4 | ConfirmGame · ReleaseGame 솔루션 구성에서 에디터 프로젝트 빌드 끄기 | 쿠킹 빌드 시간이 문제일 때 | Release 구성 빌드 프로젝트 = 런타임 + 툴만 | premake 구성별 제외. 패키지 내용에는 영향 없음 |
| GP-H5 | 게임 exe 아이콘 · 버전 리소스 | 배포할 때 | `.jgproject` `Package.Icon` · `Version` 이 exe 속성에 보임 | 부트스트랩 사본에 `UpdateResource` |
| GP-H6 | 패키지 크래시 덤프 `Saved/Crashes` | 배포 빌드에서 크래시를 분석해야 할 때 | 처리되지 않은 예외 → 미니덤프 + 로그 | GP-D0 PDB(`<Output>-Symbols`) 와 같이 쓴다 |
| GP-H10 | Alt+Enter 테두리 없는 전체 화면 전환 · 커서 모양 | 요청이 올 때 | — | GUI 세션과 같이 |
| C1-H1 | JGBuildTool 이 의존 모듈의 include 경로를 전이(근본안) | **GFW 의존 모듈이 늘 때**(사용자 결정 D-19 ②, 10-02) | 게임 템플릿이 `GameFrameWorks` 만 적어도 GFW 의존의 헤더가 잡힘 | 지금은 템플릿에 `Asset` · `Graphics` 를 직접 적어 해결(C-1). 전이는 `BuildTool.cpp` 의 includedirs 생성(직접 의존만, :325-344 근처)을 바꾸는 일. GFW 세션 규칙(10-02, GFW TODO · 현황 §5): GFW 공개 헤더가 새 모듈을 include 하게 되면 템플릿에도 넣도록 DevOps 에 알린다 |
| BS-2b | PCH 에 `Core.h` 통째 | BS-2 뒤에도 빌드 CPU 가 문제일 때 | 컴파일 CPU 217초 수준(r4) | Core 헤더 수정 때 PCH 14개 재생성으로 그 경우는 느려짐 |
| BS-H1 | 의존 줄 줄이기 · 공용 PCH | BS-1 · BS-2 뒤 전체 빌드 하한(약 50초)이 문제일 때 | — | 모듈마다 정의가 달라 PCH 공유 불가 → 모듈 구조 변경 |
| BS-H2 | 게임 프로젝트가 엔진 바이너리 공유(D3 변경) | 게임 솔루션 빌드가 문제일 때 | — | 지금은 게임 솔루션마다 엔진 14개를 다시 컴파일 |
| BS-H3 | 컴파일러 캐시(sccache · buildcache) · Unity 빌드 | worktree 마다 같은 소스를 다시 빌드하는 비용이 문제일 때 | — | `/Z7` 필요 · MSVC PCH 궁합 / 이름 충돌 정리 |
| BS-H4 | Defender 제외 · Dev Drive | 사용자 판단 | — | 보안 설정 — 에이전트가 바꾸지 않음, 측정 안 함 |

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
| R4 | 엔진 `Source/Runtime/Game`(더미, `int dummy()` 뿐) 삭제 — 09-29 에는 D5 로 불필요해 제외, 09-30 밤 사용자 결정으로 재개. `Bin/DevelopEngine/Game.*` 삭제 · 생성물 재생성(14 프로젝트) + `ProjectCreator.cpp` 에 `Game` 프로젝트 이름 예약(폴더 검사가 사라져도 `JGGameEntryActor` 충돌을 배치 1 에서 막음) | 2026-09-30 22:13 (커밋 `736ddbe`) | `Files/2026-09-30_R4_Game모듈_삭제_검증.txt` — worktree(엔진 · `MyGame` 빌드 · 실행) + 메인 트리 §12(PreBuild 차이 `Game` 3곳뿐, 빌드 오류 0, `Game` 이름 종료 코드 1, JGConsole · 런처 60초 종료 0 · error 0) |
| R6 | DevelopGame 의 게임 모듈 = ConfirmConfig (규칙) | 2026-09-29 | 템플릿 `*.module.json`, 가이드 §4 |
| — | 템플릿 13 파일 · `CreateGameProject.bat` · `GenerateGameProjectFiles.bat` · `-newproject=`(ProjectCreator) · 가이드 | 2026-09-29 | 구현검증 §3, `Files/게임프로젝트_생성가이드.md` |
| — | `PString::GetRawWString` 폴백(게임 프로젝트 창 제목 `(null)`) | 2026-09-29 | 구현검증 §4 |
| — | 엔진 단독 A/B 회귀(산출물 동일, 종료 코드 139 → 0, 2회차 컴파일 0) | 2026-09-29 | 구현검증 §1 |
| — | 게임 E2E: 생성 → 솔루션 2회 → `MyGame.sln` 17 프로젝트 오류 0 → 실행(두 모듈 로드, 종료 코드 0, live blocks 0) | 2026-09-29 | 구현검증 §3 · §4, `Files/2026-09-29_jgeditor_mygame_capture.png` |
| — | 메인 트리 반영(3-way 병합 충돌 0, PreBuild 0, 빌드 오류 0, 런처 0, 메인 배치 생성 0) | 2026-09-30 | 구현검증 §5 |
| P1 · P2 · P5 | HEAD `1308fd5` 위에서 worktree 격리 작업 · GameFrameWorks API 이름 경계 확인 | 2026-09-29 | 분석 §4-2 |
| GM-C1 | 커밋(`4c8ac73`, 사용자, 체크리스트 전부 포함 확인) + 커밋 뒤 회귀(`C:\Develop\JGEngine`: PreBuild 2회 · 엔진 빌드 · 엔진 단독 실행 · 생성 · 솔루션 2회 · 게임 빌드 · 실행 — 전부 종료 코드 0, 오류 0, 엔진 2회차 컴파일 0) | 2026-09-30 | `Files/2026-09-30_R7_게임Content_검증.txt` §2–§5 |
| GM-H1 · R5 · B10 | 보류 결정(사용자: 게임 단독 실행은 필요할 때 진행) | 2026-09-30 | 사용자 지시 |
| GM-C2 | 커밋(사용자 `736ddbe` "엔진 구현") — R7 · R8 · GM-A1 · 백로그 · R4 전부 포함, 커밋 뒤 `git status` 에 이 트랙 파일 없음 | 2026-10-01 | `git show --stat 736ddbe`(`Source/Runtime/Game/*` 삭제 · `ModuleCommands.cpp` · `ProjectCreator.*` · `JGEngine.sln`) |
| — | 분야 이름 GameModule → DevOps(폴더 이동, 진행 문서 · 보고서 HTML 3개의 경로 참조 갱신) | 2026-10-02 | 사용자 지시, README 6절 매핑 |
| BS-0 | 빌드 속도 검토 — 기준선 272.6초(평균 동시 컴파일 1.27, 프런트엔드 94%), 실험 r1~r6 · m1~m3, 증분 4종 × 4설정. 권고 BS-1 `/MP` · BS-2 PCH 서드파티 · BS-3 worktree 위치. 코드 · 설정 변경 없음 | 2026-10-02 | `Files/빌드속도_검토_2026-10-02.md`, `Document/DevOps_빌드속도_검토_2026-10-02.html`, 스크립트 `Files/tools/build_timing/` |
| BS-1 | 프로젝트 안 병렬 컴파일 `/MP` — `BuildTemplate.lua` 두 함수에 `flags { "MultiProcessorCompile" }`(사용자 결정 D-20) | 2026-10-02 11:35 (커밋 `0bd6356`) | 생성 vcxproj 14개 × 4구성 `<MultiProcessorCompilation>true`, 메인 전체 다시 빌드 66.0초(기준 272.6) · 평균 동시 10.5, 게임 솔루션(C1Check) 배치 2 뒤 엔진 · 게임 vcxproj 모두 true · 빌드 67.5초 오류 0 · 실행 종료 0. 증적 `Files/2026-10-02_BS1-BS2-BS3_적용_검증.txt` |
| BS-2 | PCH 에 spdlog · rapidjson(+ `#pragma warning(disable : 4996)`) — `Source/PCH/PCH.h`(사용자 결정 D-20) | 2026-10-02 11:35 (커밋 `0bd6356`) | 경고 = 기준선(LNK4098 4, C4996 0), GFW 공용 헤더 수정 22.3초(기준 219.8) · Core 공용 헤더 40.2초(기준 356.1), 변경 없음 컴파일 0, 회귀 `gmtest` · `console.selftest` 64/64 · `gameui.selftest` 243/243 · `datatable.selftest` 127/127 · 런처 60초 종료 0 · live 0. `net.test all` 96 통과 · transport 5 는 포트 47790 충돌(BS-V1) |
| BS-V1 | `net.test` 회귀 마무리 — 11:3x 에는 포트 47790 을 `C:\Develop\ProjectAH\Document\tools\feedback_server.pl`(피드백 페이지 "다리" 세션)이 쥐어 transport 5건 bind 실패. Server 세션이 TCP 테스트를 빈 포트(`Listen(0)`)로 바꿈(미커밋, `NetTcpTransport.h/.cpp` · `GameplayNetSelfTest.cpp`) | 2026-10-02 11:52 | 포트 47790 이 여전히 LISTEN 인 채로 메인 Bin `net.test transport` 19/19 · `net.test all` 101/101 · 종료 0 · live 0. 증적 `Files/2026-10-02_BS1-BS2-BS3_적용_검증.txt` §7, Server `Files/2026-10-02_net_test_port_results.txt` |
| BS-3 | worktree · 검증 게임 위치 규칙 — 지침서 §3 "병렬 세션 주의" 한 줄 + 변경 이력(사용자 결정 D-20). 생성 가이드에 "빌드 템플릿이 바뀌면 배치 2" 행 | 2026-10-02 (커밋 `0bd6356`) | 지침서 §3 · 변경 이력, 가이드 2절 표. 다른 세션 안내는 총괄 |
| C-1 (GFW) | 게임 템플릿 의존에 `Asset` · `Graphics` — `{PROJECT_NAME}.module.json` · `{PROJECT_NAME}Editor.module.json`(에디터도 GFW 헤더를 보면 같은 문제라 함께), 생성 가이드 4절 규칙 한 줄, 템플릿 `{PROJECT_NAME}Module.cpp` 주석(에디터 · JGConsole 이 GFW 를 먼저 연결 — DevConsole 5-1, 총괄 요청). include 전이는 보류 C1-H1 | 2026-10-02 (커밋 `0bd6356` · GFW 쪽 C-1 정리 완료 10-02) | 새 게임 프로젝트(스크래치패드 `Games/C1Check`, 메인 트리 엔진)의 게임 · 에디터 모듈이 `Actors/CameraActor.h` · `Components/StaticMeshComponent.h` · `Data/DataTable.h` include + `static_assert(sizeof…)` → 배치 2 exit 0 · include 경로에 Asset · Graphics(+ CodeGen) · 게임 솔루션 16 프로젝트 빌드 오류 0(117초) · 런처 40초(dbgevents) 종료 0 · error/critical 0 · `C1Check entry actor entered world` · live blocks 0 |
| BS-C1 | 커밋 `0bd6356` "DevOps: 빌드 속도 개선(/MP · PCH), 게임 템플릿 의존, 분야 이름 DevOps"(10-02 20:10, 사용자 요청 "관련해서 커밋해주고" → 이 세션이 `git commit -- <경로>` 로 55 파일. GUI 세션이 스테이지한 삭제 3건은 건드리지 않음) | 2026-10-02 | `git show --stat 0bd6356`. **넣지 않음:** `Bin/DevelopEngine/*`(다른 세션 미커밋 코드로 다시 빌드된 것이 섞임 — 사용자 일괄 커밋 몫) |
| MT-2 | 대시보드 에디터 실행 스크립트 `Build/BatchFiles/RunEditor.cmd`(+ `MainTree.ps1` 동작 `editor` · `MainTree.cmd` 주석) — 빌드 중이면 90, 이미 떠 있으면 89, 아니면 에디터를 따로 띄우고 0, 사용법 91 · 없음 92 · 바로 끝남 93, `-wait=<초>` · `-name=` · `-project=<게임 프로젝트>`, 출력 한국어 한 줄(`MainTree.ps1` 은 UTF-8 BOM), 기록 `Temp/MainTree/history.log` editor 줄. 생성 가이드 3절 표 | 2026-10-17 14:43 (커밋 `7b8ee6e`) | Node 파이프 호출 8가지 2회 ALL PASS(14:37 · 14:39) — 띄우기 3.8초 · 파이프가 스크립트 끝과 같이 닫힘, 89 · 90(hold 대역 · 빌드 출력 대역) · 게임 프로젝트(GameUIDemo) 0/89 · `-wait=40` 12.3초 뒤 0 · 가짜 실행 파일 93, 에러 경우 91 × 3 · 92 × 3, MainTree 회귀(status · exec 종료 코드 · 사용법), 띄운 에디터 종료 0. 증적 `Files/2026-10-17_RunEditor_에디터실행_검증.txt` |
| GM-B6 | 새 클론 첫 빌드(D-40 후속) — 새 `Build/BatchFiles/BootstrapTools.bat`(premake5 + `jgengine.lua` → `ThirdParty` lib 2개 복사 → VS 2022 MSBuild 로 Core · JGHeaderTool · JGBuildTool, DevelopEngine x64), `PreBuild.bat` 는 툴이 없을 때만 부트스트랩 · 실패면 종료 1, `CreateGameProject.bat` 는 툴이 없으면 안내 + 1 | 2026-10-19 21:03 (미커밋) | 새 클론 `Temp/wt/d40_clone`: 고치기 전 생성 종료 1 · 솔루션 MSB3202 18줄 → 고친 뒤 생성 18초 0 · 전체 빌드 37초 오류 0 · 에디터 40초 종료 0 · live 0 · 2회차 생성 3초(부트스트랩 없음) · 증분 0 · `console.selftest` 64/64 · 게임 배치 1 · 2 종료 0, 실패 안내 두 가지(VS 를 못 찾음 · 툴 없이 배치 1). 메인 반영 MainTree hold → 생성 0 → 빌드 0 → release. 증적 `Files/2026-10-19_새클론_첫빌드_검증.txt` |
| ER-024 | 같은 패키지에서 여러 개 띄울 때 로그 파일 따로 쓰기(게임 쪽 요청, 엔진 총괄 배정) — `JGGameLaunch.cpp` `claimLogFile`: `<Name>.log` → `_2` → … 차례로 `LockFileEx`(파일 끝 너머 1 바이트, 배타 · 즉시 실패)로 이름을 잡고 게임이 끝날 때까지 쥠, `-logfile=<경로>`(상대 = `Saved/Logs`), 첫머리 `Log file … (process N)` 줄. 혼자 띄우면 이름 그대로 | 2026-10-11 23:02 (커밋 `7b8ee6e`) | 메인 빌드 DevelopEngine · ReleaseGame exit 0, `LogCheck` 쿠킹 97.2초 OK(스모크 = 기본 이름), 3개 동시 → 로그 3개 각각 시작 1 · 종료 1 · 오류 0 · 섞임 없음, 혼자 → `LogCheck.log` 만 다시 씀, `-logfile` 상대 2개 · 절대 1개, 서술자 없음 종료 2, 강제 종료 뒤 잠금 풀림. 증적 `Files/2026-10-11_ER-024_패키지_로그파일_검증.txt` |
| GM-B5 | 게임 템플릿 `GenerateProjectFiles.bat` 실패 종료 코드 — 마지막 줄 `if errorlevel 1 pause` 의 `pause` 가 배치를 0 으로 끝내 툴 실패(예: 엔진 불일치 Critical)를 스크립트 · 에이전트가 성공으로 읽음(Core 보고) → `if errorlevel 1 (` · `pause` · `exit /b 1` · `)`. 이미 만든 프로젝트는 가이드 6절대로 손으로 | 2026-10-11 14:16 (커밋 `7b8ee6e`) | 스크래치 실험(옛 0 · 새 1) + 메인 Bin 배치 1 로 만든 TplCheck: 정상 exit 0 · EngineRoot 불일치 새 배치 exit 1(Critical 2줄 + failed) · 옛 마지막 줄 사본 exit 0. 증적 `Files/2026-10-11_Audio_가이드_템플릿배치_검증.txt` |
| AU-1 | 생성 가이드: 9절 "소리 쓰기"(Core 초안 — module.json `Audio` · StartupModule 연결 코드 · Content 소리 파일 · `HAudio` API · `-nosound` · 배치 2), 2절 표(게임 모듈 의존 변경 행, 엔진 모듈 추가 행 Audio 예시 + 게임 Bin JGConsole 연결 실패 줄), 5절 다른 엔진으로 옮기기, 6절 두 행 | 2026-10-11 14:16 (커밋 `7b8ee6e`) | API 이름은 메인 `Audio.h` · `AudioGlobalSystem.cpp` 와 대조. 같은 증적 |
| AU-2 | 쿠킹에 Audio — Core 검증 게임 `Temp/Games/AudioCheck`(메인 엔진) 보고서 확인: 런타임 모듈에 Audio, 패키지 `Bin/Audio.dll`, DLL 검사 · 스모크(300 프레임 종료 0 · 오류 0 · Content 밖 읽기 0) 통과, 경고 assimp 1(기존). 쿠킹 코드 변경 없음 | 2026-10-11 | `Temp/Games/AudioCheck/Temp/Cook/ReleaseGame/report.txt`(14:01:08), 같은 증적 §2 |
| AU-3 | `JGConsole/Main.cpp` `CONSOLE_ENGINE_MODULES` = `{ "GameFrameWorks", "Audio" }` + 주석 — DevOps 승인, Core 가 Audio.dll 과 같은 hold 안에서 넣음(먼저 넣으면 JGConsole 실행마다 연결 실패) | 2026-10-11 14:02 (커밋 `7b8ee6e` — Audio 본체는 `ee36bb1`) | Core 메인 회귀 10종(`console.selftest` 64/64 포함) 종료 0 · live 0 |
| MT-1 | 메인 트리 잠금 도우미 `Build/BatchFiles/MainTree.cmd` · `MainTree.ps1` — status · build · exec · hold/release, 잠금(`Temp/MainTree/lock`, 죽으면 OS 가 풂) · owner · 대기 · history, 도우미 밖 빌드 감지(MSBuild 인자 · 작업 폴더, cl · link · lib · rc · mt · cvtres · premake5 작업 폴더 — PEB 에서 읽음, 64비트 · WOW64), 빌드 출력 조용함 창 10초, build · hold 는 메인 Bin 프로세스도 기다림, `Temp/wt` · `Temp/Games` 제외 | 2026-10-03 10:45 (커밋 `7b8ee6e`) | 가짜 트리(대역 cl · link · 32비트 MSBuild · JGLauncher) 시험 표 전부 기대대로, 실제 worktree: 생성 5초 · 빌드 48초 exit 0, 빌드 중 status 에 잠금 · MSBuild · cl x3/x7 · link 표시, 도우미 밖 32비트 MSBuild 를 29초 기다린 뒤 빌드 exit 0. 증적 `Files/2026-10-03_MainTree_도우미_검증.txt` |
| GP-D5 | 런타임 모듈 닫힘: JGBuildTool 이 `Temp/CodeGen/runtime_module_info.json`(뿌리 GameHost + GameModules, SharedLib 의존 순서, 에디터 의존 · 개발용 의존 기록) + Core `HRuntimeModuleInfo` · `ECoreSystemFlags::RuntimeModulesOnly` + `JGLauncher -game` | 2026-10-02 밤 (커밋 `7b8ee6e` · Core 파일 `ee36bb1`) | GPCheck · GameUIDemo 목록 = Asset · Graphics · GameFrameWorks · GameHost · 게임, 에디터 의존 픽스처(GPCheck → DevConsole) → 쿠킹 4단계 실패. 증적 `Files/2026-10-03_GP-D3-D6_구현_검증.txt` |
| GP-D4 | 쿠킹 명령 `JGConsole.exe -runCook -project=` / `cook.run`(10단계 · 출력 가드 · 잠금 · PE import 감사 · VC++ 런타임 앱 로컬 · 콘텐츠 검사(Data GP-A1) · 스모크 · 보고서), `Source/Programs/JGConsole/Cook/` | 2026-10-03 (커밋 `7b8ee6e`) | GPCheck 81초 · GameUIDemo 85.6초(메인 엔진) exit 0, 가드 4종, 셰이더는 Content/Shaders(G4) — 임시 복사 코드 삭제. 같은 증적 |
| GP-D3 | `GameHost` 런타임 모듈(`Source/Runtime/GameHost/`) — 창(1920×1080 클라이언트, 작업 영역 맞춤, -windowed · -fullscreen · -res · -novsync) · 엔진 모듈 · 게임 모듈 · PGameViewport · VSync · 크기 변경 · 입력 · IME · 최소화 쉼 + Core `bExitRequested` | 2026-10-03 (커밋 `7b8ee6e` · Core 파일 `ee36bb1`) | `-game` · 패키지 exe 모두 월드 + 게임 UI 화면, 클릭 → 게임 UI(turn 2), 크기 3번 · 최소화/복원 뒤 새 크기로 그림(F1b), exit 0 · live 0. 같은 증적 |
| GP-D6 | 템플릿 `.jgproject` "Package" · `.gitignore` `Package/` `Saved/`, 생성 가이드 8절(게임만 실행 · 패키징) + 이름 규칙 ASCII(D-26) | 2026-10-03 (커밋 `7b8ee6e`) | 가이드 · 템플릿 diff |
| GP-D0 | 엔진 솔루션도 3rd-party 를 4구성 Bin 에 복사(`BuildTool.cpp` Run) + ConfirmConfig · ReleaseConfig `symbols "On"` + `.gitignore` `Bin/DevelopGame/` · `Bin/ConfirmGame/` · `Bin/ReleaseGame/` | 2026-10-02 밤 (커밋 `7b8ee6e`) | 손 복사 없이 엔진 ReleaseGame 빌드 오류 0 · 경고 0(51초) · PDB 16개, 메인 ReleaseGame 52초 오류 0. 증적 `Files/2026-10-02_GP-D0-D2_구현_검증.txt` |
| GP-D1 | 패키지 빌드(Core): `JG_WITH_EDITOR`(CoreDefines.h) · `HPackageDescriptor`(새 `FileIO/PackageDescriptor.*`) · 패키지 마운트(Content/JGEngine · JGGame · Shaders) · 패키지 모듈 목록 · 개발 경로 오류 · 읽기 가드 · 기본 루트 = exe 위치 기준 · `SavedDirectory` · `LogFilePath` · `RequestExit` · `.jgproject` "Package" 섹션 · `HPlatform::GetExecutableDirectory`. `IsPackageMode()` 는 사용자 지시로 제거 | 2026-10-02 밤 (커밋 `ee36bb1` — Core 묶음) | 개발 회귀 통과(메인 gmtest · console 64 · gameui 243 · datatable 127 · net all · 에디터 60초 · 게임 프로젝트 에디터), 다른 작업 폴더에서 JGConsole 정상(옛 바이너리는 잘못된 루트 + 종료 assert), ReleaseGame 손 패키지 스모크 exit 0 · error 0 · live 0, 실패 경로 2 · 3, Release 런처 패키지 없이 바로 종료. 같은 증적 |
| GP-D2 | 부트스트랩 `JGGameBootstrap`(Standalone · 정적 CRT · imports KERNEL32 · USER32) + 진입 `JGGameLaunch.dll`(-smoke · -log · -launchmodule, 종료 코드 0/2/3/4) + BuildTool `"Standalone"` · Programs 범주를 module_system_info 에서 제외 | 2026-10-02 밤 (커밋 `7b8ee6e`) | 손 패키지 `GPCheck.exe -smoke=120` 1초 exit 0, 공백 경로 · 다른 작업 폴더, `JGGameLaunch.dll` 없음 1 · 서술자 없음 2 · 게임 DLL 없음 3 · 에디터 빌드 4. 같은 증적 |
| GP-0 | 게임 빌드 파이프라인 설계 — 조사(실행 · 출력 · 크기 변경 · 입력 · 루트 · Content · 셰이더 · 모듈 로드 · 빌드 구성 · 3rd-party · ProjectAH 데이터) + ReleaseGame 프로브(`Temp/wt/release_probe`, HEAD `0bd6356`: LNK1104 → 3rd-party 손 복사 뒤 14 프로젝트 오류 0 · 경고 0 37.4초, Release 자체 테스트 · 런처 30초 정상, worktree 지움) + 설계 · 작업 16개 · 세션 배정 · 결정 D-23. 코드 변경 없음 | 2026-10-02 저녁 | `Files/게임빌드파이프라인_설계_2026-10-02.md`, `Document/DevOps_게임빌드파이프라인_설계_2026-10-02.html` |
| R7 | 게임 Content 마운트: `GameContentDirectory()` = `<Project>/Content`(엔진 단독은 빈 값), `/JGGame/` 해석, 전체 경로 → 두 Content 판별, 시작 시 게임 Content 로드. 캐시는 로드 작업 예약 전에 메인 스레드에서 채움. 시점 조건은 사용자 지시로 제거 | 2026-09-30 (커밋 `736ddbe`) | 검증 파일 §3 · §4 — `/JGGame/TempAsset/GameSample.jgasset : Success Load Asset`, 엔진 단독 변화 0 |
| R8 | `JGConsole module.test <Module> [<Module>...]` (`Source/Programs/JGConsole/ModuleCommands.cpp`, 신규). 연결 → 역순 해제, 그 사이 Error/Critical 로그가 있으면 실패. 이름은 명령 규칙(area.action)대로 `modtest` → `module.test` | 2026-09-30 (커밋 `736ddbe`) | 마무리 검증 §4 · §5 — 게임 Bin `module.test MyGame` 종료 코드 0 · `MyGame entry actor entered world` · error 0, 엔진 Bin `module.test GameFrameWorks` 0 · `NoSuchModule` 1, `console.selftest` 57/57 · `gmtest` · `net.test all` 66 통과 |
| GM-A1 | 같은 GUID 에셋: 먼저 올라온 것을 지키고 나중 것은 Error(두 경로 · GUID) + 실패 | 2026-09-30 (커밋 `736ddbe`) | 마무리 검증 §6 — 엔진 Sample 복사본을 게임 Content 에 두고 런처 실행, Error 1줄 · 엔진 Sample 로드 유지 · 종료 코드 0 |
| GM-B1 | `ModuleInfo.h` `WriteJson` 복붙 수정 → `ModuleInfoTemplate.json` 이 `ModuleName: "Name"` · `ModulePath: "Source/[Category]/[ModuleName]"` | 2026-09-30 (커밋 `736ddbe`) | 마무리 검증 §4 |
| GM-B2 | `Source/Runtime/Asset/Core.module.json` 삭제(툴이 무시하던 옛 사본). `Source/Dummy` 는 유지(닫음 표) | 2026-09-30 (커밋 `736ddbe`) | 마무리 검증 §3 — 삭제 전후 PreBuild 산출물(vcxproj · sln · lua · 코드젠) 동일 |
| GM-B4 | 엔진 루트 `GenerateProjectFiles.bat`: `cd /d "%~dp0Build\BatchFiles"` + `call .\PreBuild.bat` | 2026-09-30 (커밋 `736ddbe`) | 마무리 검증 §3 — `NoDefaultCurrentDirectoryInExePath=1`, 작업 폴더 `C:\` 에서 종료 코드 0 |
| P4 | 프로젝트 경로 검사: ASCII 가 아니면 배치 1 · 2 가 처음에 Critical + 종료 코드 1(`PProjectCreator::IsSupportedProjectPath`). 조사 결과 공백은 문제없어 허용 | 2026-09-30 (커밋 `736ddbe`) | 마무리 검증 §2 — 공백 경로 생성 · 빌드 · 실행 통과, 한글 경로는 검사 전 premake 실패 → 검사 후 배치 1 · 2 모두 명확한 메시지로 1 |

---

## 커밋 체크리스트 (GM-B6 새 클론 첫 빌드 + 커밋 상태 정정 — 2026-10-19, 미커밋)

| 구분 | 파일 | 비고 |
|---|---|---|
| 새 파일 (`git add` 필요) | `Build/BatchFiles/BootstrapTools.bat`, `Document/Memory/DevOps/Files/2026-10-19_새클론_첫빌드_검증.txt` | `.bat` 은 ASCII · CRLF |
| 수정 | `Build/BatchFiles/PreBuild.bat` · `Build/BatchFiles/CreateGameProject.bat` | |
| 문서 (수정) | `DevOps/Files/게임프로젝트_생성가이드.md`(0절 · 6절), `DevOps/TODO.md` · `현황.md`(커밋 상태 정정 포함), `진행현황.md` DevOps 행 | 진행현황은 다른 세션도 고친다 — DevOps 행만 |
| 저장소 밖 | 위키 「엔진 프로그램」 180 · 181 · 186줄 | obsidian — git 아님 |

## 커밋 체크리스트 (MT-2 — 완료, `7b8ee6e` 2026-10-17 15:03 엔진 총괄 묶음)

| 구분 | 파일 | 비고 |
|---|---|---|
| 새 파일 (`git add` 필요) | `Build/BatchFiles/RunEditor.cmd`, `Document/Memory/DevOps/Files/tools/runeditor_check.js`, `Document/Memory/DevOps/Files/2026-10-17_RunEditor_에디터실행_검증.txt` | |
| 수정 (MT-1 와 같이 아직 미추적) | `Build/BatchFiles/MainTree.ps1`(UTF-8 BOM) · `Build/BatchFiles/MainTree.cmd` | MT-1 체크리스트의 같은 파일 — 한 번에 `git add` |
| 문서 (수정) | `DevOps/Files/게임프로젝트_생성가이드.md`(3절), `DevOps/TODO.md` · `현황.md`, `진행현황.md` | |

## 커밋 체크리스트 (ER-024 — 완료, `7b8ee6e`)

| 구분 | 파일 | 비고 |
|---|---|---|
| 소스 (수정) | `Source/Programs/JGGameLaunch/JGGameLaunch.cpp` | 폴더째 아직 미추적 — GP 체크리스트의 새 파일 `Source/Programs/JGGameLaunch/` 와 같이 `git add` |
| 재빌드 Bin | `Bin/DevelopEngine/JGGameLaunch.dll` — 넣지 않음(D-40, `Bin/` 추적 그만둠) | 패키지는 쿠킹이 ReleaseGame 으로 다시 빌드 |
| 도구 · 문서 | 새 `DevOps/Files/tools/package_multi_launch.ps1` · `DevOps/Files/2026-10-11_ER-024_패키지_로그파일_검증.txt`, `DevOps/Files/게임프로젝트_생성가이드.md`, `DevOps/TODO.md` · `현황.md`, `진행현황.md` | |

## 커밋 체크리스트 (AU-1 ~ 3 · GM-B5 — 완료, `7b8ee6e` · Audio 본체 `ee36bb1`)

| 구분 | 파일 | 비고 |
|---|---|---|
| 소스 (수정) | `Source/Programs/JGConsole/Main.cpp`(Audio 연결 줄 — Core 가 넣음, Core 의 Audio 커밋 묶음과 같이) | Audio 모듈 없이 커밋하면 JGConsole 실행마다 연결 실패 — `Source/Runtime/Audio/` · `Source/ThirdParty/miniaudio/` 와 같이 |
| 템플릿 (수정) | `Build/Templates/GameProject/GenerateProjectFiles.bat` | GM-B5 |
| 문서 | `DevOps/Files/게임프로젝트_생성가이드.md`, 새 `DevOps/Files/2026-10-11_Audio_가이드_템플릿배치_검증.txt`, `DevOps/TODO.md` · `현황.md`, `진행현황.md` | |

## 커밋 체크리스트 (MT-1 — 완료, `7b8ee6e`)

| 구분 | 파일 | 비고 |
|---|---|---|
| 새 파일 (`git add` 필요) | `Build/BatchFiles/MainTree.cmd` · `Build/BatchFiles/MainTree.ps1`, `Document/Memory/DevOps/Files/2026-10-03_MainTree_도우미_검증.txt` | 잠금 · 기록은 `Temp/MainTree/`(이미 git 무시) |
| 문서 (수정) | `DevOps/TODO.md` · `현황.md`, `진행현황.md`(DevOps 행 · 7절) | 지침서 §3 한 단락은 총괄이 넣는다 |

## 커밋 체크리스트 (GP-0 설계 + GP-D0 ~ D6 — 완료, `7b8ee6e` · Core 파일 `ee36bb1` · `.gitignore` `37cffb3`)

| 구분 | 파일 | 비고 |
|---|---|---|
| 소스 (수정) | `Source/Runtime/Core/CoreDefines.h` · `CoreSystem.h/.cpp` · `FileIO/FileHelper.h/.cpp` · `FileIO/ProjectDescriptor.h/.cpp` · `Misc/Log.h/.cpp` · `Misc/Module.cpp` · `Platform/Platform.h/.cpp`, `Source/Programs/JGBuildTool/Class/BuildTool.h/.cpp` · `Class/ModuleInfo.h` · `Template/BuildTemplate.lua` | Core 를 고쳐 모든 DLL · exe 가 다시 링크됨 |
| 소스 (새 — `git add` 필요) | `Source/Runtime/Core/FileIO/PackageDescriptor.h/.cpp`, `Source/Programs/JGGameBootstrap/`(module.json · Main.cpp), `Source/Programs/JGGameLaunch/`(module.json · JGGameLaunch.cpp), `Source/Runtime/GameHost/`(4), `Source/Programs/JGConsole/Cook/`(3) | |
| 소스 (수정, 10-03 추가분) | `Core/Misc/Module.h/.cpp`(HRuntimeModuleInfo) · `CoreSystem.h/.cpp`(RuntimeModulesOnly · bExitRequested), `JGBuildTool/Class/BuildTool.h/.cpp`(writeRuntimeModuleInfo), `JGLauncher/Main.cpp`(-game), `JGConsole/Main.cpp`(-runCook — DevConsole D-30 -game 과 같은 파일), 템플릿 2 | |
| 생성물 · 설정 | `jgengine.lua` · `Source/Programs/JGBuildTool/jgengine.lua` · `JGEngine.sln`(16 프로젝트), `.gitignore`(GUI 의 imgui.ini 줄과 같은 파일) | |
| 재빌드 Bin | `Bin/DevelopEngine/*` — 넣지 않음(D-40, `6b9d212` 부터 `Bin/` 추적 그만둠 — 받은 쪽이 소스에서 빌드) | — |
| 문서 (새) | `DevOps/Files/게임빌드파이프라인_설계_2026-10-02.md`, `DevOps/Files/2026-10-02_GP-D0-D2_구현_검증.txt`, `DevOps/Files/2026-10-03_GP-D3-D6_구현_검증.txt`, `DevOps/Files/tools/gamehost_e2e.ps1`, `Document/DevOps_게임빌드파이프라인_설계_2026-10-02.html` | |
| 문서 (수정) | `DevOps/TODO.md` · `현황.md`, `Document/Memory/진행현황.md`(DevOps 행 · D-5 · D-23 · 4절 · 7절) | 진행현황은 다른 트랙도 고친다 |

## 커밋 체크리스트 (C-1 · BS-0 · BS-1~3 · 분야 이름 변경 — 완료, `0bd6356` 10-02 20:10, `Bin/` 제외)

| 구분 | 파일 | 비고 |
|---|---|---|
| 빌드 설정 (BS-1 · BS-2) | `Source/Programs/JGBuildTool/Template/BuildTemplate.lua`, `Source/PCH/PCH.h`, 재생성 `jgengine.lua` · `Source/Programs/JGBuildTool/jgengine.lua`(템플릿 4줄씩), 재빌드 `Bin/DevelopEngine/*`(PCH 가 바뀌어 모든 DLL · exe · lib · `Core.idb`) | 실행 부산물(`jg_log.txt` · `imgui.ini`)은 각 트랙 규칙대로 |
| 지침 (BS-3) | `CLAUDE.md` 3절 | |
| 증적 | `DevOps/Files/2026-10-02_BS1-BS2-BS3_적용_검증.txt` | |
| 템플릿 (C-1) | `Build/Templates/GameProject/Source/{PROJECT_NAME}/{PROJECT_NAME}.module.json`, `Source/{PROJECT_NAME}Editor/{PROJECT_NAME}Editor.module.json`, `Source/{PROJECT_NAME}/{PROJECT_NAME}Module.cpp`(주석) | 이미 만든 게임 프로젝트(ProjectAH 등)는 템플릿을 다시 읽지 않는다 — 같은 헤더를 쓰려면 각자 `module.json` 에 직접 |
| 문서 (이동) | `Document/Memory/GameModule/**` → `Document/Memory/DevOps/**` | `git add -A` 로 이름 변경이 잡힌다 |
| 문서 (새) | `DevOps/Files/빌드속도_검토_2026-10-02.md`, `DevOps/Files/tools/build_timing/`(3), `Document/DevOps_빌드속도_검토_2026-10-02.html` | |
| 문서 (수정) | `DevOps/현황.md` · `TODO.md` · `Files/게임프로젝트_생성가이드.md` · `Files/게임모듈_사전작업_분석_2026-09-29.md`, `CLAUDE.md`(분야 목록), `Document/Memory/지침서.md` · `README.md` · `진행현황.md`, 다른 분야 현황/TODO 의 `GameModule/` 경로 9개, 보고서 HTML 3개(`GUI_` · `Graphics_` · `JGEngine_진행보고_2026-09-30.html`) 링크 | 진행현황 · README 는 다른 트랙도 고친다 |

BS-0 자체는 코드 · 설정 변경이 없었다(측정은 worktree). BS-1 · BS-2 는 위 "빌드 설정" 행.

## 커밋 체크리스트 (GM-C2 — 완료, 사용자 커밋 `736ddbe` 2026-10-01 12:16)

| 구분 | 파일 | 비고 |
|---|---|---|
| 소스 (수정) | `Source/Runtime/Core/FileIO/FileHelper.cpp`, `Source/Runtime/Asset/AssetPath.cpp`, `Source/Runtime/Asset/AssetDatabase.cpp`, `Source/Programs/JGBuildTool/Main.cpp`, `Source/Programs/JGBuildTool/Class/ModuleInfo.h`, `Source/Programs/JGBuildTool/Class/ProjectCreator.h`, `Source/Programs/JGBuildTool/Class/ProjectCreator.cpp` | R7 · GM-A1 · GM-B1 · P4 |
| 소스 (신규) | `Source/Programs/JGConsole/ModuleCommands.cpp` | R8. **미추적 파일 — `git add` 필요** |
| 삭제 | `Source/Runtime/Asset/Core.module.json` | GM-B2 |
| 배치 | `GenerateProjectFiles.bat`(엔진 루트) | GM-B4 |
| 재빌드된 Bin | `Bin/DevelopEngine/*.dll` · `*.exe` · `*.lib` · `*.exp` · `Core.idb`(git status 의 Bin 수정분 전부) | `Core.lib`(FileHelper.cpp) 이 바뀌어 모든 DLL · exe 가 다시 링크됐다. **`JGBuildTool.exe` 는 P4 검사가 든 새 exe** — 배치가 이 exe 를 부른다 |
| 생성물 | `jgengine.bat`(엔진 절대경로만 `C:/Develop/JGEngine`), `Source/Programs/JGBuildTool/ModuleInfoTemplate.json`(GM-B1 로 재생성) | |
| 문서 | `Document/Memory/DevOps/현황.md`, `TODO.md`, `Files/2026-09-30_R7_게임Content_검증.txt`, `Files/2026-09-30_GameModule_마무리_검증.txt`, `Files/2026-09-30_R4_Game모듈_삭제_검증.txt`, `Files/게임프로젝트_생성가이드.md`, `Files/게임모듈_사전작업_분석_2026-09-29.md`(R4 · D5 행), `Document/Memory/진행현황.md` · `README.md`(GameModule 행 · 변경 로그), `Document/Memory/Etc/Files/DevConsole_TODO.md` · `Etc/TODO_DevConsole.md`(R8 행 완료 표시) | 진행현황 · README 는 다른 트랙도 고친다 |
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
| 문서 | `Document/Memory/DevOps/현황.md`, `TODO.md`, `Files/**`(이동 후 경로: 분석 · 가이드 · 구현검증 · 캡처 2장 · 인수인계 · 재현 결과 · `Files/tools/gameproject_repro/`) | 다른 트랙 문서 수정분(GUI_TODO 3-6, Memory_TODO 3-2, DevConsole_TODO R8 행)은 그 트랙 커밋과 겹칠 수 있음 |

넣지 않는 것(다른 트랙): `Source/Programs/JGConsole/Main.cpp` · `NetCommands.cpp` · `ConsoleCommandSelfTest.cpp`(DevConsole · Network), `Source/Editor/DevConsole/**`(DevConsole), `Build/BatchFiles/jg_log.txt` · `Bin/DevelopEngine/jg_log.txt`(로그). `Bin/DevelopEngine` 의 다른 dll · lib 는 저장소 관례(추적 중)대로.

커밋 뒤 확인: 다른 폴더에 클론(또는 `Temp/` 를 지운 상태) → 엔진 `GenerateProjectFiles.bat` → 빌드 → `현황.md` §6 의 1–3 · 6. 첫 배치 2 실행 전 엔진 코드젠이 없으면 `engine code generation is missing` 으로 종료 코드 1 이 나와야 정상.
