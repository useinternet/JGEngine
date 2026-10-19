# GUI TODO (ImGui 모듈)

갱신 2026-10-19(커밋 표기 정정 — 10-02 뒤 GUI 작업도 엔진 총괄의 10-17 15:03 묶음 커밋에 모두 들어 있다: GUI 소스 · D-22 ② GUI 몫 · GG-20 휠 몫 · 백업 zip 삭제 = `26ac012`, GP-C1 Core = `ee36bb1` · `input.selftest` = `7b8ee6e`, 3-5 `.gitignore` 줄 = `37cffb3` · `imgui.ini` 추적 해제 = `6b9d212`(D-40). `git log -S` · `git show --name-status` 로 확인, 엔진 총괄 지시 10-19. GUI 코드에 미커밋 변경 없음). 이전: 2026-10-11(커밋 표기 정정 — 09-30 P1 1-2 · 1-3 · 1-4 · 1-7 · 1-8 · 1-9 와 BL-4 는 사용자 커밋 `736ddbe`(10-01 12:16), BL-3 은 `efba16a`(10-01 16:58)). 이전: 2026-09-30. 구 TODO(`Files/GUI_TODO.md`, 2026-09-21 작성, 09-29 · 09-30 표시 추가)를 통합·최신화했다. **항목 번호는 구 TODO 그대로**(DevConsole·GameModule·Graphics 문서가 참조). 현황·구조·함정·검증 방법은 `현황.md` §2.
기준: 엔진 기반 구축 · 과설계 금지. 코드가 틀린 확정 버그와 지금 겪는 문제만 미완료 표에 두고, 증상 없는 수명 위험·정리·백로그는 보류 표에 "다시 볼 조건"과 함께 둔다. `결정 필요`는 착수 전 사용자 확인. 항목이 끝나면 완료 이력 표로 옮기고 근거를 적는다.
파일:행은 2026-09-30 소스 기준(구 TODO의 09-21 기준 행과 다른 곳은 비고에 적었다). 런처 호스트는 JGEditor다(JGDev_Graphics는 삭제).
09-30 저녁 코드·git 대조(GUI 진행 보고): 미완료 항목의 코드 상태와 파일:행은 그대로다. 다른 트랙이 GUI에 넣은 변경은 모두 커밋됐다(`4c8ac73`까지). 보고서 `Document/GUI_진행보고_2026-09-30.html`.
**09-30 21:4x 구현(커밋 `736ddbe`(10-01 12:16))**: 1-2 합치기(1-1 소멸, 1-3·1-9 포함)와 1-4·1-7·1-8. 빌드 오류 0, 런처 60초 종료 0, 리드백 PNG 2장 커밋본과 바이트 동일. 증적 `Files/2026-09-30_1-2_update_merge_verify.txt`, 캡처 `Files/2026-09-30_gui_update_merge_capture.png`. 아래 파일:행 중 `GUIModule.cpp`·`Widget.h/.cpp`·`WidgetComponent.h`·`DevScene.cpp`는 이 변경 전 기준이다.

---

## 다음에 할 일 Top 5 (권장 순서 — 착수 대상은 사용자가 정한다)

| 순서 | 항목 | 착수 조건 | 완료 기준 |
|---|---|---|---|
| 1 | **1-5 메뉴 트리 키를 전체 경로로** | 없음. `Menu/MenuTree.cpp:21, 36, 39`, `MenuTree.h:52`(`HHashMap<PName, uint64> MenuIndexesByName`가 경로 조각 이름으로 찾음) | `Windows/Open`·`File/Open`을 동시에 등록해도 각각 나옴(임시 코드, 확인 뒤 제거), 기존 경로 유지, 같은 경로 중복·Action 리프에 자식이 붙으면 경고 로그. 09-30 현재 경로 6개(`Windows/DevConsole`, `Windows/Statistics/Memory`, `Dev/DevAI`, JGEditor `Windows/Scene Viewport`·`Windows/Gameplay DevView`(22:4x C-3 에서 GFW 의 `Windows/World View` 를 옮기며 이름 변경), 게임 GfwView 메뉴 1)는 충돌 없음. 메뉴가 늘고 있어 다음 차례 |
| 2 | ~~3-5 `imgui.ini` 추적 해제 · 백업 zip 처리~~ **완료 (10-02) · 커밋(10-17 `37cffb3` · `6b9d212` · `26ac012`)** — 완료 이력 3-5 | — | — |
| 3 | **1-10 P1 검증** | 1(1-5) 뒤. 공용 캡처 스크립트 `Document/Memory/tools/capture_devscene.ps1` 의 옛 경로는 10-11 Graphics 가 고쳤다(`$PSScriptRoot` 기준 · `-WorkDir` · 기본 저장 `Temp\Capture\`) | 공통 루프(`현황.md` §2.5) + 캡처에 메뉴바·DevFeature 창·X Bot. **1-4 확인**: `CanAction`이 false인 임시 메뉴가 회색 비활성인지(확인 뒤 제거). 1-5 임시 코드 제거를 `git diff`로 |
| 4 | ~~1-11 커밋~~ **완료** — 사용자 커밋 `736ddbe`(10-01 12:16) "엔진 구현"에 1-2 · 1-3 · 1-4 · 1-7 · 1-8 · 1-9 파일(`GUIModule.h/.cpp` · `Widget.h/.cpp` · `WidgetComponent.h` · `Menu/MenuTree.cpp` · `Backends/DX12GUIBackend.cpp` · `Devkit/DevScene.h/.cpp` · `Devkit/DevFeature.cpp`)이 모두 들어 있다(1-10 검증보다 먼저 커밋됨) — 완료 이력 1-11 | — | — |
| 5 | **3-7 `HGUI` 확장 방침 한 줄 기록** | 없음 | `GUI.h` 상단 주석 한 줄. GFW 세션이 `Button`·`Separator`·`CollapsingHeader`·`InteractiveImage`를 같은 방침으로 추가 중이다 |

---

## 미완료 항목 (엔진 범위)

| ID | 항목 | 우선순위 | 상태 | 선행조건 | 완료 기준 | 비고 |
|---|---|---|---|---|---|---|
| 1-5 | 메뉴 트리 키를 전체 경로로 — `Menu/MenuTree.cpp:21, 36, 39`, `MenuTree.h` | 중간 | 미착수 | 없음 | `Windows/Open`·`File/Open`을 동시에 등록해도 각각 나옴(확인 후 제거), 기존 3경로 유지, 같은 경로 중복·Action 리프에 자식 붙을 때 경고 로그 | 현재 3경로(`Dev/DevAI`, `Windows/DevConsole`, `Windows/Statistics/Memory`)는 충돌 없음. 메뉴가 늘면 바로 걸림 |
| 1-10 | P1 검증 | — | 일부(1-2·1-7·1-8은 09-30 회귀로 확인) | 1-5(캡처 스크립트 경로는 10-11 Graphics 가 고침) | 공통 루프 + 메뉴바·DevFeature 창 캡처, 1-4 임시 메뉴(`CanAction` false → 회색) 확인, 임시 코드 제거 `git diff` | 1-2 회귀는 끝났다(`Files/2026-09-30_1-2_update_merge_verify.txt`). 남은 것은 1-4 시각 확인과 1-5. 캡처 스크립트의 옛 경로 하드코딩은 10-11 Graphics 가 고쳤다 — `Build\BatchFiles\MainTree.cmd -name=GUI exec Document\Memory\tools\capture_devscene.ps1 -WaitSeconds 30 -WorkDir <JGDevFeature=1 imgui.ini 폴더> -OutPath <png>` |
| 3-7 | `HGUI` 확장 방향 기록 — `GUI.h` 상단 | 낮음 | 결정 기록만 남음 | 없음 | `GUI.h` 상단 주석 한 줄 + 이 표에서 완료 이력으로 | 즉시 모드 함수 = `HGUI` 정적, 상태를 가진 것만 `JGWidgetComponent`. DevConsole 2-3이 이 방침으로 4개(`BeginChild/EndChild/GetFrameHeightWithSpacing/InputTextWithHistory`)를 추가했다. 함수 추가는 필요해질 때 항목을 따로 만든다 |

---

## 보류 (증상 없음 · 정리 · 백로그 — 다시 볼 조건과 함께)

| ID | 항목 | 다시 볼 조건 | 메모 |
|---|---|---|---|
| 2-1 | 스케줄러 `Unschedule(id)` 추가 + 끝난 태스크 풀 누수 + 빈 `Destroy` + `CreateSP` 대상 사망 판정 — `Core/Thread/Scheduler.h/.cpp`. 부수: `PSPDelegate::GetOwner()` 조건 반전(`Delegate.h:253`, 한 줄) | 런타임에 모듈을 `DisconnectModule`/재연결하는 코드가 생길 때(지금은 `GCoreSystem::Destroy` 안에서만 해제하고 그 뒤 스케줄러가 돌지 않음) | Core 트랙 파일. `GetOwner` 반전은 `IsBoundTo`/객체 기준 `Remove`를 쓰는 곳이 생기면 그때 한 줄 |
| 2-2 | 모듈 `CreateRaw` 태스크를 `ShutdownModule`에서 해제 — `GUIModule.cpp:30`(09-30 1-2 뒤 태스크 1개), `Graphics/JGGraphics.cpp:46-47`, `GameFrameWorks/Core/GameFrameWorksModule.cpp:51` | 2-1과 같음 | JGDev_Graphics `_test`는 소멸, JGEditor에는 태스크 없음 |
| 2-3 | `HGUIModule::ShutdownModule` 정리 순서(`Widgets.clear()`, 메뉴 트리 clear, 백엔드 마지막) — `GUIModule.cpp:34-45` | 위젯 소멸자가 GPU·모듈 자원을 만지게 될 때 | 지금 위젯 소멸자는 비어 있어 GUI 모듈 Deallocate 뒤 Flush에서 회수돼도 무해 |
| 2-6 | `GenerateWidgetGUI` 순회 중 `OpenWidget` 안전화(값 스냅샷 복사) — `GUIModule.cpp:47-56` | 위젯 `OnGenerateGUI`나 `OnUpdate` 안에서 다른 위젯을 여는 코드가 생길 때 | 메뉴 액션과 콘솔 `Submit`은 순회 밖. 09-30 1-2 뒤 `UpdateWidgets`(매 프레임, `OnUpdate` 호출)도 같은 맵 순회라 같은 조건이다. 09-30 현재 위젯 열기는 모듈 시작·메뉴 액션뿐(GFW `GameFrameWorksModule.cpp` 포함)이라 해당 없음 |
| 2-7 | 창 리사이즈 책임을 Graphics 모듈로 — `DX12GUIBackend.cpp:216-233`, `.cpp:68` | GUI를 연결하지 않는 창 호스트가 생길 때 | 구 완료 기준 "JGEditor GUI 없이"는 소멸(JGEditor가 GUI를 연결). `PDX12FrameBuffer::Resize`가 큐 Flush를 하므로 옮겨도 GPU 안전 |
| 2-8 | 멀티 뷰포트 유지 결정 `결정 필요`, 미사용 `CommandAlloc/CommandList` 멤버 제거 — `DX12GUIBackend.h:12-13`, `.cpp:56, 83-85, 212` | 렌더 스레드 도입 또는 뷰포트 자체 큐 경고(Graphics 5-16)가 실제 문제를 일으킬 때 | 권장 유지. 멤버는 `RenderPlatformWindowsDefault` 두 번째 인자로만 넘겨지고 ImGui 구현이 무시 |
| 2-9 | `HGUI::InputText` 512바이트 고정 — `GUI.cpp:65-66` | `HGUI::InputText` 호출자가 생길 때 | 현재 호출자 0. 콘솔은 `InputTextWithHistory`(`CallbackResize`) |
| 2-10 · 2-11 | P2 검증 · 커밋 | P2 항목이 승격될 때 | |
| 3-1 | 죽은 코드(`GUI.h:8` `;;`, `PlotTest`, `Backends/GUIBackends.cpp`(45바이트), `AutoSize`, `enum class GUI_API`, 폰트 주석, `HWidgetLayout`/`HWidgetComponentLayout` 중복) | 해당 파일을 다른 이유로 고칠 때 함께 | 동작 영향 없음. 파일 삭제는 JGBuildTool 재생성 |
| 3-2 | 위젯 식별자 정리(`JG_GENERATED_WIDGET_BODY`가 `GetGUID` 제공, `Begin` 제목 `"%s###%llu"`) — `Widget.cpp:47-49`, `DevConsole/DevConsole.cpp:58-61`, `DevStatistics/MemoryStatistics.cpp` | 같은 제목 위젯 두 개가 필요해질 때 | imgui.ini 창 키가 바뀌어 레이아웃이 한 번 초기화됨 |
| 3-3 | `GUI.module.json` `Asset` 의존 제거 | 모듈 의존을 정리할 일이 생길 때 | GUI 소스에 Asset include 없음. `BuildTool.cpp`에서 include·링크 양쪽 쓰는지 확인 후 |
| 3-4 | 데모 소스 제외(`IMGUI_DISABLE_DEMO_WINDOWS`, `Imgui/implot_demo.cpp` 삭제), `imgui.ini` 옛 창 항목 정리 | `GUI.dll` 크기(8.68MB)가 문제 될 때 | 작은 절약 |
| 3-8 | ImGui 업그레이드 검토(1.91.5+ `ImGui_ImplDX12_InitInfo` + 디스크립터 alloc/free 콜백, 1.92 `ImTextureData`) | ImGui 신기능이 필요해질 때 | 5-5의 SRV 구간 할당을 콜백 할당자로 바꾸는 작업이 따라옴. 검토만 |
| 3-9 · 3-10 | P3 검증 · 커밋 | P3 항목이 승격될 때 | 3-3·3-4는 프로젝트 재생성 |
| BL-1 | 컨텍스트(우클릭) 메뉴 — `GUIModule.h:30` 주석 | 필요한 위젯이 생길 때 | 옛 `HContextMenuBuilder` 참고(`GUI_백업.zip`) |
| BL-2 | 스타일/테마 | 요청 시 | 지금 `StyleColorsDark` 고정(`DX12GUIBackend.cpp`) |
| BL-3 | ~~폰트(TTF 로드 경로, 한글 글리프)~~ → **10-01 완료(DataTable DT-0-3)**, 완료 이력 참고. 남은 것: 에디터 전용 글꼴 파일(`Content/Fonts/EditorKorean.ttf`)을 저장소에 둘지, 범위 줄이기(DataTable 보류 DT-H14) | 배포용 글꼴이 필요할 때 | 시스템 `malgun.ttf` 는 배포 금지(GameUI 설계와 같은 정책) |
| BL-5 | `OpenWidget<T>` 비템플릿 경로(`OpenWidget(JGClass)`) | `Widgets` 맵 타입을 바꿀 때 | 지금은 소비자 DLL이 맵을 직접 조작 → 전부 재빌드 |
| BL-6 | 위젯 GUI 생성 시점(`GraphicsBegin`)을 `End`/`GraphicsEnd` 앞으로 | 위젯 상태 변경이 한 프레임 늦게 보이는 것이 문제 될 때 | DevScene 드로우 기록 순서와 함께 |

---

## 완료 이력

| ID | 항목 | 완료일 | 검증 근거 |
|---|---|---|---|
| 1-11 | **P1 커밋(사용자) "GUI 위젯 갱신 경로를 매 프레임 OnUpdate 하나로, 메뉴 · 백엔드 버그 수정"** — 사용자 일괄 커밋에 들어갔다(1-10 P1 검증은 1-5 뒤 그대로 남음) | 2026-10-01 | `git show 736ddbe --stat`(10-01 12:16 "엔진 구현"): `GUIModule.h/.cpp` · `Widget.h/.cpp` · `WidgetComponent.h` · `Menu/MenuTree.cpp` · `Backends/DX12GUIBackend.cpp` · `Devkit/DevScene.h/.cpp` · `Devkit/DevFeature.cpp`. `git log -S`: 갱신 훅 합치기(`UpdateFrameWidgets` 삭제) · `CanAction` · `PGUIBackend::Shutdown()` · 로그 카테고리 `GUI` 모두 `736ddbe`. 그 파일들에 지금 미커밋 변경 없음(10-11 확인) |
| GG-20 | **게임 UI 스크롤 영역(GFW GG-20, D-35)의 GUI 몫** — `HGUI::InteractiveImage` 가 보이지 않는 버튼 뒤 `ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY / X)` 로 이미지 위 휠을 가짐(창 내용이 넘쳐도 Scene Viewport 창이 휠로 함께 스크롤되지 않음 — 게임 UI 스크롤 · 카메라 줌 모두 해당하던 기존 문제), `HGUIImageInput::WheelHorizontal`(= −`MouseWheelH`, 엔진 뜻 오른쪽 +). JGEditor `SceneViewport` 가 두 축을 게임 뷰포트에 넘김 | 2026-10-04 | `../GameFrameWorks/Files/2026-10-04_GameUI_GG20_verify.txt` §4 — 검증 게임 e2e 28~37단계(휠 · 가로 휠 WM_MOUSEHWHEEL), 메인 반영 10:56(GUI 세션 MainTree), 커밋 `26ac012`(10-17 묶음 커밋, GG-C2 와 같이) |
| GP-C1 | **엔진 입력 계층(Core) — 사용자 결정 D-23(게임 빌드 파이프라인)의 GUI 몫.** 새 `Core/Input/InputEvent.h`(`HInputEvent` · `EInputEventType` · `EPointerButton` · `EInputModifier`, 휠은 `HVector2`), `Core/Platform/Windows/WindowsInput.h/.cpp`(`HWindowsInputTranslator` — Win32 메시지 → 엔진 이벤트, 순수 변환 + 버튼 · 서로게이트 상태, 가상 키 ↔ `EKeyCode`), `HWindowCallBacks::OnInput`, 창 프로시저는 **OnInput 구독자가 있을 때만** 바꿔 보내고 포인터를 잡음(누름 `SetCapture` · 나감 `TrackMouseEvent` · 휠 화면→클라이언트) — 에디터(ImGui)는 구독하지 않아 그대로, `HJWindowArguments::Mode`(`EWindowMode` Windowed · BorderlessFullscreen) · `bResizable`, `HPlatform::GetClipboardText/SetClipboardText`(UTF-8) · `SetImeCompositionRect`, JGConsole `input.selftest`(`-window` 진짜 창 · `-clipboard`). 커밋(10-17 묶음 커밋): Core 새 `Core/Input/InputEvent.h` · `Core/Platform/Windows/WindowsInput.h/.cpp`, 수정 `Core/Platform/JWindow.h` · `Platform.h/.cpp`(DevOps GP-D1 변경과 같은 파일) · `Windows/WindowsJWindow.h/.cpp` = `ee36bb1`, `Programs/JGConsole/InputSelfTest.cpp` = `7b8ee6e`, 증적 · 도구 = `26ac012`. Bin 은 D-40(`6b9d212`)부터 추적하지 않는다 | 2026-10-02 밤 (커밋 `ee36bb1` · `7b8ee6e`, 10-17) | `Files/2026-10-02_GP-C1_input_layer_verify.txt` — `input.selftest` 34/34, 진짜 창에 PostMessage 19 이벤트(창 밖 끌기 · 휠 좌표 · 반복 키 · 홍길동 · U+1F600), 창 모드 2종, 메인 회귀 · 검증 게임 E2E 20단계(에디터 입력 그대로). 클립보드 왕복 · 실제 IME 조합 창 위치는 게임 호스트 연결(GP-F1 · D3) 때 |
| 3-5 | **`imgui.ini` 추적 해제 · 백업 zip 2개 삭제** — 사용자 결정 D-19 ③("권고대로", 총괄 세션이 전함) + 이 세션 사용자 확인("둘 다 진행"). `.gitignore` 에 `Bin/DevelopEngine/imgui.ini` 한 줄(+ 주석) 추가, `git rm --cached Bin/DevelopEngine/imgui.ini`(로컬 파일은 그대로 — 해시 같음, 이제 무시됨), `git rm` `Source/Runtime/GUI_백업.zip`(871KB) · `Source/Editor/JGEditor/백업.zip`(14KB) — 옛 설계 참고용이었고 git 이력(`c5944f4` ~ `efba16a`)에 남는다. 이제 실행만으로 `git status` 가 바뀌지 않고, 새 PC · 새 워크트리 에디터는 기본 창 배치로 시작한다. 다른 세션 검증 절차(로컬 `imgui.ini` 백업 → `[JGWidget][Open]` 편집 → 실행 → 백업으로 되돌림)는 파일 복사라 그대로 동작한다(새 워크트리에는 파일이 없으니 만들어 쓴다) | 2026-10-02 (커밋 10-17: `.gitignore` 줄 = `37cffb3`, `imgui.ini` 추적 해제 = `6b9d212`(D-40 — 이제 `Bin/` 전체를 추적하지 않음), zip 2개 삭제 = `26ac012`) | 10-02 당시 `git status`: `D  Bin/DevelopEngine/imgui.ini`(로컬 파일 남음) · `D` zip 2 · ` M .gitignore`, `git check-ignore -v Bin/DevelopEngine/imgui.ini` → `.gitignore:8` |
| — | GUI 현황 분석·TODO 작성(P1 11 · P2 11 · P3 10 · 백로그 6) | 2026-09-21 | `Files/GUI_현황분석_2026-09-21.md`, `Files/GUI_TODO.md`, `Files/2026-09-21_GUI_현황분석.md` |
| — | (Memory 트랙) `HGUI::PlotBarGroups` 틱 배열 수정(메모리 위젯 크래시) | 2026-09-28 | 커밋 `d457b92` `GUI.cpp` |
| 1-6 | DevConsole 모듈 GUI 검사 반전·static 캐시 — DevConsole 3-1이 그 API(`Register/UnRegisterConsoleCommand`, `GetCheckedGUIModule`)를 통째로 삭제해 대체 | 2026-09-29 | `Source/Editor/DevConsole/DevConsoleModule.cpp`(40줄), grep 0건, 커밋 `4c8ac73` |
| 1-9 일부 | `DevConsoleModule.cpp` 카테고리 `DevStatistics` → `DevConsole`, 메시지 수정 | 2026-09-29 | 같은 파일 `:22`, 커밋 `4c8ac73` |
| — | (DevConsole 2-1) `HGUI::Text`가 문자열을 printf 서식으로 넘기던 버그 → `TextUnformatted`(메모리 통계 창 peak 열·`%` 복구, 콘솔 로그 뷰 `%s` 안전) | 2026-09-29 | `Files/2026-09-29_devconsole_ui_memorywindow.png` vs `Document/Memory/Core/Files/2026-09-28_phase2_memwidget_capture.png`, `GUI.cpp:49-59`, 커밋 `4c8ac73` |
| — | (DevConsole 2-3) `HGUI::BeginChild/EndChild(bStickToBottom)/GetFrameHeightWithSpacing/InputTextWithHistory` | 2026-09-29 | `GUI.h:28-36`, DevConsole 창 동작, 커밋 `4c8ac73` |
| 2-4 | GUI 종료 전 GPU 완료 대기 — (Graphics 5-5) `Shutdown`이 `PDirectX12API::WaitForGPUIdle()` 뒤 SRV 힙·ImGui 리소스 해제 | 2026-09-29 | 커밋 `1308fd5`, `DX12GUIBackend.cpp:165-173`, `Document/Memory/Graphics/Files/Graphics_TODO.md` 5-5 (3): 종료 시 D3D12 메시지 0(`PDirectX12API::Destroy`가 마지막으로 비움) |
| 2-5 | SRV 링 상한 검사·프레임별 세그먼트 — (Graphics 5-5) `SrvRegionStart/End`를 프레임 인덱스로 나눔(511슬롯), 초과 시 오류 로그 1회 후 마지막 슬롯 재사용, `BufferCount >= FramesInFlight` 검사 | 2026-09-29 | 커밋 `1308fd5`, `DX12GUIBackend.h:18-21`, `.cpp:94, 120-126, 254-262` |
| 3-6 | JGEditor 모듈 정리 — (GameModule R3) JGEditor가 GUI 호스트(엔진 모듈 6개 연결 순서 · `Dev/DevAI` 메뉴 · 시작 시 DevFeature 열기), 옛 잔재(`GGUIGlobalSystem`·`HMenuBuilder`·`BuildMainMenu`) 제거, JGDev_Graphics 삭제, 런처 LaunchModule = JGEditor | 2026-09-30 | `Files/GUI_TODO.md` 3-6, `Source/Editor/JGEditor/JGEditor.cpp`, 메인 트리 빌드 오류 0 · 런처 종료 0, 커밋 `4c8ac73` |
| — | GUI 진행 보고: 미완료 12항목을 소스와 대조(코드 상태·파일:행 그대로), 문서의 "미커밋" 표기 정정. 코드 변경·실행 없음 | 2026-09-30 | `Document/GUI_진행보고_2026-09-30.html` |
| 1-2 | **위젯 갱신 훅을 매 프레임 `OnUpdate` 하나로 합침**(사용자 결정 09-30). `GUIModule.cpp`에서 30Hz `Schedule`·`UpdateFrameWidgets`를 지우고 `UpdateWidgets`를 `ScheduleByFrame(Update)`로 바꿈(열린 위젯이면 `Update()`). `Widget.h/.cpp`·`WidgetComponent.h`에서 `EWidgetFlags`·`WidgetFlags`·`GetFlags`·`OnUpdateFrame`·`UpdateFrame` 삭제, `OnUpdate`에 호출 시점 주석. 결정 경위: 사용자 질문 "Update/UpdateFrame을 왜 나눴나". 분리는 `bd98a67`(09-15)에서 처음 들어왔고 두 훅 모두 소비자 0이었다. 30Hz는 엔진 유일의 시간 간격 스케줄이고 스로틀(`Scheduler.cpp:168-177`)이다. 옛 `WWidget`도 매 프레임 하나였다. 채택하지 않은 대안: `AllowUpdate \| AllowUpdateFrame` + setter | 2026-09-30 | `Files/2026-09-30_1-2_update_merge_verify.txt`: 빌드 오류 0(LNK4098만), crashwalk 60초 종료 0 · `[error]`/`[critical]` 0 · live blocks 0, 지운 심볼 grep 0건(GFW 새 위젯·게임 GfwView 포함). 커밋 `736ddbe`(10-01 12:16) |
| 1-3 | DevScene 렌더·리드백 카운터를 `OnGenerateGUI` → 새 `JGDevScene::OnUpdate`로(`OnGenerateGUI`는 `HGUI::Image`·상태 텍스트만). 출력 텍스처가 1920×1080 고정이라 렌더가 GUI 생성 뒤에 와도 안전하다 | 2026-09-30 | 리드백 2줄 `saved ok`가 `OnUpdate`에서 나옴(= 합친 경로가 매 프레임 호출됨), 두 실행 모두 `DevScene_Readback_*.png` 2장이 커밋본과 바이트 동일, 캡처 `Files/2026-09-30_gui_update_merge_capture.png`(09-29 기준 캡처와 같은 모습). 커밋 `736ddbe`(10-01 12:16). Graphics 트랙 파일이라 Graphics 현황에 기록 |
| 1-4 | 메뉴 활성 판정을 `Visibility` 재평가 → `CanAction`으로 — `Menu/MenuTree.cpp:77-81` | 2026-09-30 | 빌드·회귀 통과. `CanAction`을 바인딩한 메뉴가 없어 동작 변화 없음. 회색 표시 시각 확인은 1-10. 커밋 `736ddbe`(10-01 12:16) |
| 1-7 | `PDX12GUIBackend::Shutdown` 끝에 `PGUIBackend::Shutdown()`(델리게이트 해제) | 2026-09-30 | 종료 코드 0, 종료 순서 로그 정상. 커밋 `736ddbe`(10-01 12:16) |
| 1-8 | `JGDevFeature::OnLayout`의 DevSettings 조건을 `DevSettings.IsValid()`로 — `Devkit/DevFeature.cpp:52` | 2026-09-30 | 코드 확인, 캡처에 DevSettings 패널 250폭 그대로. 커밋 `736ddbe`(10-01 12:16) |
| 1-9 | 로그 카테고리 `JGDev_GraphicsModule` → `GUI` — `GUIModule.cpp:21`(`DevConsoleModule.cpp` 부분은 09-29 완료) | 2026-09-30 | 코드 확인(Graphics 연결 실패 경로에서만 찍힘). 커밋 `736ddbe`(10-01 12:16) |
| BL-3 | (DataTable 트랙 DT-0-3) **에디터 기본 글꼴에 한글 합침** — `Backends/DX12GUIBackend.cpp` `AddKoreanGlyphsToDefaultFont`: `Content/Fonts/EditorKorean.ttf` → 없으면 `C:/Windows/Fonts/malgun.ttf` → 없으면 경고 한 줄, 14px `MergeMode`, `GetGlyphRangesKorean`. 큰 글꼴(26px)에는 합치지 않음 | 2026-10-01 | `Document/Memory/GameFrameWorks/Files/2026-10-01_datatable_verify.txt` §5: 아틀라스 512×256 → 1024×2048(8 MB) · 시작 때 123 ms, 한글 셀 · 필터 캡처, 메인 트리 에디터 실행 경고 없음 · 종료 0. 커밋 `efba16a`(10-01 16:58, DataTable DT-C1) |
| — | (DataTable 트랙) **새 컨트롤 `Grid/GUIGrid.h/.cpp` `PGUIGrid`**(데이터를 모르는 스프레드시트 그리드: 고정 행 번호 · 키 열, 가상화, 사각 선택, 키보드, 텍스트 · 체크 · 콤보 편집기, 이벤트 17종) + **`HGUI` 래퍼 13개**(`GUI.h/.cpp` 끝 한 묶음: `BeginDocumentTabItem` · `OpenPopup` · `BeginPopup` · `BeginPopupModal` · `EndPopup` · `CloseCurrentPopup` · `MenuItem` · `SetNextItemWidth` · `BeginDisabled/EndDisabled` · `TextWrapped` · `SetKeyboardFocusHere` · `SameLineAt`). 3-7 방침(즉시 모드 함수 = `HGUI` 정적)대로. BL-1(모듈 수준 컨텍스트 메뉴 등록)은 아님 — 위젯이 `OpenPopup/BeginPopup` 으로 직접 그린다 | 2026-10-01 | DataTable V2(그리드 · 메뉴 · 모달 · 탭), 메인 빌드 exit 0. 설계 `GameFrameWorks/Files/DataTable_설계_2026-10-01.md` §14-1. 커밋 `efba16a`(10-01 16:58, DataTable DT-C1. `GUIGrid.cpp` 의 그 뒤 변경(D-22 ② 배율)은 `26ac012`(10-17)) |
| BL-4 | **위젯 열림 상태 저장 · 복원**(사용자 보고 "에디터 레이아웃 저장이 안 됨", JGEditor 세션). 창 위치 · 도킹은 원래 저장되고 있었고 열린 창 목록만 없었다. `GUIModule.h/.cpp`: `Widgets` 키 `HGuid` → `JGType`(클래스 이름으로 리플렉션 재생성), `imgui.ini` `[JGWidget][Open]` 절(`AddSettingsHandler`, 이름순), 시작 때 ini 미리 읽기, 첫 프레임에 저장된 열린 위젯 다시 열기(로그 `Layout: reopened ...`), 매 프레임 상태 기록, `OpenWidgetByDefault<T>()`(사용자가 닫아 둔 기본 창은 안 엶). `JGEditor.cpp` 씬 뷰포트를 그것으로. `WidgetComponents` 직렬화는 하지 않음(쓸 곳 없음) | 2026-10-01 | `Files/2026-10-01_layout_restore_verify.txt`: GUI · DevConsole · DevStatistics · GameGUI · JGEditor 빌드 exit 0, 메인 트리 에디터 저장 → 재시작 복원, 사용자 ProjectAH ini 로 배치 그대로 복원(`Files/2026-10-01_layout_restored.png`), 닫은 기본 창 유지(`Files/2026-10-01_layout_closed_default_stays_closed.png`), 종료 0 · live blocks 0. 커밋 `736ddbe`(10-01 12:16) |

---

## 제외 항목

| 항목 | 이유 |
|---|---|
| 1-9의 `JGDev_Graphics.cpp:43, 54, 66, 78` 메시지·카테고리 | 파일 삭제(3-6)로 소멸 |
| 2-2의 `JGDev_Graphics.cpp:72` `_test` 태스크 | 같음. JGEditor는 스케줄 태스크를 등록하지 않음 |
| 1-11 · 2-11 커밋 목록의 `JGDev_Graphics/JGDev_Graphics.cpp`, `DevConsole/DevConsoleModule.cpp` | 전자 소멸, 후자는 DevConsole 트랙 커밋에 포함 |
| 2-5의 "Graphics 5-5 **전에** 끝나야 한다" 의존 문구 | 5-5가 이 항목을 포함해 완료했다 |
| 2-7의 완료 기준 "GUI 없이 Graphics만 연결한 상태(JGEditor 런치)" | JGEditor가 GUI를 연결한다. 항목 자체는 보류 |
| 1-1 `UpdateFrameWidgets`가 `UpdateFrame()`을 부르게 | 1-2 합치기(09-30)로 `UpdateFrameWidgets`·`UpdateFrame` 자체가 없어져 소멸 |

## D-22 ② 에디터 DPI — GUI 몫 (2026-10-03, GUI 세션) — 완료 · 메인 반영(19:19) · 커밋 `26ac012`(10-17)

- 상태: **완료 · 메인 반영 · 커밋 `26ac012`(10-17 15:03 엔진 총괄 묶음 커밋 — GUI 7파일 모두).** GUI 몫 구현 · 실행 검증(작업 폴더 `Temp/wt/dpigui`, 17:3x) → JGEditor 합친 검증(`Temp/wt/dpi`, 18:00) → 사용자 확인 19:07 "권고대로"(저장 칸 배치 (가) 지금대로) → JGEditor 가 `MainTree.cmd -name=JGEditor hold`(19:10:45 ~ release 19:18:52)로 17파일(JGEditor 10 + GUI 7)을 메인에 넣음. GUI 7파일의 메인 판은 10-02 23:00 뒤로 바뀌지 않아 dpi 판 그대로 들어갔고, `GUI.cpp` 만 줄 끝을 메인처럼 CRLF 로 맞춤(내용 같음). 메인 검사(JGEditor): 빌드 exit 0 · 경고 0, gameui 300 · input 34 · gmtest · console 64 · datatable 127, 에디터 "GUI DPI scale 1.25 (editor font malgun.ttf, 16px)" → 150% 로 옮기면 "fonts and style rebuilt", 종료 0 · live 0, 메인 엔진 검증 게임 e2e 22단계 같음. 기록 `Files/2026-10-03_D-22-2_merged_verify.txt` §7. 작업 폴더 `Temp/wt/dpi` · `Temp/wt/dpigui` 는 총괄 지시로 JGEditor 가 지움.
- 바뀐 파일: `GUI/Backends/GUIBackend.h`(`DpiScale` · `GetDpiScale()`), `GUI/Backends/DX12GUIBackend.h/.cpp`(배율로 글꼴 — 정수 배율은 ProggyClean 13 · 26 × 배율 + 한글 14 × 배율, 정수가 아니면 한글 글꼴(맑은 고딕) 하나로 13 · 26 × 배율; `BaseStyle` × `ScaleAllSizes`; WM_DPICHANGED → `PendingDpiScale` → 다음 `NewFrame` 에서 GPU 대기 · `ApplyDpiScale` · `ImGui_ImplDX12_InvalidateDeviceObjects`), `GUI/GUI.h/.cpp`(임시 스텁 → `HGUI::GetDpiScale()` 실제 구현, JGEditor 요청 `HGUI::IsAppFocusLost()`, 내부 고정 픽셀에 `ScaledPx`), `GUI/Grid/GUIGrid.cpp`(행 번호 칸 · 기본 칸 폭 · 칸 여백 · 콤보 크기 × 배율), `Editor/DevStatistics/MemoryStatistics.cpp`(Memory 트랙 창 — 카드 높이 · 표 열 폭 · 간격 · 그래프 높이 × 배율. 안 고치면 카드 둘째 줄 · 표 글자가 잘림).
- 규칙: HGUI 함수가 받는 크기 · 폭 · 간격은 실제 픽셀(부르는 쪽이 `GetDpiScale` 을 곱함). 테두리 1px 는 곱하지 않음. ImPlot 기본 스타일 · DevConsole 작은 간격은 그대로(잘림 없음).
- imgui.ini: 배율로 바꾸지 않음(사용자 19:07 (가) 지금대로 — 끝). **정정(JGEditor 합친 검증 18시경)**: 도킹 칸 너비도 픽셀 그대로 저장 · 적용된다(비율이 아님) — 처음 실행 때 옆 칸(예: MemoryStatistics 777px)이 125% 에서 약 20%, 150% 에서 약 33% 좁아 보이고 표 오른쪽 열이 잘린다. 끌어서 넓히면 그 뒤 저장된다. 사용자 확인 항목 — 바꾸려면 "저장된 배치를 배율만큼 한 번 키우기"(imgui.ini 에 배율을 같이 저장, 다르면 떠 있는 창 자리 · 크기와 도킹 칸 크기를 맞춤) 약 0.5~1일, GUI 세션.
- 검증(이 PC 주 125% · 보조 150%): 로그 "GUI DPI scale 1.25 (editor font malgun.ttf, 16px)" → 150% 모니터로 옮기면 "1.25 -> 1.50: fonts and style rebuilt" → 되돌리면 "1.50 -> 1.25 …", 종료 0 · live 0 · 디버그 출력(D3D12) 0, gameui 256 · input 34. 기록 `Files/2026-10-03_D-22_GUI_dpi_verify.txt`, 캡처 `Files/2026-10-03_D-22_GUI_{125,150_moved,125_before_memstats_fix}.png`.
- 알려진 것(보류 후보): 실행 중 모니터를 옮기면 ImGui 도킹 칸 픽셀 폭이 그대로라 오른쪽 칸이 논리 폭으로 좁아짐(칸을 넓히면 됨). 첫 실행에서 되돌리기 창 이동이 한 번 안 먹음(원인 미확인, 다시 해 보면 정상).
- 메인 반영 합의(10-03 18시경, JGEditor — 19:19 그대로 끝남): 사용자 확인 뒤 JGEditor 가 `MainTree.cmd -name=JGEditor hold` 로 17파일(JGEditor 10 + GUI 7, `Temp/wt/dpi` 판)을 함께 넣고 PreBuild → build → e2e · GfwView · 검사 → release. 그동안 GUI 는 메인 빌드 · 실행을 쉰다. GUI 7파일의 메인 판은 10-02 23:00 뒤로 안 바뀜(확인함) — 넣을 때 그보다 늦게 바뀐 것이 있으면 GUI 가 3-way 로 합친다. 사용자가 imgui.ini (나)를 고르면 GUI 가 `Temp/wt/dpigui` 에서 만들어 `Temp/wt/dpi` 로 넘기고 JGEditor 가 다시 합쳐 검증한다.
- 다음 일(총괄 순서): ER-016 게임 UI 기본 글꼴을 게임 Content 에서(요청서 `ProjectAH/Document/엔진요청/ER-016_GameUI_기본_글꼴을_게임_Content_에서_정하기.md`).
