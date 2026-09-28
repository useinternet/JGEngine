# GUI 모듈 할 일 목록 (순차 진행용)

작성 2026-09-21. 근거는 `Document/GUI_현황분석_2026-09-21.md`(3-A 확정 버그, 3-B 수명·동기화 위험, 3-C 정리 대상)와 `Document/Memory/2026-09-21_GUI_현황분석.md`.
위에서부터 순서대로 진행한다. P1(버그) → P2(수명·동기화) → P3(정리) 순서이며, 각 단계 끝에 검증과 커밋 항목이 있다.
한 항목이 끝나면 `[x]`로 바꾸고, 한 단계가 끝나면 `Document/Memory/2026-09-21_GUI_현황분석.md` 끝에 진행 상황을 한 줄 추가한다.
`결정 필요` 표시 항목은 착수 전에 사용자 확인을 받는다. 파일:행 표기는 2026-09-21 소스 기준이므로 편집 후에는 어긋날 수 있다.

검증 루프(모든 단계 공통, `Document/Memory/2026-09-18_Phase5_전송관리자.md` "검증 방법"과 같다):
JGCLASS/JGPROPERTY를 바꿨으면 `Build/BatchFiles`에서 `JGHeaderTool.exe`, 파일을 추가·삭제했으면 `JGBuildTool.exe`(둘 다 종료 시 세그폴트 139, 산출물은 정상) → MSBuild `JGEngine.sln` DevelopEngine|x64 오류 0(경고 기준선 C4244 4건·LNK4098 1건) → `Document/Memory/tools/capture_devscene.ps1 -WaitSeconds 30` → ExitCode 0 → `Bin/DevelopEngine/jg_log.txt`에 `[error]`/`[critical]`/`D3D12 DebugLayer` 0건 → 캡처에 X Bot이 보임.

---

## P1. 확정 버그 수정 (현황분석 3-A)

- [ ] **1-1. `UpdateFrameWidgets`가 `UpdateFrame()`을 부르도록 수정** — `Source/Runtime/GUI/GUIModule.cpp:133`
  `Widget->Update()` → `Widget->UpdateFrame()`. `JGWidget::UpdateFrame()`(`Widget.cpp:71`)은 현재 호출자 0.
  완료 조건: `OnUpdateFrame`이 위젯·컴포넌트에서 호출된다(1-3에서 실제 사용으로 확인).

- [ ] **1-2. 위젯 업데이트 플래그 기본값과 설정 API** — `Source/Runtime/GUI/Widget.h:49`, `Widget.h:17-24` — `결정 필요`
  `EWidgetFlags WidgetFlags`를 설정하는 함수가 없어 모든 위젯이 `None`이고 `UpdateWidgets`/`UpdateFrameWidgets`(`GUIModule.cpp:96-135`)가 항상 `continue`한다.
  권장안: 기본값을 `AllowUpdate | AllowUpdateFrame`(열린 위젯은 갱신)으로 두고, `protected` 설정 함수 `SetWidgetFlags/AddWidgetFlags/RemoveWidgetFlags`를 추가한다(옛 설계 `WWidget`과 같은 이름). `AllowAlways*`는 닫혀 있어도 갱신하고 싶은 위젯만 켠다.
  대안: 기본값 `None` 유지 + 설정 함수만 추가(위젯마다 `OnInitialize`에서 켜야 함. 잊으면 지금처럼 조용히 갱신이 안 됨).
  `UpdateWidgets`(30Hz)와 `UpdateFrameWidgets`(매 프레임)의 판정 로직이 복붙이므로 `shouldUpdate(Widget, AlwaysFlag, OpenFlag)` 하나로 합친다.
  완료 조건: 플래그 설정 없이 만든 위젯이 열려 있으면 `OnUpdate`(30Hz)와 `OnUpdateFrame`(매 프레임)을 받는다.

- [ ] **1-3. `JGDevScene` 렌더를 `OnGenerateGUI`에서 `OnUpdateFrame`으로 이동** — `Source/Runtime/Devkit/DevScene.cpp:182-208`, `DevScene.h`
  `RenderScene()` 호출과 리드백 카운터(`FramesSinceMeshLoaded`, `DumpReadbackOnce`, 194-199행)를 새 `OnUpdateFrame()` 오버라이드로 옮긴다. `OnGenerateGUI`에는 `HGUI::Image`와 상태 텍스트만 남긴다.
  순서 근거: `Update` 버킷은 `GraphicsBegin`(GUI 생성) 뒤, `GraphicsEnd`(EndFrame) 앞이므로 같은 프레임 안이고, 드로우는 Default 리스트(1000), ImGui는 Present 리스트(MAX)라 GPU에서 씬 드로우가 이미지 표시보다 먼저 실행된다. 182행 위 주석("GenerateGUI는 GraphicsBegin 뒤에…")도 새 위치에 맞게 고친다.
  완료 조건: 캡처에 X Bot이 이전과 같이 보이고, 리드백 PNG 2장이 이전과 같이 생성된다(`ReadPixelsImmediate called inside a frame` 경고는 그대로 1건).

- [ ] **1-4. 메뉴 활성 판정에 `CanAction` 사용** — `Source/Runtime/GUI/Menu/MenuTree.cpp:78-80`
  `bEnable`이 `Visibility`를 다시 평가한다. `InNode.Item.CanAction.IsBound()`이면 `CanAction.Execute()`로 바꾼다.
  완료 조건: `CanAction`에 `false`를 돌려주는 람다를 바인딩한 임시 메뉴가 회색으로 비활성 표시된다(확인 후 임시 메뉴는 제거).

- [ ] **1-5. 메뉴 트리 키를 전체 경로로** — `Source/Runtime/GUI/Menu/MenuTree.cpp:21, 39`, `MenuTree.h:51`
  `MenuIndexesByName`이 토큰 이름만 키로 써서 다른 부모 아래 같은 이름이 한 노드로 합쳐진다. 토큰을 누적한 경로(`Windows`, `Windows/Statistics`, `Windows/Statistics/Memory`)를 키로 쓴다.
  같은 경로를 두 번 등록하면 경고 로그 후 무시, Action이 있는 리프에 자식이 붙으면(리프 판정이 `ChildIndexes.empty()`라 Action이 사라짐) 경고 로그를 남긴다.
  완료 조건: `Windows/Open`과 `File/Open`을 동시에 등록해도 각각 나온다(확인 후 임시 메뉴 제거). 기존 3개 메뉴 동작 유지.

- [ ] **1-6. DevConsole 모듈의 GUI 모듈 검사 반전 수정** — `Source/Editor/DevConsole/DevConsoleModule.cpp:65-75`
  `JG_CHECK(GUIModule == nullptr)` → `!= nullptr`. `static` 캐시 포인터를 없애고 매번 `FindModule<HGUIModule>()`한다(모듈 재연결 시 댕글링 방지). `Register/UnRegisterConsoleCommand`는 아직 호출자가 없으니 동작 확인은 임시 호출로.
  완료 조건: `RegisterConsoleCommand`를 임시로 한 번 호출해도 assert가 나지 않는다.

- [ ] **1-7. `PDX12GUIBackend::Shutdown`에서 기반 `Shutdown` 호출** — `Source/Runtime/GUI/Backends/DX12GUIBackend.cpp:135`
  ImGui 컨텍스트 파괴 뒤 `PGUIBackend::Shutdown()`을 호출해 `OnGUI`/`OnMainMenuGUI`의 생포인터 델리게이트를 지운다.
  완료 조건: 코드 확인. 종료 코드 0 유지.

- [ ] **1-8. `JGDevFeature::OnLayout` 복붙 조건 수정** — `Source/Runtime/Devkit/DevFeature.cpp:52`
  두 번째 분기 `if (DevScene.IsValid())` → `if (DevSettings.IsValid())`.

- [ ] **1-9. 로그 카테고리·메시지 복붙 정리**
  `GUIModule.cpp:21` 카테고리 `JGDev_GraphicsModule` → `GUI`.
  `DevConsoleModule.cpp:23` 카테고리 `DevStatistics` → `DevConsole`, 메시지 "DevConsoleModule Need GUI Module".
  `JGDev_Graphics.cpp:43` "Fail Connect Graphics Module..." → "GUI", `:54` "Asset" → "DevStatistics", `:66` "Asset" → "Devkit", `:78` 카테고리 `DevStatistics` → `JGDev_GraphicsModule`, 메시지 "JGDev_GraphicsModule Need GUI Module".
  `JG_LOG`의 카테고리는 토큰을 문자열로 바꿔 쓰므로(`Log.h:56`) 선언 없이 새 이름을 쓸 수 있다.
  완료 조건: 실행 로그에서 GUI 관련 줄이 `[GUI]`, `[DevConsole]` 카테고리로 찍힌다.

- [ ] **1-10. P1 검증** — 공통 검증 루프. 추가로: 캡처에서 메뉴바(`Dev`, `Windows`)와 DevFeature 창이 그대로 보인다. 임시 확인 코드(1-4, 1-5, 1-6)는 모두 제거했는지 `git diff`로 확인.

- [ ] **1-11. 커밋** — "GUI 위젯 업데이트 경로 복구, 메뉴 트리 버그 수정" (사용자가 직접 커밋). 포함 파일: GUI/GUIModule.cpp, Widget.h/.cpp, Menu/MenuTree.h/.cpp, Backends/DX12GUIBackend.cpp, Devkit/DevScene.h/.cpp, DevFeature.cpp, DevConsole/DevConsoleModule.cpp, JGDev_Graphics/JGDev_Graphics.cpp.

---

## P2. 수명·동기화 정리 (현황분석 3-B)

- [ ] **2-1. 스케줄러에 `Unschedule(uint64 id)` 추가와 태스크 정리** — `Source/Runtime/Core/Thread/Scheduler.h:63-65`, `Scheduler.cpp:44-113, 115-118`
  `_syncTaskPool`(소유, `PSharedPtr`)에서 ID로 erase하면 `_sortedSyncTasks`/`_reservedSyncTasks`의 약참조가 다음 `Update`에서 `Pin()` 실패로 제거된다. 순회 중 호출될 수 있으니 `_bIsTaskRunning`이면 삭제 예약 목록에 넣고 순회 끝에 처리한다.
  같이 고칠 것: (a) `updateTask`가 false를 돌려 목록에서 빠진 태스크가 `_syncTaskPool`에는 영원히 남는다(반복 끝난 태스크 누수). 목록에서 뺄 때 풀에서도 erase. (b) `Destroy()`가 빈 함수(`// Flush`). 풀·정렬 목록·예약 큐를 비운다. (c) `CreateSP` 델리게이트는 대상이 죽어도 `IsBound()`가 true라 태스크가 남는다(`Task.cpp:25`, `Delegate.h:548`). 호출은 무해(`Delegate.h:264`)하지만 누수이므로 `PTask::IsValid`가 SP 대상 생존을 함께 보게 하거나, 그대로 두고 문서화한다.
  부수 발견(Core): `PSPDelegate::GetOwner()`(`Delegate.h:253`)의 조건이 반전되어 있다(유효하면 nullptr, 죽었으면 Pin). `IsBoundTo(pObject)`/객체 기준 Remove가 SP 바인딩에 대해 항상 틀린다. 한 줄 수정.
  완료 조건: `Schedule` 반환 ID로 `Unschedule`하면 그 태스크가 다시 실행되지 않는다(임시 테스트 태스크로 확인).

- [ ] **2-2. 모듈이 등록한 `CreateRaw` 태스크를 `ShutdownModule`에서 해제** — `GUIModule.cpp:30-31`, `Source/Runtime/Graphics/JGGraphics.cpp:46-47`, `Source/Editor/JGDev_Graphics/JGDev_Graphics.cpp:72`
  반환 ID를 멤버에 보관하고 `ShutdownModule`에서 `Unschedule`. 세 모듈 같은 패턴. 백엔드의 `NewFrame`(`CreateSP`, `DX12GUIBackend.cpp:82`)은 백엔드 파괴로 무효화되므로 그대로 두되 2-1(c) 결과에 따라 해제 추가.
  완료 조건: 런처에서 `DisconnectModule("GUI")` → `ConnectModule("GUI")`를 임시로 한 번 수행해도 크래시 없이 GUI가 다시 뜬다(확인 후 임시 코드 제거). 이 테스트는 2-3, 2-4가 끝난 뒤 다시 돌린다.

- [ ] **2-3. `HGUIModule::ShutdownModule` 정리 순서** — `GUIModule.cpp:34-45`
  위젯 `Shutdown()` 루프 뒤 `Widgets.clear()`(GC 규칙: PSharedPtr 보유 시스템은 ShutdownModule에서 해제), `MainMenuTree` 노드·인덱스 clear, 그 뒤 백엔드 `Shutdown()`/Reset. 태스크 해제(2-2)는 맨 앞.
  완료 조건: 종료 코드 0, `DisconnectModule("GUI")` 직후 Flush에서 위젯 객체가 회수된다(메모리 로그 또는 소멸자 임시 로그로 확인 후 제거).

- [ ] **2-4. GUI 종료 전 GPU 완료 대기** — `Source/Runtime/Graphics/JGGraphicsAPI.h:49-52`, `DirectX12/DirectX12API.h/.cpp:90`, `DX12GUIBackend.cpp:137-140`
  `PJGGraphicsAPI`에 `virtual void WaitForGPUIdle() = 0`(이름은 Graphics 규칙에 맞게)를 추가하고 `PDirectX12API`가 `_commandQueue->Flush()`로 구현한다(`PCommandQueue`는 GRAPHICS_API 미노출이라 GUI에서 직접 못 부른다, `CommandQueue.h:18`). `PDX12GUIBackend::Shutdown` 첫 줄에서 호출한 뒤 SRV 힙·`ImGui_ImplDX12_Shutdown`을 진행한다(ImGui 공식 예제의 `WaitForLastSubmittedFrame()` 자리).
  완료 조건: 종료 시 `D3D12 DebugLayer` 메시지 0. `_DEBUG`에서 `Destroy` 전 InfoQueue 드레인이 돌아 확인 가능(`DirectX12API.cpp` EndFrame 드레인 참고).

- [ ] **2-5. SRV 링 상한 검사와 프레임별 세그먼트** — `DX12GUIBackend.h:17-20`, `DX12GUIBackend.cpp:96-98, 150-167, 205-222`
  (a) `ConvertImGuiTextureID`에서 `CurrentSrvIndex`가 세그먼트 끝에 닿으면 오류 로그 1회 후 슬롯 0(폰트)이나 마지막 유효 슬롯을 돌려준다. 힙 밖 `CopyDescriptorsSimple` 금지.
  (b) 힙 1024를 `BufferCount`(3, `JGGraphics.cpp:40`) 세그먼트로 나누고 `NewFrame`마다 세그먼트를 돌린다(백엔드 자체 프레임 카운터 `% BufferCount`). `OnUpdate` 끝의 인덱스 리셋(167행)은 제거한다(다음 프레임 `NewFrame`이 다른 세그먼트로 시작하므로 불필요).
  이 항목은 Graphics TODO 5-5(프레임 파이프라이닝) **전에** 끝나야 한다. 5-5 항목에 이 의존을 한 줄 적어 둔다.
  완료 조건: 임시로 `HGUI::Image`를 한 프레임에 400회 호출해도 크래시 없이 오류 로그만 남는다(세그먼트 341개 기준). 확인 후 임시 코드 제거.

- [ ] **2-6. `GenerateWidgetGUI` 순회 중 `OpenWidget` 안전화** — `GUIModule.cpp:47-56`
  `Widgets`의 값을 지역 `HList<PSharedPtr<JGWidget>>`로 복사한 뒤 순회한다(위젯 수가 적어 비용 무시). `UpdateWidgets`/`UpdateFrameWidgets`도 같은 방식.
  완료 조건: 위젯 `OnGenerateGUI` 안에서 임시 버튼으로 다른 위젯을 열어도 크래시 없음(확인 후 제거).

- [ ] **2-7. 창 리사이즈 책임을 Graphics 모듈로 이동** — `DX12GUIBackend.cpp:55, 179-195`, `DX12GUIBackend.h:33`, `Source/Runtime/Graphics/JGGraphics.cpp:22`(또는 `DirectX12API.cpp` Initialize)
  `WindowCallBacks->OnResize` 구독과 `FrameBuffer->Resize` 호출을 `HJGGraphicsModule`(또는 `PDirectX12API`)로 옮기고 GUI 백엔드의 `OnResize`를 삭제한다. `PDX12FrameBuffer::Resize`(`DX12FrameBuffer.cpp:106`)는 내부에서 큐 `Flush()`를 하므로 GPU 안전성은 그대로다. 전달받은 폭/높이를 무시하고 클라이언트 크기를 재조회하는 현재 동작은 유지해도 되나 이유를 주석으로 남긴다(WM_SIZE 인자와 클라이언트 크기 차이).
  `ShutdownModule`에서 구독 해제(`HDelegateHandle` 보관 → `Remove`). `GCoreSystem::Destroy`가 `WindowCallBacks`를 먼저 null로 만드므로(`CoreSystem.cpp:84`) 해제 시 null 검사.
  완료 조건: GUI 없이 Graphics만 연결한 상태(JGEditor 런치 또는 임시)에서 창 크기를 바꿔도 스왑체인이 따라간다. JGDev_Graphics에서는 기존과 동일. `capture_devscene.ps1`에 캡처 전 `MoveWindow`로 크기를 한 번 바꾸는 선택 단계를 추가해 회귀 확인.

- [ ] **2-8. 멀티 뷰포트 유지 여부 결정과 미사용 멤버 제거** — `DX12GUIBackend.cpp:41`, `DX12GUIBackend.h:14-15`, `.cpp:70-72, 137-139, 175` — `결정 필요`
  ImGui DX12 백엔드는 뷰포트마다 자체 큐·펜스·스왑체인을 만들어 엔진 큐 밖에서 제출한다(Graphics TODO 5-16 경고 출처). 권장: 지금은 **유지**(기능상 문제 없음, 경고는 무해로 기록됨)하고, 5-5 파이프라이닝이나 렌더 스레드 도입 시 재검토. 끄기로 결정하면 `ImGuiConfigFlags_ViewportsEnable` 한 줄과 `OnPresent`의 플랫폼 창 렌더를 제거.
  결정과 무관하게: `CommandAlloc`/`CommandList` 멤버는 `RenderPlatformWindowsDefault`의 두 번째 인자로만 넘겨지고 ImGui 구현이 무시하므로(`imgui_impl_dx12.cpp:1020`) 생성·해제 코드와 함께 삭제한다.
  완료 조건: 빌드 통과, 창을 메인 창 밖으로 끌어내는 동작이 결정한 대로다.

- [ ] **2-9. `HGUI::InputText` 버퍼 길이 고정 제거** — `Source/Runtime/GUI/GUI.cpp:59-71`
  `char Buf[512]` 대신 `std::vector<char>`(엔진 컨테이너는 2MB 한도·정적 금지 규칙이 있으니 지역 std::vector)를 `max(OutStr.Length() + 256, 512)`로 잡고 `memcpy_s` 길이를 `size-1`로 제한, 마지막 바이트 널 보장. 또는 `ImGuiInputTextFlags_CallbackResize`로 가변 길이.
  완료 조건: 600자 문자열을 넣어도 내용이 유지된다.

- [ ] **2-10. P2 검증** — 공통 검증 루프 + 2-2의 Disconnect/Connect 임시 테스트 + 2-7의 리사이즈 확인 + 종료 시 D3D12 메시지 0. 임시 코드 제거를 `git diff`로 확인.

- [ ] **2-11. 커밋** — "GUI/스케줄러 수명 정리: Unschedule, 종료 순서, GPU 대기, SRV 링 세그먼트, 리사이즈 책임 이동" (사용자가 직접 커밋). Core(Scheduler, Delegate), Graphics(JGGraphicsAPI, DirectX12API, JGGraphics), GUI, JGDev_Graphics가 함께 바뀐다.

---

## P3. 정리 · 방향 결정 (현황분석 3-C)

- [ ] **3-1. 죽은 코드 제거** — `GUI.h:9`(`;;`), `GUI.h:23`·`GUI.cpp:97`(`PlotTest` 빈 함수), `Backends/GUIBackends.cpp`(include 한 줄, 파일 삭제 → JGBuildTool 재생성), `WidgetComponent.h:11-16`(`AutoSize` 구현하거나 제거), `DX12GUIBackend.cpp:35`(폰트 주석. `Source/Font` 없음. 폰트 도입은 3-8 백로그), `Widget.h:17`·`WidgetComponent.h:9`(`enum class GUI_API` 제거), `Widget.h:27`·`WidgetComponent.h:19`(`HWidgetLayout`/`HWidgetComponentLayout` 동일 구조 → 하나로 합칠지 결정).
  완료 조건: 빌드 통과, 동작 변화 없음.

- [ ] **3-2. 위젯 식별자 정리(`GetGUID`/`PushID`)** — `Widget.cpp:47-49`, `Widget.h:5-13`, `Source/Editor/DevConsole/DevConsole.cpp:29-32`, `DevStatistics/MemoryStatistics.cpp:103-106`
  `Begin` 앞의 `PushID`는 창 식별에 영향이 없다(ImGui 창 식별자는 제목). `JG_GENERATED_WIDGET_BODY`가 `GetGUID() override { return GetStaticGUID(); }`를 제공하게 하고, `Begin` 제목을 `"%s###%llu"`(표시 제목 + GUID 해시)로 만들어 같은 제목 위젯이 합쳐지지 않게 한다. DevConsole/MemoryStatistcs의 수동 오버라이드는 삭제. `imgui.ini` 창 항목 키가 바뀌므로 이전 레이아웃은 한 번 초기화된다.
  완료 조건: 제목이 같은 임시 위젯 두 개가 별도 창으로 뜬다(확인 후 제거). 기존 창 도킹 레이아웃 재저장.

- [ ] **3-3. `GUI.module.json`의 `Asset` 의존 제거** — `Source/Runtime/GUI/GUI.module.json:4`
  GUI 소스에 Asset include 없음. GUI가 포함하는 Graphics 헤더 7개에도 Asset include 없음. `ModuleDependencies`가 include 경로·링크 양쪽에 쓰이는지 `BuildTool.cpp:290`에서 확인 후 제거 → JGBuildTool 재생성 → 빌드.
  완료 조건: GUI 프로젝트 빌드·링크 통과.

- [ ] **3-4. 데모 소스 제외** — `Imgui/imconfig.h`, `Imgui/implot_demo.cpp`
  `imconfig.h`에 `#define IMGUI_DISABLE_DEMO_WINDOWS`(`imgui_demo.cpp:201`이 빈 스텁으로 컴파일됨). ImPlot은 비활성 매크로가 없으므로 `implot_demo.cpp` 삭제(`ImPlot::ShowDemoWindow` 선언만 남고 호출자 없어 링크 문제 없음) → JGBuildTool 재생성. `Bin/DevelopEngine/imgui.ini`의 옛 창 항목(`Test`, `ImPlot Demo`, `Debug##Default`)도 정리.
  완료 조건: 빌드 통과, `GUI.dll` 크기 감소(현재 8.4MB) 기록.

- [ ] **3-5. 저장소의 백업 zip 2개와 `imgui.ini` 추적 정리** — `Source/Runtime/GUI_백업.zip`(871KB), `Source/Editor/JGEditor/백업.zip`, `Bin/DevelopEngine/imgui.ini` — `결정 필요`
  zip은 init 커밋부터 추적됨. 옛 설계 참고가 필요하면 압축을 풀어 `Document/Reference/GUI_2024/`로 옮기고 `git rm`, 아니면 이력에만 남기고 삭제. `imgui.ini`는 실행마다 바뀌는 사용자 레이아웃이라 추적 해제(`.gitignore`) 권장. `Bin/` 전체 추적 정책은 별도 논의.
  완료 조건: `git status`에 실행만으로 생기는 변경이 줄어든다.

- [ ] **3-6. JGEditor 모듈 정리** — `Source/Editor/JGEditor/JGEditor.h:7, 22`, `JGEditor.cpp:5, 12, 36, 44, 58-68`, `JGEditor.module.json` — `결정 필요`
  옛 설계 잔재(`GGUIGlobalSystem`, `HMenuBuilder`, `BuildMainMenu`)만 있는 런치 모듈. (a) 살리려면 GUI를 실제로 연결하고 `HGUIModule::AddMainMenuItem` 방식으로 메뉴를 옮긴다. (b) 지우려면 모듈 폴더와 json 제거 후 JGBuildTool 재생성. 어느 쪽이든 2026-09-17 모듈 수명 기록의 "모듈 RefCount 미적용"(JGEditor와 JGDev_Graphics가 함께 뜨면 Graphics 이중 Disconnect) 항목과 함께 처리한다.
  완료 조건: 결정에 따라 빌드 통과.

- [ ] **3-7. `HGUI` 래퍼 확장 범위 결정** — `Source/Runtime/GUI/GUI.h` — `결정 필요`
  현재 9개(`SameLine, NextLine, Text×2, InputText, PlotBarGroups, Selectable, Image`). 옛 컴포넌트 9종(`WBorder, WButton, WComboBox, WInputScalar, WInputText, WList, WSelectable, WText`)을 (a) `HGUI` 정적 함수로 되살릴지, (b) `JGWidgetComponent` 파생으로 만들지, (c) 둘을 섞을지 정한다. 권장: 즉시 모드 함수는 `HGUI`(Button, Checkbox, Combo, InputScalar, Separator, TreeNode, BeginTable/EndTable, BeginMenu 계열), 상태를 가진 것만 컴포넌트. `ESelectableFlags`처럼 ImGui 플래그를 엔진 열거형으로 감싸는 규칙(`GUI.cpp:16-37` 매크로)을 유지한다.
  완료 조건: 결정 내용을 `GUI.h` 상단 주석과 이 문서에 기록. 실제 함수 추가는 필요해질 때 항목을 따로 만든다.

- [ ] **3-8. ImGui 업그레이드 검토** — `Imgui/imgui.h:30`(1.91.1 WIP), `imgui_impl_dx12.h:37`(레거시 `ImGui_ImplDX12_Init`)
  최신 docking 브랜치는 `ImGui_ImplDX12_InitInfo` + `SrvDescriptorAllocFn/FreeFn` 콜백 방식이고, 1.92부터 텍스처 관리(`ImTextureData`)가 바뀐다. 업그레이드하면 2-5의 SRV 링을 콜백 기반 할당자로 바꿔야 한다. 지금은 **검토만**: 버전 차이와 필요한 변경을 `Document/`에 정리하고 시점은 Graphics 5-5 이후로 잡는다.
  완료 조건: 검토 메모 작성. 코드 변경 없음.

- [ ] **3-9. P3 검증** — 공통 검증 루프. 3-3, 3-4에서 프로젝트 재생성이 들어가므로 `Temp/ProjectFiles` 재생성 후 전체 빌드. `GUI.dll` 크기 전후 기록.

- [ ] **3-10. 커밋** — "GUI 정리: 죽은 코드, 데모 제외, 의존 정리, 백업 zip 처리" (사용자가 직접 커밋). 3-5, 3-6은 결정에 따라 별도 커밋.

---

## 백로그 (순서 미정 · 필요할 때 항목으로 승격)

- 컨텍스트(우클릭) 메뉴: `GUIModule.h:30` 주석 "Context 메뉴 추가". 옛 `HContextMenuBuilder` 참고.
- 스타일/테마: 옛 `GUIStyles/GenericGUIStyle`. 지금은 `StyleColorsDark` 고정(`DX12GUIBackend.cpp:36`).
- 폰트: TTF 로드 경로와 한글 글리프 범위. `Source/Font` 폴더 신설 여부.
- 위젯 상태 저장: `JGWidget`의 `bOpen`/`WidgetComponents`가 JGPROPERTY로 직렬화 코드가 생성되어 있으나 저장/복원 호출은 없음. 레이아웃 복원(imgui.ini)과 함께 설계.
- `OpenWidget<T>`가 소비자 DLL에서 `Widgets` 맵을 직접 조작하는 구조: 맵 타입을 바꾸면 GUI 사용 모듈 전부 재빌드. 비템플릿 `OpenWidget(JGClass)` 경로를 두는 것도 선택지.
- 위젯 GUI 생성이 `Update` 단계보다 앞선다(`GraphicsBegin` 버킷). 위젯 상태 변경이 한 프레임 늦게 보이는 것이 문제가 되면 GUI 생성을 `End`/`GraphicsEnd` 앞으로 옮기는 것을 검토(DevScene 드로우 기록 순서와 함께).
