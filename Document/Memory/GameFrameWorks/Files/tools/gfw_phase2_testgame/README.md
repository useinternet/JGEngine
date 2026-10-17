# GFW Phase 2 화면 검증용 게임 (GfwView)

엔진 밖 검증용 게임 코드다. 씬 뷰포트 · Gameplay DevView(둘 다 JGEditor 위젯, 2026-09-30 C-3 에서 GFW 로부터 이관) · 피킹을 게임 프로젝트 에디터에서 확인할 때 쓴다. 게임 콘텐츠가 아니라 검증 장치다.

| 파일 | 내용 |
|---|---|
| `GfwViewEntryActor.cpp` | 6x6 사각 보드, 엔티티 4개(X Bot 메시 + 메시 경계 상자 판정, 컴포넌트 `GfwViewUnit`), 칸 타일 36개(절차 생성 메시), 궤도 카메라(칸 (3, 3) 중심), 컨트롤러. 검증 스크립트 액터: 3초 뒤 EndTurn, 5초 뒤 에디터 안에서 `gmtest` |
| `GfwViewModule.cpp` | 템플릿 그대로(GFW 연결 → 엔트리 액터 → 월드 로드). 창은 에디터가 가진다 — 게임 모듈은 에디터 위젯을 참조하지 않는다 |
| `GfwView.module.json` | 템플릿 의존성(`Core`, `GameFrameWorks`)에 `Asset`, `Graphics`, `GUI` 추가. GFW 카메라 · 메시 컴포넌트 헤더가 이 모듈들 헤더를 include 하고 JGBuildTool 은 include 경로를 전이하지 않기 때문 |
| `imgui.ini` | 창 배치 (씬 뷰포트 왼쪽 1300x810, DevView 오른쪽) + 열린 위젯 목록 `[JGWidget][Open]` `JGGameplayDevView=1`(DevView 를 시작부터 연다, 2026-10-03). 게임 `Bin/DevelopEngine/` 에 둔다 |

## 재현

`<dir>` 는 저장소 `Temp/Games`(지침서 §3, 2026-10-02 D-20 — `%TEMP%` 아래는 MSBuild 가 헤더 변경을 추적하지 않는다). 2026-10-02 C-1 뒤 새 템플릿에는 `Asset` · `Graphics` 가 들어 있어 `GfwView.module.json` 복사는 없어도 된다(이 파일은 그 위에 `GUI` 만 더하고, 지금 게임 코드는 GUI 헤더를 include 하지 않는다).

```
Build\BatchFiles\CreateGameProject.bat <dir>\GfwView GfwView
copy 이 폴더의 .cpp · .module.json → <dir>\GfwView\Source\GfwView\  (GfwViewEditor.module.json 은 그대로 둬도 된다)
<dir>\GfwView\GenerateProjectFiles.bat
MSBuild <dir>\GfwView\GfwView.sln -m -p:Configuration=DevelopEngine -p:Platform=x64      (처음 약 5분)
copy imgui.ini → <dir>\GfwView\Bin\DevelopEngine\      (실행마다 다시 — 아래 "imgui.ini")
powershell -File ..\gfw_worldview_verify.ps1 -BinDir <dir>\GfwView\Bin\DevelopEngine -OutPrefix <out>\run -WaitSeconds 30 -Clicks "1000,1000;658,417;1400,233;1347,103;948,598" -RightDrag "500,400,600,400"
```

씬 뷰포트는 에디터가 프로젝트 모드에서 자동으로 연다. DevView 는 `imgui.ini` 의 열린 위젯 목록(`[JGWidget][Open]` `JGGameplayDevView=1`)으로 시작부터 열린다 — 로그 `[GUI]: Layout: reopened JGGameplayDevView`.
메뉴로 열지 않는 이유(2026-10-03): 예전 순서는 `Windows` 메뉴(30, 9) → 항목 `Gameplay DevView`(60, 85) 를 눌렀는데, 10-03 JGEditor 세션 실행(125%)에서 (30, 9) 가 `GfwView` 메뉴를 열어 DevView 가 열리지 않았다. 메뉴 막대 항목 순서 · 폭에 기대는 좌표라 레이아웃 복원으로 바꿨다. 같은 실행에서 이 방식으로 피킹 E3.1 · 칸 (3, 3) / 빈 칸 (5, 0) 숫자가 09-30 과 같고 DevView 선택 · Undo 정상(`Document/Memory/Etc/Files/2026-10-03_GP-E1_D-22_verify.txt` run2). 메뉴 자체를 확인할 때는 그 실행 캡처로 메뉴 막대 좌표를 먼저 잰다.
클릭 좌표는 앱(ImGui) 좌표다. 이 `imgui.ini` 배치 · 화면 125% 에서: 포커스용 빈 곳(1000, 1000) · 씬 뷰포트 이미지 가운데(658, 417) · DevView 엔티티 E3.1 줄(1400, 233) · Undo 버튼(1347, 103) · 오른쪽 빈 칸(948, 598).
DevView 두 좌표는 2026-10-03 D-22 ② 메인 반영(19:1x, 에디터 DPI 인식 + 글꼴 배율)으로 바뀌었다: 125% 에서 에디터 글꼴이 13 → 16px(맑은 고딕)이 되어 목록 줄 간격이 약 21px 가 됐고, 옛 (1400, 190) 은 E1.1 을 누른다. 옛 좌표(그 전 빌드): E3.1 (1400, 190) · Undo (1346, 83). 다른 배율에서는 첫 실행 캡처로 다시 잰다(글자 높이에 기대는 좌표).
포커스가 없는 앱의 첫 클릭은 포커스만 옮기므로 빈 곳을 먼저 누른다.

imgui.ini: 에디터는 닫힐 때 창 배치와 열린 위젯 목록(`[JGWidget][Open]`)을 실행 폴더 `imgui.ini` 에 쓴다. 실행 중 창이 옮겨지거나 다른 위젯이 열리면 다음 실행 배치가 달라져 클릭 좌표가 어긋난다 → 실행마다 이 폴더의 `imgui.ini` 를 다시 복사한다.
화면 배율: 10-03 D-22 ② 메인 반영 뒤 에디터는 DPI 인식(모니터별)이라 앱 좌표 = 실제 픽셀이다(125% 클라이언트 2402x1352). 스크립트가 창의 DPI 인식을 보고 맞춘다. 피킹 숫자는 E3.1 at (601.40, 25.18, 582.83) distance 1770.67 / 칸 (3, 3) at (608.78, 0.00, 602.73), 빈 칸 (5, 0) at (948.41, 0.00, 31.01) — 같은 앱 좌표가 이미지의 (0.503, 0.497) 를 눌러 그 전 빌드의 (0.500, 0.500) 숫자(593.75 … / (600.03, 0.00, 600.10))와 조금 다르다(같은 엔티티 · 같은 칸). 그 전(비인식) 빌드에서는 앱 좌표가 논리 픽셀이라 창 크기가 화면 배율에 따라 달랐다(125% → 1920x1080, 150% → 1711x1048). 150% 에서는 DevView(Pos 1320, 너비 590)가 본 창 밖으로 걸쳐 ImGui 가 따로 OS 창으로 띄운다 — 클릭 좌표는 그대로 맞고, 스크립트는 같은 프로세스 창이면 누르고 캡처에 모든 창을 모아 그린다(2026-10-01).

기대 결과 (`Bin\DevelopEngine\jg_log.txt`):

```
GfwView script: EndTurn by E0.1 -> result 1 ()
game instance section skipped: world GfwView is already loaded
GfwView script: gmtest in editor -> OK, game world kept, actors 45 -> 45
Controller click: actor Entity_E3.1 entity E3.1 at (...) / board cell (3, 3) at (600.03, 0.00, 600.10)
Controller click: actor none / board cell (5, 0) at (938.00, 0.00, 33.77)
```

캡처: DevView 요약 `Sequence 1 | Round 1 | Step TurnMain | Input E1.1 | Turns 2 | Entities 4`(2026-10-01 흐름 공용 칸 — 단계 이름 · 입력 행동자), E3.1 선택 → `Zone: Units`, `Board: (3, 3)`, `[GfwViewUnit] {"Value": {"Health": 25, "Team": 1}}`, Undo 뒤 `Sequence 0 | … | Input E0.1 | Turns 1` 과 이벤트 로그 `-- state replaced`. DevView 를 메뉴로 늦게 열면 이벤트 로그는 `-- watching GameMaster` 부터다(시작 3초의 EndTurn 이벤트는 창을 연 뒤에만 쌓인다).
로그의 `[error]` 2줄은 에디터 안 `gmtest` 가 일부러 내는 것(R5 · §13 단계 한도), `[warning]` 은 기준선(`[Asset] NOT Support Asset Path`, Undo 가드, 월드 테스트 건너뜀, DevView 가 따로 창일 때 Graphics `ID3D12CommandQueue1::Wait` 1줄).
마지막 실행 기록: `Document/Memory/Etc/Files/2026-10-03_D-22-2_merged_verify.txt` §3(2) · §7(2026-10-03 D-22 ② — 작업 폴더 빌드와 메인 반영 뒤, 새 DevView 좌표, 캡처 `Etc/Files/2026-10-03_D-22-2_main_gfwview_devview_undo.png`). 이전: `Document/Memory/GameFrameWorks/Files/2026-10-01_ER-009_verify.txt`, `Document/Memory/Etc/Files/2026-09-30_jgeditor_C-3_verify.txt`.
