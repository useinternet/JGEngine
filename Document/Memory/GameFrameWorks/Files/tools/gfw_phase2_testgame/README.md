# GFW Phase 2 화면 검증용 게임 (GfwView)

엔진 밖 검증용 게임 코드다. 씬 뷰포트 · Gameplay DevView(둘 다 JGEditor 위젯, 2026-09-30 C-3 에서 GFW 로부터 이관) · 피킹을 게임 프로젝트 에디터에서 확인할 때 쓴다. 게임 콘텐츠가 아니라 검증 장치다.

| 파일 | 내용 |
|---|---|
| `GfwViewEntryActor.cpp` | 6x6 사각 보드, 엔티티 4개(X Bot 메시 + 메시 경계 상자 판정, 컴포넌트 `GfwViewUnit`), 칸 타일 36개(절차 생성 메시), 궤도 카메라(칸 (3, 3) 중심), 컨트롤러. 검증 스크립트 액터: 3초 뒤 EndTurn, 5초 뒤 에디터 안에서 `gmtest` |
| `GfwViewModule.cpp` | 템플릿 그대로(GFW 연결 → 엔트리 액터 → 월드 로드). 창은 에디터가 가진다 — 게임 모듈은 에디터 위젯을 참조하지 않는다 |
| `GfwView.module.json` | 템플릿 의존성(`Core`, `GameFrameWorks`)에 `Asset`, `Graphics`, `GUI` 추가. GFW 카메라 · 메시 컴포넌트 헤더가 이 모듈들 헤더를 include 하고 JGBuildTool 은 include 경로를 전이하지 않기 때문 |
| `imgui.ini` | 창 배치 (씬 뷰포트 왼쪽 1300x810, DevView 오른쪽). 게임 `Bin/DevelopEngine/` 에 둔다 |

## 재현

```
Build\BatchFiles\CreateGameProject.bat <dir>\GfwView GfwView
copy 이 폴더의 .cpp · .module.json → <dir>\GfwView\Source\GfwView\  (GfwViewEditor.module.json 은 그대로 둬도 된다)
<dir>\GfwView\GenerateProjectFiles.bat
MSBuild <dir>\GfwView\GfwView.sln -m -p:Configuration=DevelopEngine -p:Platform=x64      (처음 약 5분)
copy imgui.ini → <dir>\GfwView\Bin\DevelopEngine\      (실행마다 다시 — 아래 "imgui.ini")
powershell -File ..\gfw_worldview_verify.ps1 -BinDir <dir>\GfwView\Bin\DevelopEngine -OutPrefix <out>\run -WaitSeconds 30 -Clicks "1000,1000;30,9;60,85;658,417;1400,190;1346,83;948,598" -RightDrag "500,400,600,400"
```

씬 뷰포트는 에디터가 프로젝트 모드에서 자동으로 연다. DevView 는 메뉴 `Windows/Gameplay DevView` 로 연다.
클릭 좌표는 앱(ImGui) 좌표다. 이 `imgui.ini` 배치에서: 포커스용 빈 곳(1000, 1000) · `Windows` 메뉴(30, 9) · 메뉴 항목 `Gameplay DevView`(60, 85) · 씬 뷰포트 이미지 가운데(658, 417) · DevView 엔티티 E3.1 줄(1400, 190) · Undo 버튼(1346, 83) · 오른쪽 빈 칸(948, 598).
포커스가 없는 앱의 첫 클릭은 포커스만 옮기고 메뉴를 열지 않으므로 빈 곳을 먼저 누른다.

imgui.ini: 에디터는 닫힐 때 창 배치와 열린 위젯 목록(`[JGWidget][Open]`)을 실행 폴더 `imgui.ini` 에 쓴다. 그대로 두고 다시 돌리면 DevView 가 시작부터 열려 있어 위 순서(메뉴로 열기)가 어긋난다 → 실행마다 이 폴더의 `imgui.ini` 를 다시 복사한다.
화면 배율: 앱 좌표는 화면 배율에 따라 창 크기가 달라진다(125% → 1920x1080, 150% → 1711x1048). 150% 에서는 DevView(Pos 1320, 너비 590)가 본 창 밖으로 걸쳐 ImGui 가 따로 OS 창으로 띄운다 — 클릭 좌표는 그대로 맞고, 스크립트는 같은 프로세스 창이면 누르고 캡처에 모든 창을 모아 그린다(2026-10-01).

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
마지막 실행 기록: `Document/Memory/GameFrameWorks/Files/2026-10-01_ER-009_verify.txt` (이전: `Document/Memory/Etc/Files/2026-09-30_jgeditor_C-3_verify.txt`).
