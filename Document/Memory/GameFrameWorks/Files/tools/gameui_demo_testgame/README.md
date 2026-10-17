# 게임 UI 화면 검증용 게임 (GameUIDemo)

엔진 밖 검증용 게임 코드다(게임 콘텐츠가 아니라 검증 장치). GameFrameWorks 의 게임 UI(`UI/`, `JGGameWidget` · CommonUI식 레이어 스택)를 게임 모듈이 쓰고, 에디터 Scene Viewport(JGEditor)가 월드 위에 그려 포인터 · Esc 를 먼저 넘기는지 본다. 설계 `../../GameUI_설계_2026-10-01.md` §6 V2.

| 파일 | 내용 |
|---|---|
| `GameUIDemoHUD.h/.cpp` | `JGGameUIDemoHUD`(JGCLASS, Game 레이어 · 입력 모드 Game). 위 띠(반투명 패널, 입력 받음) + 턴 글자 + 두 줄 안내 + Menu 버튼(관리자 기본 스타일), 카드 뒷면 2장(엔진 `Content/RawResources/CardBack_Tree_{Dark,Gold}_63x88mm_300dpi.png`), 손패 글자, 비활성 Draw 버튼, 오른쪽 아래 주황 End Turn 버튼(직접 정한 스타일, 누르면 턴 +1). 오른쪽 위 줄바꿈 패널(ER-007: 폭 600 문단 `Word` · 잘라내기, 높이 = `MeasureContent`, 그 아래 말줄임 한 줄), 그 아래 입력란 2개(ER-008: 주소 = 숫자 · `.` · `:` 만 21자, 이름 16자, Enter 로 확정하면 로그). 왼쪽 서식 패널(입력을 받지 않음 — 빈 곳 클릭은 월드로): ER-010 마크업 구간 서식 한 줄(굵게 · 기울임 · 빨강 · 파랑 · 장평 140% · 70% · `&lt;b&gt;`), 양쪽 맞춤 문단(줄 간격 36 · 자간 +20 · 문단 간격 10), 요소 전체 합성 굵게 + 기울임 + 장평 90%, ER-011 대체 글꼴 줄(Arial + 맑은 고딕), 따로 연 글꼴의 아틀라스 두 페이지 줄(첫 프레임 뒤 `OnUpdate` 가 128px 글자로 페이지 0 을 채워 아래 줄 글자가 페이지 1 에 들어간다) + 아틀라스 통계 줄. 10-03: 어두운 카드 뒷면은 그리기만 -5° 기울임(GG-14 그리기 변환, 기준점 = 왼쪽 아래 피벗), 가운데 아래 놓을 곳 `DropZone`(700~1120 × 789~1040, 입력 받음 — 카드보다 먼저 붙여 끄는 카드가 위에 그려짐), 금색 카드 뒷면 `Card1` 끌기(GG-15 — 관리자 `OnDragBegin/Move/End` 를 `OnActivated` 에서 `AddRaw`, `OnDeactivated` 에서 `RemoveObject`. 끄는 동안 그리기 변환으로 포인터를 따라가며 4° · 1.05배 · 불투명도 0.85, 아래 대상이 놓을 곳이면 초록. 놓을 곳에 놓으면 그 안 오른쪽(920, -40)에 남고, 아니면 배치를 안 바꿔 제자리). 10-03 저녁 GG-16: 처음 활성 때 카드 두 장이 왼쪽 밖(-700)에서 날아온다(옮기기 + 불투명도 동시 묶음, 0.5 · 0.7초 OutCubic — 끝나면 `tween CardFlyIn finished`). End Turn 은 피벗을 가운데로(자리는 그대로 1540~1880 × 930~1040) 두고 누르면 0.9배로 눌렸다가 OutBack 으로 돌아온다(`tween EndTurnPop finished`). Menu 버튼은 일시정지 메뉴 한 객체를 다시 올린다(`PushWidgetInstance` — 닫히는 중이면 다시 열림). 10-03 밤 GG-18 · 10-04 GG-19: 오른쪽 입력란과 End Turn 사이(1240~1880 × 760~916)에 굵게 세기 비교 패널 `BoldStrengthPanel`(입력 받지 않음) — 같은 `<b>폭로 – 강제 – 전투.</b> 본문` 을 문단 서식 `BoldStrength` 만 바꿔 기본(0 — 옛 넓힘, 흐림) · em/28 · em/20(선명한 굵게) 세 줄, 40 논리 단위(에디터 뷰포트 800px 폭에서 약 17px). 10-04 GG-20: 카드와 놓을 곳 사이 목록 칸 `ListPanel/ListScroll`(450~680 × 789~1040, 방향 Both, 열쇠 `DemoList`, 줄 버튼 40개 320 × 36 · 간격 40 — 보이는 칸 214 × 235 · 최대 106 × 1361, 세로 · 가로 막대, 바로 넘김)과 가운데 이야기 칸 `StoryPanel/StoryScroll`(680~1220 × 640~780, 20줄 18px · 줄 간격 22 — 최대 300, 세로 막대, 부드럽게 0.15초). 넘길 때 `scroll <칸> offset (x, y)`, 줄을 누르면 `list row N clicked` 로그. 카드보다 먼저 붙여 끄는 카드가 위에 그려진다. 10-10 GG-21: 오른쪽 줄바꿈 패널과 입력란 사이 지도 틀 `MapFrame`(1240~1880 × 430~580 — 자식 잘라내기 · 입력 받기 · 끌기 · 휠 받기) 안의 지도 `MapContent`(칸 160 × 150 을 8 × 2, "장소 A1"~"H2", 피벗 왼쪽 위, 처음 배율 0.5 = 틀에 꼭 맞음). 틀의 `OnWheel` → 포인터 기준 확대(목표 배율 × 1.15 ^ 칸 수, 0.5~2.0, 배율 · 옮기기 트윈 한 동시 묶음 0.12초 OutCubic — 끝나면 `map zoom done … moved` 로그), 틀 끌기 = 지도 옮기기(`map pan begin/end`), 끄는 중 휠은 바로 확대(`(dragging)`) |
| `GameUIDemoPauseMenu.h/.cpp` | `JGGameUIDemoPauseMenu`(JGCLASS, Menu 레이어 · 입력 모드 Menu · 뒤로가기 처리). 화면을 어둡게 덮는 막 + 가운데 패널(제목 · 안내 · Resume 버튼). Resume 또는 Esc 로 닫힌다. GG-16: 열림 전환 `Fade(0.3)` · 닫힘 전환 `Fade(0.4, InQuad)`(막 + 패널 전체), 패널은 `OnActivated` 에서 40 아래 → 0 으로 올라오고 `OnDeactivated`(닫힐 때 `IsClosing()`)에서 다시 내려간다 — 닫히는 동안에도 트윈이 돈다. `pause menu deactivated (closing 1)` 로그 |
| `GameUIDemoModule.h/.cpp` | GFW 연결 → 엔트리 액터 → 월드 로드 → `GetUI()->PushWidget<JGGameUIDemoHUD>(EGameUILayer::Game)`. 종료 때 `ClearAllWidgets()` |
| `GameUIDemoEntryActor.cpp` | 배경 월드: X Bot 정적 메시 + 궤도 카메라(헤더는 템플릿이 만든 것 그대로) |
| `GameUIDemo.module.json` | 템플릿 의존(`Core`, `GameFrameWorks`)에 `Asset`, `Graphics` 추가(JGBuildTool 은 include 경로를 전이하지 않는다). 별도 UI 모듈은 없다 |
| `gameui_e2e.ps1` | 화면 검증(UTF-8 BOM). 런처 실행 → 캡처 → 주황 End Turn 을 색으로 찾아 뷰포트 이미지 사각형을 역산 → 기준 해상도 좌표로 PostMessage 클릭 · Esc(1~10단계) → 입력란 클릭 · `WM_CHAR` 글자 · Backspace · Enter · 입력 중 Esc(11~20단계) → 끌기(21: 금색 카드 → 놓을 곳, 22: 다시 끌어 뷰포트 이미지 밖에서 떼기) → 열림 · 닫힘 전환(GG-16, 23~27: 메뉴 열기 → Esc 바로 뒤 Resume 자리 클릭 · 닫히는 중 캡처 → 다시 열기 · 여는 중 캡처 → Esc 바로 뒤 Menu = 닫히다 다시 열기 → Resume) → 스크롤 칸(GG-20, 28~37: 목록 휠 3칸 · 줄 클릭 · 칸 아래 잘린 줄 자리 클릭 · 세로 손잡이 끌기 · 세로 막대 빈 곳 · 위 띠 위 휠 · 가로 손잡이 끌기 · 가로 휠(`WM_MOUSEHWHEEL`) · 이야기 휠 2칸 · 이야기 막대 빈 곳) → 휠 받기(GG-21, 38~41: 지도 휠 앞으로 2칸 · 다른 자리에서 뒤로 1칸 · 지도 틀 끌기 · 끄는 중 휠(`DragWheel-Ref` — 휠 wParam 에 MK_LBUTTON)) → 캡처 8장(before · menu · input · closing · opening · scroll · map · after) → WM_CLOSE. 런처 창에만 메시지를 보낸다. 10-11(GG-H32): 자리가 필요한 묶음(클릭 [이동 · 누름 · 뗌], 끌기 시작 [이동 · 누름] · 끝 [이동 · 뗌], 휠 [이동 · 휠])은 C# `PostBurst` 로 앱 프레임에 맞춘 뒤 잇달아 넣고(`SyncFrame` = WM_NULL SendMessageTimeout), 끝에 게임 로그로 41단계를 자동 판정한다 — `E2E CHECK: PASS`(종료 코드 0) / `FAIL` + 빠진 단계(종료 코드 6). `-CheckLog <jg_log.txt>` = 띄우지 않고 지난 로그만 판정, `-NoCheck` = 판정 생략 |
| `gameui_input_probe.ps1` | 입력 빠짐 실험 도구(GG-H32, UTF-8 BOM). 런처를 띄워 End Turn 을 한 방식으로 `-Trials` 번 누르고 끝난 로그의 "End Turn clicked" 수를 센다 — `-Method A`(PowerShell 줄마다) · `B`(이동 뒤 30ms 쉬고 누름 · 뗌 — 원인 재현, 모두 빠짐) · `C`(C# 묶음) · `D`(프레임에 맞춘 C# 묶음). 앱 Info 로그는 경고 이상일 때만 바로 쓰여 실행 중에는 못 세므로 방식마다 따로 띄운다 |

## 재현

```
Build\BatchFiles\CreateGameProject.bat <dir>\GameUIDemo GameUIDemo
copy 이 폴더의 .h · .cpp · .module.json → <dir>\GameUIDemo\Source\GameUIDemo\   (GameUIDemoEntryActor.h · GameUIDemoEditor 는 템플릿 그대로)
<dir>\GameUIDemo\GenerateProjectFiles.bat
MSBuild <dir>\GameUIDemo\GameUIDemo.sln -m -p:Configuration=DevelopEngine -p:Platform=x64      (처음 약 4분)
powershell -ExecutionPolicy Bypass -File gameui_e2e.ps1 -BinDir <dir>\GameUIDemo\Bin\DevelopEngine -OutDir <out> -WaitSeconds 40
   → 출력 끝 "E2E CHECK: PASS - steps 1-41, 51 ordered checks + 7 counts"(종료 코드 0). 빠지면 "E2E CHECK: FAIL" + 단계별 문제, 종료 코드 6
powershell -ExecutionPolicy Bypass -File gameui_e2e.ps1 -CheckLog <jg_log.txt>      (지난 실행 로그만 다시 판정)
```

기대 결과 (`<dir>\GameUIDemo\Bin\DevelopEngine\jg_log.txt`, 순서대로 — 스크립트 끝의 자동 판정(`$expected` 차례 검사 51줄 + `$counts` 세기 7개)이 이 줄들을 본다. HUD · 단계를 바꾸면 둘 다 고친다):

```
[GameUI]: Font loaded : C:/Windows/Fonts/malgun.ttf      ← HUD 가 OnInitialize 에서 문단 높이를 재므로 먼저 읽힌다
[GameUIDemo]: GameUIDemo scroll boxes: list max (106.0, 1361.0), story max (0.0, 300.0)   ← GG-20 스크롤 칸 둘(이야기 글 높이를 재며 기본 글꼴이 먼저 읽힌다)
[GameUIDemo]: GameUIDemo paragraph measured 598.8 x 207.5 (width 600)   ← 폭 ≤ 600, 높이 = 줄 수(6) × 줄 간격(맑은 고딕 26px). 글꼴이 다르면 값도 다르다
[GameUI]: Font loaded : C:/Windows/Fonts/arial.ttf       ← 대체 글꼴 줄(맑은 고딕은 이미 읽은 기본 글꼴을 다시 쓴다)
[GameUIDemo]: GameUIDemo HUD initialized: 15 root elements    ← 10-10 지도 틀(GG-21)이 늘었다(10-04 스크롤 칸 둘(목록 · 이야기 판)로 14, 10-03 밤 굵게 세기 패널로 12, 10-03 아침 놓을 곳으로 11, 10-02 왼쪽 서식 패널로 10, 그 전 9)
[GameUI]: GameUI push JGGameUIDemoHUD on Game (stack 1)
[GameUIDemo]: GameUIDemo HUD activated
[GameUI]: Texture loaded : .../Content/RawResources/CardBack_Tree_Dark_63x88mm_300dpi.png (744x1039)   ← Gold 도 한 줄
[GameUIDemo]: GameUIDemo atlas filled: pages 2, filler 70 glyphs   ← 첫 그리기 뒤 한 번(맑은 고딕 128px 70자로 페이지 0 이 찬다). 1단계보다 먼저 나온다
[GameUIDemo]: GameUIDemo tween CardFlyIn finished (canceled 0)   ← GG-16 카드 날아오기(0.7초). 1단계보다 먼저 나온다
1  [JGEditor]: SceneViewport Click (...): game UI (not picked)   + [GameUIDemo]: GameUIDemo End Turn clicked (turn 2)   + (0.36초 뒤) GameUIDemo tween EndTurnPop finished (canceled 0)
2  [GameUIDemo]: GameUIDemo Menu clicked   + [GameUI]: GameUI push JGGameUIDemoPauseMenu on Menu (stack 1)   + pause menu activated
3  [JGEditor]: SceneViewport Click (...): game UI (not picked)   ← End Turn 이 막힘(턴 그대로)
4  [JGEditor]: SceneViewport Click (...): game UI (not picked)   ← 빈 곳도 막힘(월드 피킹 없음)
5  GameUI remove JGGameUIDemoPauseMenu from Menu (stack 0) + pause menu deactivated (closing 1) + GameUI back handled by JGGameUIDemoPauseMenu + SceneViewport Back (Esc): handled by game UI
   → 0.4초 뒤 [GameUI]: GameUI closed JGGameUIDemoPauseMenu on Menu + pause menu closed(OnShutdown)   ← GG-16 닫힘 전환. 전환이 없던 때는 remove 바로 뒤 closed
6  [GameUIDemo]: GameUIDemo End Turn clicked (turn 3)   + tween EndTurnPop finished (canceled 0)
7  [JGEditor]: SceneViewport Click (...): nothing (no controller in the world)   ← 월드 피킹 경로
8  [JGEditor]: SceneViewport Click (...): game UI (not picked)   ← 비활성 Draw: 클릭 로그("Draw clicked while disabled") 없음
9  GameUI push JGGameUIDemoPauseMenu ... / 10  [GameUIDemo]: GameUIDemo Resume clicked + GameUI remove ... (stack 0) + deactivated (closing 1) → GameUI closed ... + pause menu closed
11 [JGEditor]: SceneViewport Click (...): game UI (not picked)   ← 주소 입력란 포커스
14 [GameUIDemo]: GameUIDemo address committed: 192.168.0.10:4777   ← 'x' 는 허용 문자가 아니라 버려지고, Backspace 가 끝 '1' 을 지움
15 [JGEditor]: SceneViewport Click (...): game UI (not picked)
17 [GameUIDemo]: GameUIDemo name committed: 홍길동ab
18 [JGEditor]: SceneViewport Click (...): game UI (not picked)
19 (로그 없음)   ← 입력 중 Esc 는 입력란 포커스만 푼다(확정도 뒤로가기도 아님 — "name committed: 홍길동abz" 가 없어야 한다)
20 [JGEditor]: SceneViewport Back (Esc): no game UI back handler   ← 다음 Esc 부터 뒤로가기(HUD 는 뒤로가기 처리 위젯이 아님)
21 [JGEditor]: SceneViewport Click (0.166, 0.844): game UI (not picked)   + [GameUIDemo]: GameUIDemo drag begin Card1 at (318, 912)
   + [GameUIDemo]: GameUIDemo drag end Card1: dropped on DropZone at (1002, 912) (moves N)   ← 시작 자리 = 누른 자리(논리 단위, 클라이언트 좌표 반올림으로 몇 단위 어긋남)
22 [JGEditor]: SceneViewport Click (0.522, 0.844): game UI (not picked)   + GameUIDemo drag begin Card1 at (1002, 912)
   + GameUIDemo drag end Card1: returned, drop target none at (2380 안팎, 912) (moves N)   ← 이미지 밖에서 뗌 = 놓은 곳 없음, 취소 아님(카드는 놓을 곳 안 그대로)
23 GameUIDemo Menu clicked + GameUI push JGGameUIDemoPauseMenu on Menu (stack 1) + pause menu activated
24 GameUI remove ... (stack 0) + deactivated (closing 1) + back handled + Back (Esc): handled by game UI
   + [JGEditor]: SceneViewport Click (0.500, 0.59x): nothing (no controller in the world)   ← 닫히는 메뉴의 Resume 자리를 눌렀는데 메뉴가 받지 않고 월드로("Resume clicked" 가 없어야 한다)
   + 그 뒤에 GameUI closed JGGameUIDemoPauseMenu on Menu + pause menu closed   ← 클릭 줄이 closed 보다 먼저 = 아직 그려지는 동안 눌렀다
25 Menu clicked + push (stack 1) + pause menu activated   ← 다시 처음부터(닫힘이 끝났으므로 OnInitialize 부터)
26 remove ... + deactivated (closing 1) + back handled + Back (Esc)   + Menu clicked + GameUI push JGGameUIDemoPauseMenu on Menu (stack 1) - reopened while closing + pause menu activated
   ← 그 사이 "pause menu closed"(OnShutdown)가 없어야 한다 — 닫히다 다시 열림(지금 모습에서 이어서 열림)
27 Resume clicked + remove + deactivated (closing 1) → GameUI closed + pause menu closed
28 GameUIDemo scroll ListScroll offset (0.0, 60.0) · (0.0, 120.0) · (0.0, 180.0)   ← 목록 칸 휠 3칸(한 칸 60, 바로 넘김)
29 SceneViewport Click (0.273, 0.758): game UI (not picked) + GameUIDemo list row 5 clicked   ← 넘긴 양 180 에서 (520, 820) = 내용 211 = 줄 5
30 SceneViewport Click (0.296, 0.978): nothing (no controller in the world)   ← 칸 아래(565, 1060) — 잘려 안 보이는 줄 자리는 누르지 않는다("list row" 가 없어야 한다)
31 Click (0.351, 0.771) + scroll ListScroll offset (0.0, 0.0) · (0.0, 1361.0)   ← 세로 손잡이를 아래 밖까지 끌기 = 끝. 중간 값(0.0 등)은 보낸 메시지 사이 쓰레기 자리 — 마지막이 1361
32 scroll ListScroll offset (0.0, 1155.4) + Click (0.351, 0.738)   ← 세로 막대 손잡이 위 빈 곳 = 보이는 칸 235 의 7/8 만큼 위로(넘김 줄이 클릭 줄보다 먼저 찍힌다)
33 (넘김 로그 없음)   ← 위 띠 위 휠: 받을 칸이 없어 UI 가 소비만(카메라 줌도 없음)
34 Click (0.274, 0.953) + scroll ListScroll offset (106.0, 1155.4)   ← 가로 손잡이를 오른쪽 밖까지 = 가로 끝
35 scroll ListScroll offset (46.0, 1155.4)   ← 가로 휠(WM_MOUSEHWHEEL) 왼쪽 한 칸 = −60
36 scroll StoryScroll offset (0.0, 16.8) … (0.0, 60.0) … (0.0, 120.0)   ← 이야기 칸 휠 2칸, 부드럽게(0.15초 OutCubic — 프레임마다 한 줄, 값은 실행마다 조금 다르다)
37 Click (0.631, 0.711) + scroll StoryScroll offset … (0.0, 242.5)   ← 이야기 막대 손잡이 아래 빈 곳 = 140 의 7/8(122.5), 부드럽게
38 GameUIDemo map zoom wheel 1.00 -> scale 0.575 at (1399, 504) + map zoom done scale 0.575, point under the pointer moved 0.00   ← 지도 휠 앞으로 1칸째(포인터 기준 1.15배)
   + map zoom wheel 1.00 -> scale 0.661 at (1399, 504) + map zoom done scale 0.661, … moved 0.00   ← 2칸째(목표 배율을 쌓는다)
39 map zoom wheel -1.00 -> scale 0.575 at (1697, 470) + map zoom done scale 0.575, … moved 0.00   ← 다른 자리에서 뒤로 1칸(축소) — moved 가 0 이 아니면 포인터 기준 확대가 깨진 것
40 SceneViewport Click (0.780, 0.480): game UI (not picked) + map pan begin at (1498, 518) + map pan end at (1380, 480): translation (-103, -54), scale 0.575 (moves N)   ← 지도 틀 끌기 = 지도 옮기기
41 Click (0.832, 0.462) + map pan begin at (1598, 499) + map zoom wheel 1.00 -> scale 0.661 at (1558, 499) (dragging) + map zoom done … moved 0.00 + map pan end at (1538, 499): translation (-232, -72), scale 0.661   ← 끄는 중 휠을 끄는 틀이 받는다(끄는 동안의 판정은 끄는 요소를 뺀다)
```

그리고 `[error]` · `[critical]` 0(경고는 기존 1줄: Asset `NOT Support Asset Path`. 10-01 아침까지 있던 DevScene 즉시 리드백 경고는 DevFeature 시작 열기가 빠져 없어짐), 종료 때 `GameUI remove JGGameUIDemoHUD` → GameFrameWorks 종료, live blocks 0, 종료 코드 0. 종료 때 Graphics 로그에 `GameUIFontAtlas_malgun_0` · `_1`(아틀라스 두 페이지 텍스처) 해제가 보인다. 캡처(GG-21): before 의 지도 틀은 칸 16개가 꼭 맞고(배율 0.5), map 은 확대 · 옮긴 지도(배율 0.661 — 틀 밖 지도는 잘림). 캡처(GG-20): before 의 목록 칸은 00~05 줄(라벨 오른쪽이 보이는 칸에서 잘림, 세로 · 가로 막대), 이야기 칸은 1~6줄(7줄째가 칸 아래에서 잘림), scroll 은 목록 29~34 줄(가로로 46 밀림) · 이야기 12~17줄 — 칸 밖 글이 없고 에디터 창은 그대로(밀리지 않음). 캡처: before 는 "턴 1" + 오른쪽 위 여러 줄 문단(패널 폭 안, 긴 주소가 단어 안에서 줄바꿈) · 말줄임 줄(끝이 "…") · 빈 입력란 2개(흐린 자리 글) + 왼쪽 서식 패널(굵게 · 기울임 · 색 · 장평이 섞인 줄, 오른쪽 끝이 맞는 양쪽 맞춤 문단, 굵은 기울임 줄, Arial 줄 안의 한글, "페이지 0 글자" · "페이지 1 글자" 줄, "ER-011 atlas: pages 2 / limit 4 …"), menu 는 어두운 막 위 일시정지 패널(뒤 HUD 는 그대로 보임), input 은 주소 칸 `192.168.0.10:4777` · 이름 칸 `홍길동ab` + 커서(포커스 색), after 는 "턴 3"(메뉴 없음) + 금색 카드가 놓을 곳 오른쪽 안. before 부터 어두운 카드가 왼쪽 아래를 축으로 기울어 그려지고(GG-14), 가운데 아래 "놓을 곳 / Drop zone" 패널이 보인다.
before 의 오른쪽 아래 굵게 세기 패널(GG-18 · GG-19)은 첫 줄(기본, 세기 0)이 옛 넓힘이라 굵은 구간이 회색으로 번지고, em/28 · em/20 줄은 선명하고 진하다(em/28 = 옛 넓힘의 em/16 과 같은 굵기). closing 캡처(GG-16)는 메뉴가 반쯤 흐려진 채 패널이 아래로 내려가는 중, opening 은 패널이 올라오는 중(열림은 OutCubic 이라 앞부분에서 거의 다 보인다)이다. before 에서는 카드 날아오기가 이미 끝나 있다(시작 40초 뒤).
실제 한글 IME 조합(ㅎ → 하 → 한, 조합 창이 입력란 커서 옆에 뜨는지)은 스크립트로 못 본다 — 사람이 한 번 입력해 본다.

## 알아둘 것

- End Turn 색은 코드에서 (0.95, 0.45, 0.10)이고, 화면에 보이는 색은 출력 색 공간에 따라 다르다(Graphics D-3 · 5-14). 5-14 전(게임 UI 를 선형 FP16 출력 텍스처에 그림)은 화면에서 한 번 더 sRGB 로 바뀌어 (249, 179, 89), 5-14 뒤(8비트 sRGB 디스플레이 텍스처에 그림, 10-01 13:33 메인 반영 — 20단계 확인)는 코드 값 그대로 (242, 115, 26). 스크립트는 (242, 115, 26) → 없으면 (249, 179, 89) 순서로 찾고, 어느 쪽인지 `Orange pixels … as …` 줄에 찍는다. 버튼 색을 바꾸면 스크립트도 고친다.
- 클릭 좌표는 기준 해상도(1920x1080) 기준이다. HUD · 메뉴 배치를 바꾸면 스크립트의 좌표(주석에 적음)도 고친다.
- Esc 는 Scene Viewport 창에 포커스가 있을 때만 게임 UI 로 간다. 스크립트는 그 전에 뷰포트 안을 클릭한다(ImGui 는 클릭한 창에 포커스를 준다).
- 이 게임은 `gameui.selftest`(헤드리스)로 못 보는 것만 본다: 실제 GPU 그리기 · 한글 글리프 · 이미지 파일 · 에디터 입력 연결(포인터 · Esc · 글자 · 편집 키).
- 글자는 `WM_CHAR` 를 **`PostMessageW`** 로 보낸다(스크립트의 P/Invoke 는 `CharSet.Unicode`). 문자 집합을 빼면 `PostMessageA` 로 묶여 한글이 코드 페이지로 바뀌어 깨진다(10-01 첫 실행 "M8冒ab").
- 입력란 좌표(기준 해상도): 주소 가운데 (1560, 630), 이름 (1560, 710). HUD 오른쪽 위 패널 · 입력란 배치를 바꾸면 스크립트 좌표도 고친다. 빈 곳 (480, 540) 은 입력을 받는 요소를 두지 않는다(월드 피킹 단계). 10-02 왼쪽 서식 패널이 그 자리를 덮지만 `SetHitTestVisible` 을 켜지 않아 클릭이 월드로 간다 — 패널에 입력을 켜면 7단계가 "game UI (not picked)" 로 바뀐다.
- 게임 프로젝트 `Temp` 가 `%TEMP%` 아래면 엔진 헤더가 바뀐 뒤에는 게임 sln 을 `-t:Rebuild` 한다(MSBuild 가 헤더 변경을 추적하지 않는다, MSB8029).

- 끌기(21 · 22단계)는 [이동 · 누름]과 [이동 · 뗌]을 각각 붙여 보낸다. 실제 커서가 창 밖이면 보낸 이동마다 `WM_MOUSELEAVE` 가 곧 따라와 ImGui 마우스 자리가 비고(창에 포커스가 있으면 실제 커서 자리가 들어온다), 그 사이 프레임의 이동 자리는 쓰레기다 — 그래서 끄는 중 캡처는 찍지 않고 결과는 로그(시작 자리 · 놓은 곳)로 본다. 누름 · 뗌은 바로 앞 이동과 한 묶음(프레임에 맞춘 `PostBurst`)이라 그 자리에서 처리된다. 카드 · 놓을 곳 배치를 바꾸면 스크립트 좌표(주석)도 고친다.
- 전환 단계(24 · 26)는 Esc 뒤 기다리지 않고 바로 클릭을 넣는다(닫힘 0.4초 안). 메뉴 닫힘 전환을 줄이면 이 단계가 빗나갈 수 있다 — 그때는 클릭 줄이 `closed` 뒤에 찍힌다.
- D-22 ②(10-03 19:19) 뒤 에디터는 모니터 배율(125% 등)로 그린다. 그 전에 저장된 게임 Bin 의 `imgui.ini` 는 Scene Viewport 창 크기가 픽셀 그대로라 창이 작아 End Turn 이 잘리고 "End Turn button not found" 로 끝난다 — 게임 Bin 의 `imgui.ini` 를 치우고(기본 배치) 돌린다.
- 엔진 UI 헤더가 바뀌면(클래스 크기 — 10-03 GG-16 은 `JGGameWidget` · `PGameUIElement`) 게임 DLL 을 다시 빌드하고, 엔진에 새 파일이 생기면(GG-16 `UI/GameUITween.*`) 게임 폴더의 `GenerateProjectFiles.bat` 부터 다시 돌린다(게임 sln 이 엔진 모듈을 자기 프로젝트 파일로 빌드한다 — 안 하면 LNK2019).
- 휠(28 · 33 · 35 · 36단계)은 칸마다 [이동 · WM_MOUSEWHEEL/WM_MOUSEHWHEEL]을 붙여 보낸다(휠 lParam 은 화면 좌표, wParam 위 16비트 = ±120). ImGui 가 이동을 먼저 처리하고 다음 프레임에 휠을 처리해 그 자리에서 휠이 온다. 10-04 GG-20 전 에디터는 Scene Viewport 창 내용(그림 + 상태 줄)이 창보다 길면 휠로 **창도 스크롤**해 그림이 위로 밀렸다(클릭 뒤 상태 줄이 늘 때 — 29단계부터 클릭이 기준 y +48 에 찍힘) → GUI `InteractiveImage` 가 이미지 위 휠을 갖게 고침. 같은 증상이 다시 보이면 이것부터 본다.
- 입력 빠짐(10-11 고침, GG-H32): 예전에는 이동 · 누름 · 뗌 · 휠을 PowerShell 줄마다 PostMessage 해서 회차마다 한두 개가 빠졌다. ImGui Win32 백엔드는 이동을 처리할 때 TrackMouseEvent 를 걸고, 실제 커서가 창 위에 없으면 WM_MOUSELEAVE 가 곧바로 들어온다. 줄 사이(약 0.12ms, 처음 부를 때는 더)에 앱이 큐를 비우면 이 떠남이 누름 · 뗌 · 휠보다 먼저 처리되어 마우스 자리가 무효인 채 들어갔다(이동 뒤 30ms 쉬고 누름 · 뗌을 넣으면 30번 모두 빠짐 — `gameui_input_probe.ps1 -Method B`). 지금은 묶음을 C# 에서 앱 프레임에 맞춘 뒤 잇달아 넣는다. 다시 빠지면 출력의 "Input bursts … timeouts"(프레임 맞추기 시간 초과)와 판정 문제 목록부터 본다. 다른 스크립트에 같은 방식이 필요하면 `GUIE2E.PostBurst` 를 옮긴다.
- 지도 단계(38~41) 좌표는 지도 틀 안(1240~1880 × 430~580)이고 결과는 앞 단계 값을 따른다(옮기기 · 배율이 쌓인다). 끌기 중간 프레임의 쓰레기 자리가 남지 않게 데모는 옮기기를 "시작 옮기기 + 누른 자리에서 온 거리"로 매번 처음부터 정한다. 41단계의 끄는 중 휠은 휠 메시지 wParam 아래 16비트에 MK_LBUTTON 을 넣는다 — 휠이 빠지면 `(dragging)` 줄이 없다.
- 스크롤 단계 좌표는 앞 단계 결과(넘긴 양)를 따른다 — 손잡이 자리가 넘긴 양에 따라 바뀐다(스크립트 주석에 막대 · 손잡이 자리). 목록 · 이야기 칸 배치를 바꾸면 좌표도 고친다.
- 에디터 뷰포트는 이미지 밖에서 뗀 프레임에 `Leave` 를 뗌보다 먼저 보낸다. 게임 UI 의 `Leave` 는 호버만 풀고 끌기는 잇는다 — 22단계가 "returned, drop target none" 이어야 하고 "canceled" 면 이 규칙이 깨진 것(10-03 첫 실행에서 겪음).

마지막 실행 기록: `../../2026-10-11_GameUI_e2e_input_verify.txt`(입력 묶음 · 자동 판정 — 메인 엔진 5회 연속 `E2E CHECK: PASS`, 작업 폴더 엔진 게임 2회 PASS). 이전: `../../2026-10-10_GameUI_GG21_verify.txt`(41단계 — 지도 휠 38~41, 작업 폴더 엔진 1회 41단계 모두 · 메인 엔진 4회는 지도 휠 · 스크롤 칸 단계 모두이고 보낸 입력이 회차마다 하나씩 빠짐). 이전: `../../2026-10-04_GameUI_GG20_verify.txt`(37단계 — 스크롤 칸 28~37, 메인 엔진 — 사본 `Temp/Games/GameUIDemo_GUI`. 보낸 메시지 입력이 가끔 하나 빠진다 — 다시 돌린다). 이전: `../../2026-10-04_GameUI_GG19_verify.txt`(27단계 + 굵게 세기 패널), `../../2026-10-03_GameUI_GG18_verify.txt`, `../../2026-10-03_GameUI_GG16_verify.txt`(27단계, 데모를 바꾸기 전 22단계 회귀 포함), `../../2026-10-03_GameUI_GG14_GG15_verify.txt`(22단계), `../../2026-10-02_GameUI_ER010_ER011_verify.txt`(20단계 + 서식 · 아틀라스 패널), `../../2026-10-01_GameUI_ER007_ER008_verify.txt`(20단계), `../../2026-10-01_GameUI_GFW_verify.txt`(10단계).
