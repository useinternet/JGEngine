// 2-2. 프레임 처리 순서 — 묶음 = 메인 스레드 실행 단계. GraphicsBegin: NewFrame → ImGui::NewFrame → 메인 메뉴 · 위젯 GUI,
// Update: 열린 위젯 OnUpdate, GraphicsEnd: 프레임 버퍼 OnUpdate 이벤트에서 ImGui::Render · DX12 그리기, OnPresent 이벤트에서 별도 OS 창.
// 코드: Runtime/GUI/Backends/DX12GUIBackend.cpp(NewFrame · OnUpdate · OnPresent), GUIModule.cpp(GenerateMainMenuGUI · GenerateWidgetGUI · UpdateWidgets),
//       Graphics/DirectX12/DX12FrameBuffer.cpp(Update · Present), Core/Thread/MainThreadExecutionOrder.h
module.exports = {
  file: 'GUI_03_프레임순서',
  alt: '프레임 처리 순서 — GraphicsBegin 단계에서 PDX12GUIBackend::NewFrame이 ImGui 프레임과 전체 화면 도킹 공간을 시작한 뒤 OnMainMenuGUI로 메인 메뉴 막대를, OnGUI로 열린 위젯의 GUI를 생성하고, Update 단계에서 열린 위젯의 OnUpdate를 호출하며, GraphicsEnd 단계에서 프레임 버퍼 OnUpdate 이벤트로 ImGui::Render와 DX12 그리기를, OnPresent 이벤트로 별도 OS 창을 그린다',
  type: 'flow',
  w: 960,
  h: 396,
  groups: [
    { x: 20, y: 20, w: 920, h: 216, label: 'GraphicsBegin 단계' },
    { x: 20, y: 256, w: 300, h: 120, label: 'Update 단계' },
    { x: 340, y: 256, w: 600, h: 120, label: 'GraphicsEnd 단계' }
  ],
  nodes: [
    { id: 'A', x: 40, y: 106, w: 230, h: 56, color: 'yellow', title: 'PDX12GUIBackend::NewFrame', lines: ['GUI 백엔드 프레임 시작'] },
    { id: 'B', x: 320, y: 106, w: 210, h: 56, color: 'gray', title: 'ImGui::NewFrame', lines: ['전체 화면 도킹 공간'] },
    { id: 'C', x: 590, y: 56, w: 320, h: 66, color: 'green', title: 'OnMainMenuGUI 이벤트', lines: ['HMainMenuTree — 메인 메뉴 막대'] },
    { id: 'D', x: 590, y: 142, w: 320, h: 76, color: 'blue', title: 'OnGUI 이벤트', lines: ['HGUIModule — 첫 프레임 열린 창 복원', '열린 위젯 GenerateGUI'] },
    { id: 'E', x: 40, y: 292, w: 260, h: 66, color: 'blue', title: 'HGUIModule::UpdateWidgets', lines: ['열린 위젯 OnUpdate 호출'] },
    { id: 'F', x: 370, y: 292, w: 250, h: 66, color: 'gray', title: 'ImGui::Render', lines: ['프레임 버퍼 OnUpdate 이벤트', 'DX12 그리기'] },
    { id: 'G', x: 670, y: 292, w: 250, h: 66, color: 'yellow', title: 'PDX12GUIBackend::OnPresent', lines: ['프레임 버퍼 OnPresent 이벤트', '별도 OS 창(멀티 뷰포트) 그리기'] }
  ],
  edges: [
    { from: 'A', to: 'B', fs: 'r', ts: 'l' },
    { points: [[530, 134], [560, 134], [560, 89], [590, 89]] },
    { points: [[560, 134], [560, 180], [590, 180]] },
    { points: [[750, 218], [750, 246], [170, 246], [170, 292]] },
    { from: 'E', to: 'F', fs: 'r', ts: 'l' },
    { from: 'F', to: 'G', fs: 'r', ts: 'l' }
  ]
};
