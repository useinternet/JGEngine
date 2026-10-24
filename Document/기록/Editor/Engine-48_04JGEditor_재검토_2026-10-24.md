# 위키 04. JGEditor 재검토 (Engine-48)

2026-10-24 19:12 Editor 세션. 상위 일감은 구조도 하위 문서 재검토(Engine-36)이고, 사용자 Q-004 답에 따른 것이다. 문서는 고치지 않았다. 아래 표가 수정안이다.

- 대상: `C:/obsidian/ProjectJG/02. 위키/JGEngine/04. JGEditor.md`(절 0~4, 그림 6장).
- 그림 원본: `Tools/Ops/wiki_svg/specs/JGEditor/`.
- 대조한 코드: 메인 HEAD 008cf58. 소스는 c6dd99d 와 같다.
- 컨텐츠 브라우저(Engine-31)는 아직 메인에 없으므로 "틀림"으로 치지 않았다. 패치를 적용한 뒤 고칠 곳은 `Document/기록/Editor/Engine-31_컨텐츠브라우저_검증_2026-10-24.md` 의 "04. JGEditor 위키 갱신할 곳" 표에 따로 적었다.

## 결론

- **① 코드와 맞는가.** 대체로 맞다.
  - 맞는 것: 시작 · 종료 순서, 엔진 모듈 연결 목록, 창 제목, 화면 배율, 씬 뷰포트 흐름, 창 복원, 의존 모듈, 클래스 이름이 모두 코드와 같다.
  - 고칠 곳: 작은 것 셋 — 메뉴 표 순서, `imgui.ini` 자리 설명, 궤도 · 줌 조건.
- **② 게임 프로젝트가 이해하기 쉬운가.** 부족하다. 게임 프로젝트 사람이 이 문서에서 알고 싶은 것은 둘이다.
  - 내 게임 에디터 모듈은 무엇을 쓸 수 있나 — 지금은 2절 끝 한 문장뿐이다.
  - 내 창 · 메뉴는 어떻게 붙고 언제 열리나.
- **③ API Reference 로 바로 갈 수 있는가.** 길이 없다.
  - JGEditor 에는 게임 프로젝트가 부르는 공개 API 가 없다. 템플릿의 게임 에디터 모듈(`Build/Templates/GameProject/Source/{PROJECT_NAME}Editor`)은 JGEditor 에 의존하지 않는다. 의존 모듈은 Core · Graphics · GUI · GameFrameWork · 게임 모듈이다.
  - 게임 에디터 모듈이 실제로 쓰는 API 는 GUI 의 `JGWidget` · `HGUIModule` · `HMainMenuItem` · `HGUI` 다. 문서가 이 점을 밝히고, GUI Reference 로 이어 주어야 한다.
- **④ 설명과 절차가 섞였는가.** 섞이지 않았다. 문서 전체가 설계 설명이고, 하우투 단계는 없다.
  - 다만 2절 끝 문장("게임 에디터 모듈은 … 직접 사용해 … 추가한다")은 하우투를 바라는 독자를 거기서 끊는다. Reference · 하우투로 가는 안내 문장으로 바꾸는 것이 좋다.

## 수정안

| # | 어디를 | 어떻게 | 왜 |
|---|---|---|---|
| 1 | 2절 메뉴 표 줄 순서 | 실제 메뉴 순서로 바꾼다: `Windows/Statistics/Memory` → `DevConsole` → `Scene Viewport` → `Gameplay DevView` → `Data Table Editor` → `Session Lobby` | 4-1절이 "등록 순서대로 표시된다"고 말하는데 표 순서가 다르다. 코드는 `JGEditor.cpp` `openDefaultWidgets` 에서 메모리 · 콘솔을 맨 앞에 등록한다. 에디터 화면에서도 이 순서로 보인다 |
| 2 | 4절 · 4-1절 "실행 폴더의 `imgui.ini`" | "작업 폴더(보통 `Bin/<구성>`)의 `imgui.ini`" 로 쓴다 | ImGui 는 현재 작업 폴더 기준으로 `imgui.ini` 를 읽고 쓴다(`IO.IniFilename = "imgui.ini"`). 실행 파일 폴더가 아니다. `MainTree.cmd` 와 대시보드 실행 단추는 작업 폴더를 `Bin/<구성>` 으로 두고 띄운다. 다른 폴더에서 띄우면 배치가 그 폴더에 따로 생긴다 |
| 3 | 3-1절 "게임이 받지 않은 입력은 에디터의 카메라 궤도 · 줌에 사용한다" (그림 04 설명도) | "게임이 받지 않은 오른쪽 끌기 · 휠은, 활성 카메라가 궤도 모드일 때 궤도 · 줌에 쓴다" 로 좁힌다 | 코드(`SceneViewport.cpp`)는 오른쪽 끌기 · 휠만, 그것도 궤도 모드 `JGCameraActor` 일 때만 쓴다. 지금 글은 모든 입력을 쓰는 것처럼 읽힌다 |
| 4 | 0절 첫 문단 끝(또는 2절 첫 문단) | 한 문장을 더한다: "JGEditor는 게임 프로젝트가 직접 부르는 공개 API가 없다. 게임 에디터 모듈은 JGEditor가 아니라 GUI의 `JGWidget` · `HGUIModule`(메뉴 · 창)을 쓴다" | 게임 프로젝트 독자가 이 문서에서 Reference 를 찾지 않게 한다. 템플릿 게임 에디터 모듈의 의존 목록과 같다 |
| 5 | 2절 끝 문장 "게임 에디터 모듈(`<프로젝트>Editor`)은 … 추가한다" | 안내 문장 + 링크로 바꾼다. 내용: 게임 에디터 모듈은 시작할 때 게임 모듈을 먼저 연결하고(`ConnectModule("<프로젝트>")`) `HGUIModule::AddMainMenuItem` 으로 메뉴를 등록한다. 창 클래스는 `JGWidget` 을 상속한다. 쓰는 법은 GUI API Reference 를 본다 | 지금 문장은 "직접 사용해"에서 끝나 다음에 무엇을 볼지 알 수 없다. 단계(하우투)는 이 문서에 넣지 않고 Reference · 하우투로 보낸다(④). 순서 제약(게임 모듈 먼저, 의존 모듈은 끊지 않음)은 템플릿 코드 주석에 있는 사실이다 |
| 6 | 그림 05(메뉴 등록과 창 복원) · 4-1절 | 게임 에디터 모듈 창의 복원 조건을 한 구절 더한다: "저장된 창은 그 클래스를 가진 모듈이 연결된 실행에서만 다시 열린다(프로젝트 없이 띄우면 게임 창은 건너뛰고 항목은 남는다)" | 4-1절에 "이번 실행에 없는 클래스는 건너뛴다"가 이미 있지만, 그것이 게임 에디터 모듈 창 이야기라는 것이 드러나지 않는다. 게임 프로젝트 사람에게 가장 쓸모 있는 사실이다 |
| 7 | 2절 메뉴 표 | 열을 하나 더한다: "선언"(헤더 위치, 예: `Source/Editor/JGEditor/Widgets/SceneViewport.h`) | API Reference 항목 꼴(9절: 선언 = 헤더 위치)과 맞춰 둔다. 나중에 에디터 창을 Reference 로 정리할 때 그대로 옮길 수 있다 |
| 8 | 머리 칸 `출처` | `Build/Templates/GameProject/Source/{PROJECT_NAME}Editor/{PROJECT_NAME}EditorModule.cpp` 를 더한다(4 · 5 를 넣으면) | 게임 에디터 모듈 이야기의 근거 파일이다 |

## 맞는 것 (고치지 않음)

| 절 | 확인한 내용 | 근거 |
|---|---|---|
| 0 | JGLauncher 가 `LaunchModule = "JGEditor"`, `-game` 이면 GameHost | `Source/Programs/JGLauncher/Main.cpp` |
| 0 · 그림 00 | 의존 Core · GameFrameWork · Graphics · GUI, 에디터 창 6개(지금 메인 기준) | `JGEditor.module.json`, `Widgets/` |
| 0 | 게임 패키지에 들지 않음 — 패키지는 `JG_WITH_EDITOR 0` 구성만, 런타임 모듈이 에디터 모듈에 의존하면 쿡 실패 | `Source/Programs/JGConsole/Cook/CookCommand.cpp` 4단계 |
| 1 · 1-1 · 그림 01 | 시작: 창 → 엔진 모듈(Graphics · GUI · GameFrameWork) → 메뉴 → 프로젝트 모듈(GameModules → EditorModules) → 씬 뷰포트. 종료: 프로젝트 모듈 역순 → GameFrameWork → 창 → 나머지 엔진 모듈 역순 | `JGEditor.cpp` `StartupModule` · `ShutdownModule` · `disconnectProjectModules` |
| 1-2 · 그림 02 | 창 제목 `JGEditor` / `JGEditor - <프로젝트>`, 시작 때 DPI 인식, 기본 크기 × 배율, `WM_DPICHANGED` 뒤 글꼴 · 스타일 다시 만듦 | `JGEditor.cpp`, `JGLauncher/Main.cpp`, `DX12GUIBackend.cpp` |
| 2 · 그림 03 | 클래스마다 객체 하나, 창 제목 · 메뉴 경로(`MemoryStatistcs` 철자 포함), 각 창이 부르는 API | `GUIModule.h` `OpenWidget`, `Widgets/*.cpp` `GetTitleName` |
| 3 · 3-1 · 그림 04 | `OnUpdate` 에서 `PGameViewport::Render`, 입력 → `HInputEvent` → `HandleInput`, 표시 크기가 0.3초 그대로면 `Resize`(셰이더 재컴파일 때문), GameHost 도 `PGameViewport` | `SceneViewport.h/.cpp`, `GameHostModule.cpp` |
| 4-1 · 그림 05 | 경로 문자열 메뉴(`HMainMenuTree`), 열린 창 목록을 클래스 이름으로, 첫 프레임 `RestoreLayoutWidgets`, 없는 클래스는 건너뛰고 항목 유지, 기본 창은 닫혀 있으면 열지 않음 | `GUIModule.cpp`, `Menu/MenuTree.h`, `JGEditor.cpp` `openSceneViewport` |

## 덧붙임

- 위키 글 규칙(사소한 세부 · 주의 사항은 설계 설명 문서에 쓰지 않음)에 걸리는 문장은 없다.
- 3-1절의 "셰이더 재컴파일을 동반하므로"는 세부이지만, 왜 한 번만 적용하는지 알려 주는 설계 까닭이라 두는 것이 좋다.
- 컨텐츠 브라우저 반영(창 7개, 메뉴 한 줄, 그림 00 · 03)은 Engine-31 기록의 표대로 따로 한다.
