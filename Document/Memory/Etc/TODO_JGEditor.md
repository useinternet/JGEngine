# JGEditor TODO (에디터 호스트 · 게임 창)

갱신 2026-10-01 09:4x (E-2 레이아웃 복원). 현황 · 구조 · 함정 · 검증 방법은 `현황.md` §5.
범위: JGEditor 모듈(`Source/Editor/JGEditor/`) — 런처의 에디터 호스트(창 · 엔진 모듈 연결 · 게임 프로젝트 모듈 연결)와 게임 월드를 보는 에디터 창(씬 뷰포트 · Gameplay DevView).
사용자 결정(2026-09-30 밤): **위젯 작업은 JGEditor 가 하고 GameFrameWorks 는 순수 로직을 제공한다.** 창이 부르는 월드 · 카메라 · 피킹 · GameMaster API 는 GFW 에 둔다.
기준: 엔진 기반 구축 · 과설계 금지. 지금 겪는 문제만 미완료 표에, 나머지는 보류 표에 "다시 볼 조건"과 함께 둔다.

---

## 다음에 할 일

| 순서 | 항목 | 착수 조건 | 완료 기준 |
|---|---|---|---|
| 1 | **E-C1. 커밋** (사용자) | GFW Phase 2 커밋(GFW TODO 1-9)과 같은 커밋 또는 그 뒤. 아래 커밋 체크리스트 | 클린 체크아웃에서 PreBuild → 빌드 → 게임 프로젝트 에디터에 씬 뷰포트가 뜬다 |
| 2 | **ProjectAH 다시 빌드** (사용자) | E-2 가 엔진 `GUI` · `JGEditor` 를 바꿨다. ProjectAH 는 자기 Bin 에 엔진 DLL 을 따로 빌드한다 | ProjectAH 솔루션 빌드 뒤 창을 한 번 메뉴로 열면, 그다음 재시작부터 열린 창 · 배치가 그대로 |

구현할 미완료 항목은 없다. 새 요구는 사용자가 정한다(아래 보류 표 참고).

## 미완료 항목

| ID | 항목 | 상태 | 선행조건 | 완료 기준 |
|---|---|---|---|---|
| E-C1 | 커밋 (사용자) | 대기 | GFW 1-9 | 위 표 |

## 보류 (다시 볼 조건이 오면 착수)

| ID | 항목 | 지금 상태 | 다시 볼 조건 |
|---|---|---|---|
| E-H1 | 씬 뷰포트 렌더 해상도가 1280x720 고정 | 창 크기에 비율 유지로 줄여 보인다(최소 640x360). G버퍼를 창마다 다시 만들지 않으려는 GFW 쪽 원래 설계 | 창을 크게 써서 흐림이 문제되거나 16:9 가 아닌 화면이 필요할 때 |
| E-H2 | Gameplay DevView 자동 열기 | 메뉴 `Windows/Gameplay DevView` 로만 연다. 창을 연 뒤부터 이벤트를 받는다(이관 전에는 검증 게임이 시작 때 열었다) | 게임 제작 중 시작 이벤트까지 봐야 하거나, 사용자가 프로젝트 모드 기본 창으로 원할 때 |
| E-H3 | 에디터에서 월드 재시작(Play/Stop) | 없음. 월드는 게임 모듈이 시작할 때 한 번 로드된다 | 게임을 고치며 월드를 다시 올리는 일이 반복돼 에디터 재실행이 부담될 때 |
| E-H5 | 에디터 OS 창 크기 · 위치 · 최대화 저장 | 없음. 창은 항상 1936x1119(논리)로 만들고, 화면보다 크면 Windows 가 줄인다(10-01 이 PC 150% 화면에서 클라이언트 1711x1048). 도킹 칸은 비율로 맞춰지므로 배치는 유지된다 | 사용자가 창을 최대화 · 이동해 쓰는데 매번 원래 크기로 떠서 불편할 때(`HJWindowArguments` 와 창 배치 저장 위치를 같이 정한다) |
| E-H4 | JGHeaderTool 이 지운 JGCLASS 헤더의 생성 파일(`Temp/CodeGen/<Module>/<Name>.generation.*`)을 남긴다 | 이번 이관에서 겪음: 남은 `WorldView.generation.cpp` 가 premake 에 잡혀 GameFrameWorks 빌드가 없는 헤더를 찾는다. 손으로 지우고 PreBuild 재실행으로 해결(`현황.md` §5 함정) | 도구 소관(GameModule 트랙의 `JGHeaderTool`). JGCLASS 헤더를 또 옮기거나 지울 때, 또는 다른 세션이 같은 오류를 만날 때 |

## 완료 이력

| ID | 항목 | 완료일 | 검증 근거 |
|---|---|---|---|
| E-1 | GFW TODO **C-3 위젯 이관**: `JGWorldView` → `JGSceneViewport`(창 "Scene Viewport"), `JGGameplayDevView` 는 이름 그대로 `Source/Editor/JGEditor/Widgets/` 로. 메뉴 `Windows/Scene Viewport` · `Windows/Gameplay DevView` 는 에디터가 등록, 씬 뷰포트는 프로젝트 모드(게임 모듈이 GFW 연결)에서만 자동으로 연다. GFW 는 GUI 의존 · 창 등록 삭제 | 2026-09-30 22:4x (미커밋) | `Files/2026-09-30_jgeditor_C-3_verify.txt` — GFW · JGEditor 빌드 exit 0, `gmtest` 156/156, 프로젝트 없는 에디터 종료 0(메뉴 캡처 `Files/2026-09-30_jgeditor_windows_menu.png`), 검증 게임 에디터에서 클릭 → E3.1 · 칸 (3, 3) / (5, 0) 이관 전과 같은 값, DevView 선택 · Undo, 궤도 끌기(`Files/2026-09-30_jgeditor_sceneviewport_devview.png`, `Files/2026-09-30_jgeditor_undo_orbit.png`) |
| E-2 | **에디터 레이아웃 복원**(사용자 보고 "레이아웃 저장이 안 되는 것 같다") = GUI TODO BL-4. 원인: 창 위치 · 도킹은 `imgui.ini` 에 저장되고 있었지만 열린 창 목록은 저장되지 않아, 재시작하면 기본 창만 열리고 도킹 칸이 접혔다. GUI 모듈이 열린 위젯을 `[JGWidget][Open]` 에 저장하고 첫 프레임에 다시 연다. 씬 뷰포트는 `OpenWidgetByDefault`(닫아 두면 안 엶) | 2026-10-01 09:4x (미커밋) | `Files/2026-10-01_layout_restore_verify.txt` — 빌드 5모듈 exit 0, 메인 트리 에디터 저장 → 재시작 복원, 사용자 ProjectAH ini 배치 그대로 복원(`Files/2026-10-01_layout_restored.png`), 닫은 기본 창 유지(`Files/2026-10-01_layout_closed_default_stays_closed.png`) |

## 커밋 체크리스트 (E-C1)

- 새 파일(미추적 — `git add` 필요): `Source/Editor/JGEditor/Widgets/{SceneViewport, GameplayDevView}.{h,cpp}`
- 수정: `Source/Editor/JGEditor/JGEditor.{h,cpp}`
- E-2(10-01): `Source/Runtime/GUI/GUIModule.{h,cpp}`(GUI 세션의 1-2 미커밋 변경과 같은 파일 — GUI 체크리스트 1-11 과 같이), 바이너리 `Bin/DevelopEngine/{GUI, DevConsole, DevStatistics, JGEditor}.{dll,exp,lib}`(09:20~09:21 빌드. 09:48 GUI 세션이 게임 UI 반영 뒤 메인 트리 전체를 다시 빌드했다 — 커밋에는 그 빌드 산출물. `GameGUI` 모듈은 그때 삭제됨), 문서 `Etc/Files/2026-10-01_layout_*`, `Etc/TODO_GUI.md` BL-4
- GFW 쪽(이관으로 지운 것 · 고친 것 — GFW 커밋 체크리스트 1-9 와 겹친다): `Source/Runtime/GameFrameWorks/Widgets/` 폴더 없음(미추적이었으므로 git 에서는 "추가 안 함"), `Core/GameFrameWorksModule.{h,cpp}`, `GameFrameWorks.module.json`(GUI 삭제), 주석만 바뀐 `Core/World.h` · `Core/WorldSelfTest.cpp` · `Components/CameraComponent.h` · `Actors/GameplayControllerActor.h`
- 바이너리: `Bin/DevelopEngine/{GameFrameWorks, JGEditor}.{dll,exp,lib}` (22:25~22:26 빌드)
- 재생성: `jgengine.lua`, `Source/Programs/JGBuildTool/jgengine.lua` (GFW 의 GUI include · link 삭제. 다른 세션의 `Game` 모듈 삭제분과 섞인다)
- 문서: `Document/Memory/Etc/{TODO_JGEditor.md, 현황.md §5, Files/2026-09-30_jgeditor_*}`, `Document/Memory/GameFrameWorks/{TODO.md, 현황.md, Files/tools/gfw_worldview_verify.ps1, Files/tools/gfw_phase2_testgame/}`, `Document/Memory/{진행현황.md, README.md}`
- 제외: `Bin/DevelopEngine/imgui.ini` · `jg_log.txt`(실행마다 바뀜), `Temp/`
