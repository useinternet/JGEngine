# 게임 UI 화면 검증용 게임 (GameUIDemo)

엔진 밖 검증용 게임 코드다(게임 콘텐츠가 아니라 검증 장치). GameFrameWorks 의 게임 UI(`UI/`, `JGGameWidget` · CommonUI식 레이어 스택)를 게임 모듈이 쓰고, 에디터 Scene Viewport(JGEditor)가 월드 위에 그려 포인터 · Esc 를 먼저 넘기는지 본다. 설계 `../../GameUI_설계_2026-10-01.md` §6 V2.

| 파일 | 내용 |
|---|---|
| `GameUIDemoHUD.h/.cpp` | `JGGameUIDemoHUD`(JGCLASS, Game 레이어 · 입력 모드 Game). 위 띠(반투명 패널, 입력 받음) + 턴 글자 + 두 줄 안내 + Menu 버튼(관리자 기본 스타일), 카드 뒷면 2장(엔진 `Content/RawResources/CardBack_Tree_{Dark,Gold}_63x88mm_300dpi.png`), 손패 글자, 비활성 Draw 버튼, 오른쪽 아래 주황 End Turn 버튼(직접 정한 스타일, 누르면 턴 +1). 오른쪽 위 줄바꿈 패널(ER-007: 폭 600 문단 `Word` · 잘라내기, 높이 = `MeasureContent`, 그 아래 말줄임 한 줄), 그 아래 입력란 2개(ER-008: 주소 = 숫자 · `.` · `:` 만 21자, 이름 16자, Enter 로 확정하면 로그) |
| `GameUIDemoPauseMenu.h/.cpp` | `JGGameUIDemoPauseMenu`(JGCLASS, Menu 레이어 · 입력 모드 Menu · 뒤로가기 처리). 화면을 어둡게 덮는 막 + 가운데 패널(제목 · 안내 · Resume 버튼). Resume 또는 Esc 로 닫힌다 |
| `GameUIDemoModule.h/.cpp` | GFW 연결 → 엔트리 액터 → 월드 로드 → `GetUI()->PushWidget<JGGameUIDemoHUD>(EGameUILayer::Game)`. 종료 때 `ClearAllWidgets()` |
| `GameUIDemoEntryActor.cpp` | 배경 월드: X Bot 정적 메시 + 궤도 카메라(헤더는 템플릿이 만든 것 그대로) |
| `GameUIDemo.module.json` | 템플릿 의존(`Core`, `GameFrameWorks`)에 `Asset`, `Graphics` 추가(JGBuildTool 은 include 경로를 전이하지 않는다). 별도 UI 모듈은 없다 |
| `gameui_e2e.ps1` | 화면 검증(UTF-8 BOM). 런처 실행 → 캡처 → 주황 End Turn 을 색으로 찾아 뷰포트 이미지 사각형을 역산 → 기준 해상도 좌표로 PostMessage 클릭 · Esc(1~10단계) → 입력란 클릭 · `WM_CHAR` 글자 · Backspace · Enter · 입력 중 Esc(11~20단계) → 캡처 4장(before · menu · input · after) → WM_CLOSE. 런처 창에만 메시지를 보낸다 |

## 재현

```
Build\BatchFiles\CreateGameProject.bat <dir>\GameUIDemo GameUIDemo
copy 이 폴더의 .h · .cpp · .module.json → <dir>\GameUIDemo\Source\GameUIDemo\   (GameUIDemoEntryActor.h · GameUIDemoEditor 는 템플릿 그대로)
<dir>\GameUIDemo\GenerateProjectFiles.bat
MSBuild <dir>\GameUIDemo\GameUIDemo.sln -m -p:Configuration=DevelopEngine -p:Platform=x64      (처음 약 4분)
powershell -ExecutionPolicy Bypass -File gameui_e2e.ps1 -BinDir <dir>\GameUIDemo\Bin\DevelopEngine -OutDir <out> -WaitSeconds 25
```

기대 결과 (`<dir>\GameUIDemo\Bin\DevelopEngine\jg_log.txt`, 순서대로):

```
[GameUI]: Font loaded : C:/Windows/Fonts/malgun.ttf      ← HUD 가 OnInitialize 에서 문단 높이를 재므로 먼저 읽힌다
[GameUIDemo]: GameUIDemo paragraph measured 598.8 x 207.5 (width 600)   ← 폭 ≤ 600, 높이 = 줄 수(6) × 줄 간격(맑은 고딕 26px). 글꼴이 다르면 값도 다르다
[GameUIDemo]: GameUIDemo HUD initialized: 9 root elements
[GameUI]: GameUI push JGGameUIDemoHUD on Game (stack 1)
[GameUIDemo]: GameUIDemo HUD activated
[GameUI]: Texture loaded : .../Content/RawResources/CardBack_Tree_Dark_63x88mm_300dpi.png (744x1039)   ← Gold 도 한 줄
1  [JGEditor]: SceneViewport Click (...): game UI (not picked)   + [GameUIDemo]: GameUIDemo End Turn clicked (turn 2)
2  [GameUIDemo]: GameUIDemo Menu clicked   + [GameUI]: GameUI push JGGameUIDemoPauseMenu on Menu (stack 1)   + pause menu activated
3  [JGEditor]: SceneViewport Click (...): game UI (not picked)   ← End Turn 이 막힘(턴 그대로)
4  [JGEditor]: SceneViewport Click (...): game UI (not picked)   ← 빈 곳도 막힘(월드 피킹 없음)
5  [JGEditor]: SceneViewport Back (Esc): handled by game UI   + [GameUI]: GameUI back handled by JGGameUIDemoPauseMenu   + GameUI remove ... (stack 0) + pause menu closed
6  [GameUIDemo]: GameUIDemo End Turn clicked (turn 3)
7  [JGEditor]: SceneViewport Click (...): nothing (no controller in the world)   ← 월드 피킹 경로
8  [JGEditor]: SceneViewport Click (...): game UI (not picked)   ← 비활성 Draw: 클릭 로그("Draw clicked while disabled") 없음
9  GameUI push JGGameUIDemoPauseMenu ... / 10  [GameUIDemo]: GameUIDemo Resume clicked + GameUI remove ... (stack 0)
11 [JGEditor]: SceneViewport Click (...): game UI (not picked)   ← 주소 입력란 포커스
14 [GameUIDemo]: GameUIDemo address committed: 192.168.0.10:4777   ← 'x' 는 허용 문자가 아니라 버려지고, Backspace 가 끝 '1' 을 지움
15 [JGEditor]: SceneViewport Click (...): game UI (not picked)
17 [GameUIDemo]: GameUIDemo name committed: 홍길동ab
18 [JGEditor]: SceneViewport Click (...): game UI (not picked)
19 (로그 없음)   ← 입력 중 Esc 는 입력란 포커스만 푼다(확정도 뒤로가기도 아님 — "name committed: 홍길동abz" 가 없어야 한다)
20 [JGEditor]: SceneViewport Back (Esc): no game UI back handler   ← 다음 Esc 부터 뒤로가기(HUD 는 뒤로가기 처리 위젯이 아님)
```

그리고 `[error]` · `[critical]` 0(경고는 기존 1줄: Asset `NOT Support Asset Path`. 10-01 아침까지 있던 DevScene 즉시 리드백 경고는 DevFeature 시작 열기가 빠져 없어짐), 종료 때 `GameUI remove JGGameUIDemoHUD` → GameFrameWorks 종료, live blocks 0, 종료 코드 0. 캡처: before 는 "턴 1" + 오른쪽 위 여러 줄 문단(패널 폭 안, 긴 주소가 단어 안에서 줄바꿈) · 말줄임 줄(끝이 "…") · 빈 입력란 2개(흐린 자리 글), menu 는 어두운 막 위 일시정지 패널(뒤 HUD 는 그대로 보임), input 은 주소 칸 `192.168.0.10:4777` · 이름 칸 `홍길동ab` + 커서(포커스 색), after 는 "턴 3"(메뉴 없음).
실제 한글 IME 조합(ㅎ → 하 → 한, 조합 창이 입력란 커서 옆에 뜨는지)은 스크립트로 못 본다 — 사람이 한 번 입력해 본다.

## 알아둘 것

- End Turn 색은 코드에서 선형 (0.95, 0.45, 0.10)인데 화면에는 sRGB (249, 179, 89)로 보인다(출력 FP16 이 선형으로 표시, ImGui 와 같음, Graphics D-3). 스크립트가 이 색으로 버튼을 찾으므로 색을 바꾸면 스크립트도 고친다.
- 클릭 좌표는 기준 해상도(1920x1080) 기준이다. HUD · 메뉴 배치를 바꾸면 스크립트의 좌표(주석에 적음)도 고친다.
- Esc 는 Scene Viewport 창에 포커스가 있을 때만 게임 UI 로 간다. 스크립트는 그 전에 뷰포트 안을 클릭한다(ImGui 는 클릭한 창에 포커스를 준다).
- 이 게임은 `gameui.selftest`(헤드리스)로 못 보는 것만 본다: 실제 GPU 그리기 · 한글 글리프 · 이미지 파일 · 에디터 입력 연결(포인터 · Esc · 글자 · 편집 키).
- 글자는 `WM_CHAR` 를 **`PostMessageW`** 로 보낸다(스크립트의 P/Invoke 는 `CharSet.Unicode`). 문자 집합을 빼면 `PostMessageA` 로 묶여 한글이 코드 페이지로 바뀌어 깨진다(10-01 첫 실행 "M8冒ab").
- 입력란 좌표(기준 해상도): 주소 가운데 (1560, 630), 이름 (1560, 710). HUD 오른쪽 위 패널 · 입력란 배치를 바꾸면 스크립트 좌표도 고친다. 빈 곳 (480, 540) 은 계속 비워 둔다(월드 피킹 단계).
- 게임 프로젝트 `Temp` 가 `%TEMP%` 아래면 엔진 헤더가 바뀐 뒤에는 게임 sln 을 `-t:Rebuild` 한다(MSBuild 가 헤더 변경을 추적하지 않는다, MSB8029).

마지막 실행 기록: `../../2026-10-01_GameUI_ER007_ER008_verify.txt`(20단계, 워크트리 · 메인 엔진). 이전: `../../2026-10-01_GameUI_GFW_verify.txt`(10단계).
