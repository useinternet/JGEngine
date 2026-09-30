# GUI TODO (ImGui 모듈)

갱신 2026-09-30. 구 TODO(`Files/GUI_TODO.md`, 2026-09-21 작성, 09-29 · 09-30 표시 추가)를 통합·최신화했다. **항목 번호는 구 TODO 그대로**(DevConsole·GameModule·Graphics 문서가 참조). 현황·구조·함정·검증 방법은 `현황.md` §2.
기준: 엔진 기반 구축 · 과설계 금지. 코드가 틀린 확정 버그와 지금 겪는 문제만 미완료 표에 두고, 증상 없는 수명 위험·정리·백로그는 보류 표에 "다시 볼 조건"과 함께 둔다. `결정 필요`는 착수 전 사용자 확인. 항목이 끝나면 완료 이력 표로 옮기고 근거를 적는다.
파일:행은 2026-09-30 소스 기준(구 TODO의 09-21 기준 행과 다른 곳은 비고에 적었다). 런처 호스트는 JGEditor다(JGDev_Graphics는 삭제).

---

## 다음에 할 일 Top 5 (권장 순서 — 착수 대상은 사용자가 정한다)

| 순서 | 항목 | 착수 조건 | 완료 기준 |
|---|---|---|---|
| 1 | **1-2 위젯 업데이트 플래그 기본값·설정 API** `결정 필요` | 사용자 결정. 권장: 기본값 `AllowUpdate \| AllowUpdateFrame`(열린 위젯은 갱신) + `protected` `SetWidgetFlags/AddWidgetFlags/RemoveWidgetFlags`, `AllowAlways*`는 닫혀도 갱신할 위젯만 | 플래그를 건드리지 않은 위젯이 열려 있으면 `OnUpdate`(30Hz)·`OnUpdateFrame`(매 프레임)을 받는다. `UpdateWidgets/UpdateFrameWidgets`(`GUIModule.cpp:90-136`)의 복붙 판정을 `shouldUpdate(Widget, AlwaysFlag, OpenFlag)` 하나로 |
| 2 | **1-1 `UpdateFrameWidgets`가 `UpdateFrame()`을 부르게** + **1-3 DevScene 렌더·리드백 카운터를 `OnUpdateFrame`으로** | 1 뒤. `Source/Runtime/Devkit/DevScene.cpp`는 Graphics 트랙 파일(미커밋 변경 있음) → `git status`로 확인하고 그 세션과 조정 | `GUIModule.cpp:133` `Widget->UpdateFrame()`. `DevScene.cpp:162-185`의 `Renderer->Render(*Scene, Camera)`와 리드백 카운터(`FramesSinceMeshLoaded`, `DumpReadbackOnce`)가 새 `OnUpdateFrame()`으로, `OnGenerateGUI`에는 `HGUI::Image`·상태 텍스트만. 캡처에 X Bot이 이전과 같고 리드백 PNG 2장 생성, `:170-171` 주석 갱신 |
| 3 | **소규모 확정 버그 4건: 1-4 · 1-7 · 1-8 · 1-9** | 없음(각 1~3줄) | 1-4 `MenuTree.cpp:78-80` `bEnable`을 `CanAction.IsBound() ? CanAction.Execute() : true`로 / 1-7 `DX12GUIBackend.cpp:165-185` `Shutdown` 끝에 `PGUIBackend::Shutdown()` / 1-8 `DevFeature.cpp:52` `DevScene.IsValid()` → `DevSettings.IsValid()` / 1-9 `GUIModule.cpp:21` 카테고리 `JGDev_GraphicsModule` → `GUI`. 빌드 통과, 60초 회귀 종료 0 |
| 4 | **3-5 `imgui.ini` 추적 해제 · 백업 zip 처리** `결정 필요` | 사용자 결정 | `.gitignore`에 `Bin/DevelopEngine/imgui.ini` + `git rm --cached` → 실행만으로 `git status`가 바뀌지 않는다. zip 2개는 결정대로(`git rm` 또는 유지) |
| 5 | **1-10 P1 검증 → 1-11 커밋**(사용자) | 1~3 뒤 | 공통 루프(`현황.md` §2.5) + 캡처에 메뉴바(`Dev`, `Windows`)·DevFeature 창. 1-4·1-5 임시 확인 코드 제거를 `git diff`로. 커밋 파일: `GUI/GUIModule.cpp`, `Widget.h/.cpp`, `Menu/MenuTree.h/.cpp`, `Backends/DX12GUIBackend.cpp`, `Devkit/DevScene.h/.cpp`, `Devkit/DevFeature.cpp`. DevConsole 커밋(`GUI.h/.cpp`)이 먼저 들어가야 충돌이 없다 |

---

## 미완료 항목 (엔진 범위)

| ID | 항목 | 우선순위 | 상태 | 선행조건 | 완료 기준 | 비고 |
|---|---|---|---|---|---|---|
| 1-1 | `UpdateFrameWidgets`가 `Update()` 대신 `UpdateFrame()` 호출 — `Source/Runtime/GUI/GUIModule.cpp:133` | 높음 | 미착수 | 1-2와 함께 | `OnUpdateFrame`이 위젯·컴포넌트에서 호출됨(1-3으로 확인) | `JGWidget::UpdateFrame()` 호출자 0 |
| 1-2 | 위젯 플래그 기본값·설정 API — `Widget.h:17-25, 49` | 높음 | `결정 필요` | 사용자 | Top 1 | setter가 없어 모든 위젯이 `None` → 두 스케줄이 빈 순회. DevScene이 `OnGenerateGUI`에서 렌더하는 이유. GFW 2-3 DevView가 위젯이면 여기에 걸린다 |
| 1-3 | DevScene 렌더·리드백 카운터 `OnGenerateGUI` → `OnUpdateFrame` — `Devkit/DevScene.cpp:162-185`, `DevScene.h:45` | 높음 | 미착수 | 1-1 · 1-2, Graphics 트랙과 파일 조정 | Top 2 | `Update` 버킷은 `GraphicsBegin`(GUI 생성) 뒤·`GraphicsEnd` 앞이라 같은 프레임. 구 TODO의 `RenderScene()`·`:182-208`은 5-7 `PScene` 이후 이름·행이 바뀜. 5-25 리드백 덤프(콘솔 명령 예정)도 여기 있음 |
| 1-4 | 메뉴 활성 판정에 `CanAction` 사용 — `Menu/MenuTree.cpp:78-80` | 중간 | 미착수 | 없음 | `CanAction`이 false인 임시 메뉴가 회색 비활성(확인 후 제거) | 증상 없음(`CanAction`을 바인딩한 메뉴가 아직 없음). 복붙으로 `Visibility`를 재평가 |
| 1-5 | 메뉴 트리 키를 전체 경로로 — `Menu/MenuTree.cpp:21, 36, 39`, `MenuTree.h` | 중간 | 미착수 | 없음 | `Windows/Open`·`File/Open`을 동시에 등록해도 각각 나옴(확인 후 제거), 기존 3경로 유지, 같은 경로 중복·Action 리프에 자식 붙을 때 경고 로그 | 현재 3경로(`Dev/DevAI`, `Windows/DevConsole`, `Windows/Statistics/Memory`)는 충돌 없음. 메뉴가 늘면 바로 걸림 |
| 1-7 | `PDX12GUIBackend::Shutdown`에서 `PGUIBackend::Shutdown()` 호출 — `Backends/DX12GUIBackend.cpp:165-185` | 낮음 | 미착수 | 없음 | 코드 확인, 종료 0 | 백엔드가 곧 파괴되어 지금 무해. Graphics 5-5가 `WaitForGPUIdle`은 넣었고 기반 호출은 그대로 빠져 있음 |
| 1-8 | `JGDevFeature::OnLayout` 복붙 조건 — `Devkit/DevFeature.cpp:52` | 낮음 | 미착수 | 없음 | 코드 확인 | `DevSettings`가 항상 있어 증상 없음. Devkit 파일 |
| 1-9 | 로그 카테고리 — `GUIModule.cpp:21` `JGDev_GraphicsModule` → `GUI` | 낮음 | 일부 완료 | 없음 | 실행 로그의 GUI 줄이 `[GUI]` | `DevConsoleModule.cpp` 부분은 완료(09-29). `JGDev_Graphics.cpp` 부분은 파일 삭제로 소멸 |
| 1-10 | P1 검증 | — | 미착수 | 1-1 ~ 1-9 | 공통 루프 + 메뉴바·DevFeature 창 캡처, 임시 코드 제거 `git diff` | 캡처 1장을 `Files/`에 남긴다 |
| 1-11 | P1 커밋(사용자) "GUI 위젯 업데이트 경로 복구, 메뉴 트리 버그 수정" | — | 대기 | 1-10 | Top 5 파일 목록 | 구 목록의 `DevConsoleModule.cpp`·`JGDev_Graphics.cpp`는 제외 |
| 3-5 | `imgui.ini` 추적 해제, 백업 zip 2개 — `Bin/DevelopEngine/imgui.ini`, `Source/Runtime/GUI_백업.zip`(871KB), `Source/Editor/JGEditor/백업.zip`(14KB) | 중간 | `결정 필요` | 사용자 | Top 4 | 실행마다 `git status`에 `MM imgui.ini`(스테이지·미스테이지 섞임). 다른 PC로 옮길 때 절반만 커밋될 위험을 메인 세션이 확인했다. zip은 옛 설계(GGUIGlobalSystem·Builder·컴포넌트 9종) 참고용 |
| 3-7 | `HGUI` 확장 방향 기록 — `GUI.h` 상단 | 낮음 | 결정 기록만 남음 | 없음 | `GUI.h` 상단 주석 한 줄 + 이 표에서 완료 이력으로 | 즉시 모드 함수 = `HGUI` 정적, 상태를 가진 것만 `JGWidgetComponent`. DevConsole 2-3이 이 방침으로 4개(`BeginChild/EndChild/GetFrameHeightWithSpacing/InputTextWithHistory`)를 추가했다. 함수 추가는 필요해질 때 항목을 따로 만든다 |

---

## 보류 (증상 없음 · 정리 · 백로그 — 다시 볼 조건과 함께)

| ID | 항목 | 다시 볼 조건 | 메모 |
|---|---|---|---|
| 2-1 | 스케줄러 `Unschedule(id)` 추가 + 끝난 태스크 풀 누수 + 빈 `Destroy` + `CreateSP` 대상 사망 판정 — `Core/Thread/Scheduler.h/.cpp`. 부수: `PSPDelegate::GetOwner()` 조건 반전(`Delegate.h:253`, 한 줄) | 런타임에 모듈을 `DisconnectModule`/재연결하는 코드가 생길 때(지금은 `GCoreSystem::Destroy` 안에서만 해제하고 그 뒤 스케줄러가 돌지 않음) | Core 트랙 파일. `GetOwner` 반전은 `IsBoundTo`/객체 기준 `Remove`를 쓰는 곳이 생기면 그때 한 줄 |
| 2-2 | 모듈 `CreateRaw` 태스크를 `ShutdownModule`에서 해제 — `GUIModule.cpp:30-31`, `Graphics/JGGraphics.cpp:46-47`, `GameFrameWorks/Core/GameFrameWorksModule.cpp:51` | 2-1과 같음 | JGDev_Graphics `_test`는 소멸, JGEditor에는 태스크 없음 |
| 2-3 | `HGUIModule::ShutdownModule` 정리 순서(`Widgets.clear()`, 메뉴 트리 clear, 백엔드 마지막) — `GUIModule.cpp:34-45` | 위젯 소멸자가 GPU·모듈 자원을 만지게 될 때 | 지금 위젯 소멸자는 비어 있어 GUI 모듈 Deallocate 뒤 Flush에서 회수돼도 무해 |
| 2-6 | `GenerateWidgetGUI` 순회 중 `OpenWidget` 안전화(값 스냅샷 복사) — `GUIModule.cpp:47-56` | 위젯 `OnGenerateGUI` 안에서 다른 위젯을 여는 버튼이 생길 때 | 메뉴 액션과 콘솔 `Submit`은 순회 밖 |
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
| BL-3 | 폰트(TTF 로드 경로, 한글 글리프, `Source/Font` 폴더) | 한글 표시가 필요할 때 | 그때까지 표시 문자열은 영어(DevConsole 규칙) |
| BL-4 | 위젯 상태 저장(`bOpen`·`WidgetComponents` 직렬화 코드는 생성돼 있으나 저장/복원 호출 없음) | 레이아웃 복원이 필요할 때 | imgui.ini와 함께 설계 |
| BL-5 | `OpenWidget<T>` 비템플릿 경로(`OpenWidget(JGClass)`) | `Widgets` 맵 타입을 바꿀 때 | 지금은 소비자 DLL이 맵을 직접 조작 → 전부 재빌드 |
| BL-6 | 위젯 GUI 생성 시점(`GraphicsBegin`)을 `End`/`GraphicsEnd` 앞으로 | 위젯 상태 변경이 한 프레임 늦게 보이는 것이 문제 될 때 | DevScene 드로우 기록 순서와 함께 |

---

## 완료 이력

| ID | 항목 | 완료일 | 검증 근거 |
|---|---|---|---|
| — | GUI 현황 분석·TODO 작성(P1 11 · P2 11 · P3 10 · 백로그 6) | 2026-09-21 | `Files/GUI_현황분석_2026-09-21.md`, `Files/GUI_TODO.md`, `Files/2026-09-21_GUI_현황분석.md` |
| — | (Memory 트랙) `HGUI::PlotBarGroups` 틱 배열 수정(메모리 위젯 크래시) | 2026-09-28 | 커밋 `d457b92` `GUI.cpp` |
| 1-6 | DevConsole 모듈 GUI 검사 반전·static 캐시 — DevConsole 3-1이 그 API(`Register/UnRegisterConsoleCommand`, `GetCheckedGUIModule`)를 통째로 삭제해 대체 | 2026-09-29 | `Source/Editor/DevConsole/DevConsoleModule.cpp`(40줄), grep 0건 |
| 1-9 일부 | `DevConsoleModule.cpp` 카테고리 `DevStatistics` → `DevConsole`, 메시지 수정 | 2026-09-29 | 같은 파일 `:22` |
| — | (DevConsole 2-1) `HGUI::Text`가 문자열을 printf 서식으로 넘기던 버그 → `TextUnformatted`(메모리 통계 창 peak 열·`%` 복구, 콘솔 로그 뷰 `%s` 안전) | 2026-09-29 | `Files/2026-09-29_devconsole_ui_memorywindow.png` vs `Document/Memory/Memory/Files/2026-09-28_phase2_memwidget_capture.png`, `GUI.cpp:49-59`(미커밋) |
| — | (DevConsole 2-3) `HGUI::BeginChild/EndChild(bStickToBottom)/GetFrameHeightWithSpacing/InputTextWithHistory` | 2026-09-29 | `GUI.h:28-36`, DevConsole 창 동작(미커밋) |
| 2-4 | GUI 종료 전 GPU 완료 대기 — (Graphics 5-5) `Shutdown`이 `PDirectX12API::WaitForGPUIdle()` 뒤 SRV 힙·ImGui 리소스 해제 | 2026-09-29 | 커밋 `1308fd5`, `DX12GUIBackend.cpp:165-173`, `Document/Memory/Graphics/Files/Graphics_TODO.md` 5-5 (3): 종료 시 D3D12 메시지 0(`PDirectX12API::Destroy`가 마지막으로 비움) |
| 2-5 | SRV 링 상한 검사·프레임별 세그먼트 — (Graphics 5-5) `SrvRegionStart/End`를 프레임 인덱스로 나눔(511슬롯), 초과 시 오류 로그 1회 후 마지막 슬롯 재사용, `BufferCount >= FramesInFlight` 검사 | 2026-09-29 | 커밋 `1308fd5`, `DX12GUIBackend.h:18-21`, `.cpp:94, 120-126, 254-262` |
| 3-6 | JGEditor 모듈 정리 — (GameModule R3) JGEditor가 GUI 호스트(엔진 모듈 6개 연결 순서 · `Dev/DevAI` 메뉴 · 시작 시 DevFeature 열기), 옛 잔재(`GGUIGlobalSystem`·`HMenuBuilder`·`BuildMainMenu`) 제거, JGDev_Graphics 삭제, 런처 LaunchModule = JGEditor | 2026-09-30 | `Files/GUI_TODO.md` 3-6, `Source/Editor/JGEditor/JGEditor.cpp`, 메인 트리 빌드 오류 0 · 런처 종료 0(미커밋) |

---

## 제외 항목

| 항목 | 이유 |
|---|---|
| 1-9의 `JGDev_Graphics.cpp:43, 54, 66, 78` 메시지·카테고리 | 파일 삭제(3-6)로 소멸 |
| 2-2의 `JGDev_Graphics.cpp:72` `_test` 태스크 | 같음. JGEditor는 스케줄 태스크를 등록하지 않음 |
| 1-11 · 2-11 커밋 목록의 `JGDev_Graphics/JGDev_Graphics.cpp`, `DevConsole/DevConsoleModule.cpp` | 전자 소멸, 후자는 DevConsole 트랙 커밋에 포함 |
| 2-5의 "Graphics 5-5 **전에** 끝나야 한다" 의존 문구 | 5-5가 이 항목을 포함해 완료했다 |
| 2-7의 완료 기준 "GUI 없이 Graphics만 연결한 상태(JGEditor 런치)" | JGEditor가 GUI를 연결한다. 항목 자체는 보류 |
