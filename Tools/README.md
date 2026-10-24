# 검증 · 운영 도구 (Tools)

엔진 코드가 아닌 도구 모음이다. 빌드(`Build/`)나 프로젝트 파일 생성(JGBuildTool 은 `Source/` 만 훑는다)과 상관없다.
2026-10-24 `Document/` 개편 때 옛 자리(`Document/Memory/tools/`, `Document/Memory/<분야>/Files/tools/`)에서 쓰는 것만 옮겼다(보고함 H-079). 옮기지 않은 도구는 `git show 2588c3e:Document/Memory/<옛 경로>` 로 꺼낸다.

## 지키는 것

- 빌드 결과(`*.exe` · `*.obj` · `out/`)는 git 에 넣지 않는다(`.gitignore`). 처음 쓸 때 그 폴더의 `build.bat` 을 한 번 돌린다.
- 메인 Bin(`Bin/DevelopEngine`)을 띄우거나 빌드하는 도구는 `Build\BatchFiles\MainTree.cmd -name=<세션> exec <도구> [인자]` 로 감싼다(CLAUDE.md 3절).
- `.ps1.txt` 는 Windows Defender 가 격리하지 않게 이름만 바꿔 둔 PowerShell 스크립트다. `$TEMP` 등에 `.ps1` 로 복사해 실행한다.
- 한글이 든 `.ps1` 은 UTF-8 BOM 을 지킨다(BOM 이 없으면 PowerShell 5.1 이 한글 주석 뒤 줄을 삼킨다).
- `uitest_hook` 패치는 스크래치 워크트리에만 넣는다. 메인 트리에 넣지 않는다.

## 폴더

| 폴더 | 담당 | 무엇 |
|---|---|---|
| `Common/` | DevOps | 여러 분야의 검증 루프가 함께 쓰는 도구 |
| `Ops/` | 엔진 총괄 | 팀 운영 — 대화 압축 감시, 위키 그림 그리개 |
| `Core/` | Core | 메모리 · 오디오 · 입력 검증 |
| `DevOps/` | DevOps | 게임만 실행 · 패키지 · RunEditor 확인 |
| `Editor/` | Editor | 에디터 창 · DPI · 콘솔 입력 확인 |
| `GameFrameWork/` | GameFrameWork | 게임 UI · 월드 뷰 · DataTable 화면, 리뷰 재현, 네트워크(리슨 서버) |
| `Graphics/` | Graphics | 창 크기 · 디버그 출력 · 시험 FBX |

## 도구

| 도구 | 하는 일 | 부르는 법 |
|---|---|---|
| `Common/crashwalk/` | 대상을 디버거로 띄워 예외가 나면 콜스택을 찍고, 정한 초 뒤 WM_CLOSE 로 닫아 종료 코드를 돌려준다 | `build.bat` → `crashwalk.exe "<exe> [인자]" <작업 폴더> <닫기 초> [종료 행 초=30]` |
| `Common/capture_devscene.ps1` | 런처(JGLauncher)를 띄워 에디터 창을 캡처하고 WM_CLOSE | `MainTree.cmd -name=<세션> exec Tools\Common\capture_devscene.ps1 [-WaitSeconds 30] [-OutPath x.png] [-WorkDir 폴더]` |
| `Common/uitest_hook/` | 워크트리 전용 ImGui 훅 — `JG_UITEST=1` 이면 PostMessage 로 보낸 마우스만 쓴다(창 드라이버의 클릭용) | 주석 줄을 빼고 `git apply` |
| `Ops/compaction_watch.js` | 엔진 분야 세션의 대화 압축 수 감시 | `node Tools/Ops/compaction_watch.js [--list]` |
| `Ops/wiki_svg/` | 엔진 위키 그림 그리개 | `GUIDE.md` 먼저. `node render.js <문서키>` (위키 대신 다른 폴더로: `WIKI_SVG_OUT=<폴더>`) |
| `Core/audiocheck/` | 오디오 재생 검증 장치(게임 모듈 · 출력 레벨 측정 · 판정) | `README.md` |
| `Core/measure_resident.ps1.txt` | 런처 상주 메모리 · CPU 시간 표본 | `.ps1` 로 복사 → `-WaitSeconds 25` |
| `Core/memlog_stats.sh` | `JG_MEMORY_TRACE` 빌드 로그를 리플레이해 풀 사용 통계 | `bash Tools/Core/memlog_stats.sh <jg_log 사본>` |
| `Core/memstats_capture.ps1.txt` | 메모리 위젯 캡처(풍선 도움말 위치 포함) | `.ps1` 로 복사 → `-BinDir` · `-OutPrefix` |
| `Core/sharedptr_semantics_test.cpp` | `PSharedPtr` · `PWeakPtr` 대입 의미를 std 와 비교 | 파일 머리 주석(clang++) |
| `Core/gp_c1_window_check.ps1.txt` | `JGConsole input.selftest -window` 창에 입력을 보내 이벤트 확인 | `.ps1` 로 복사 → `-BinDir` · `-OutFile` |
| `DevOps/gamehost_e2e.ps1` | 게임만 실행(`JGLauncher -game` · 패키지 exe) 창 캡처 · 클릭 · 종료 코드 | `-Exe <exe> -OutDir <폴더>` |
| `DevOps/package_multi_launch.ps1` | 패키지 exe 를 여러 개 동시에 띄워 로그가 갈리는지 확인 | `-Package <패키지 폴더> [-Count 3]` |
| `DevOps/runeditor_check.js` | `RunEditor.cmd` 의 8가지 경우 확인(메인 트리가 비었을 때) | `node Tools/DevOps/runeditor_check.js` |
| `Editor/devconsole_ui_runner.ps1.txt` | 런처 창에 동작 목록(click · type · key · shot · wait)을 보내는 러너. 클릭은 `uitest_hook` 필요 | `.ps1`(BOM) 로 복사 → `-Exe` · `-WorkDir` · `-OutPrefix` · `-Actions` |
| `Editor/devconsole_ui_check.ps1.txt` | DevConsole 입력 고정 시나리오(help · 오류 줄 · 기록 복원 등) | `.ps1` 로 복사 → `-Exe` · `-WorkDir` · `-OutPath` |
| `Editor/editor_dpi_check.ps1` | 에디터 DPI 인식 · 창 크기 확인 | `-BinDir` · `-OutPrefix` |
| `Editor/editor_dpi_before_after.ps1` | 두 모니터(125% · 150%)에서 에디터 캡처 | `-BinDir` · `-OutPrefix` |
| `Editor/sceneviewport_focuslost_check.ps1` | 포커스 잃음 A~D(GameUIDemo) | `-BinDir` · `-OutDir` |
| `Editor/sceneviewport_resize_check.ps1` | 창을 빠르게 줄여 뷰포트 크기 맞춤 확인 | `-BinDir` · `-OutPrefix` |
| `GameFrameWork/gameui_demo_testgame/` | 게임 UI 검증 게임 + `gameui_e2e.ps1` | `README.md` |
| `GameFrameWork/gfw_phase2_testgame/` | 씬 뷰포트 · DevView · 피킹 검증 게임 | `README.md` |
| `GameFrameWork/gfw_worldview_verify.ps1` | 월드 뷰 클릭 · 끌기 캡처 | `-BinDir` · `-OutPrefix` · `-Clicks` |
| `GameFrameWork/gfw_review_repro/` | 리뷰 재현 — 워크트리에서 JGConsole 에 넣어 `simreview` | 파일 머리 |
| `GameFrameWork/datatable_ui_driver.ps1.txt` | DataTable 창 입력 드라이버. `uitest_hook` 필요 | `.ps1` 로 복사 |
| `GameFrameWork/net_duel_testgame/` | 리슨 서버 결투 검증 게임 | `README.md` |
| `GameFrameWork/net_lobby_check/` | 세션 로비 창 검증. `uitest_hook` 필요 | `README.md` |
| `GameFrameWork/net_two_process/run_two_process.sh` | JGConsole 여러 프로세스를 TCP 로 붙여 결정론 확인 | `bash … <Bin/DevelopEngine> [포트] [명령 수] [출력 폴더] [kernel\|world] …` |
| `Graphics/gp_window_test.ps1` | 창 크기 · 최소화 · Alt+Enter 확인 | `-Exe` · `-WorkDir` |
| `Graphics/dbgevents/` | 디버그 이벤트 · OutputDebugString 세기 | `build.bat` → `dbgevents.exe <exe> <작업 폴더> <닫기 초>` |
| `Graphics/fbx_embedded_texture/` | 임베디드 텍스처 시험 FBX 생성기(스스로 다시 읽어 확인) | `build.bat <출력 폴더>` → `make_embedded_texture_fbx.exe <출력 .fbx>` |
