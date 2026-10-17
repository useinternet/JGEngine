// 프레임마다 GUI가 그려지는 순서 — 묶음 = 메인 스레드 실행 단계.
// 윗줄 GraphicsBegin: NewFrame → ImGui::NewFrame → (메인 메뉴 · 위젯 GUI 두 갈래), 아랫줄 Update → GraphicsEnd 로 이어진다.
// Mermaid 의 "Present:" 상자는 프레임버퍼 OnPresent 이벤트(PDX12GUIBackend::OnPresent)라 GraphicsEnd 묶음 안에 두었다.
module.exports = {
  file: 'GUI_00_프레임순서',
  alt: '프레임마다 GUI가 그려지는 순서 — GraphicsBegin에서 PDX12GUIBackend::NewFrame · ImGui::NewFrame 뒤 메인 메뉴 막대와 열린 위젯 GUI를 생성하고, Update에서 열린 위젯 OnUpdate를 호출하며, GraphicsEnd에서 ImGui::Render로 DX12 그리기 후 Present 이벤트에서 별도 OS 창을 그린다',
  type: 'flow',
  w: 960,
  h: 386,
  groups: [
    { x: 20, y: 20, w: 920, h: 206, label: 'GraphicsBegin 단계' },
    { x: 20, y: 246, w: 300, h: 120, label: 'Update 단계' },
    { x: 340, y: 246, w: 600, h: 120, label: 'GraphicsEnd 단계' }
  ],
  nodes: [
    { id: 'A', x: 40, y: 101, w: 230, h: 56, color: 'purple', title: 'PDX12GUIBackend::NewFrame', lines: ['GUI 백엔드 프레임 시작'] },
    { id: 'B', x: 320, y: 101, w: 210, h: 56, color: 'gray', title: 'ImGui::NewFrame', lines: ['전체 화면 DockSpace'] },
    { id: 'C', x: 590, y: 56, w: 300, h: 56, color: 'green', title: 'OnMainMenuGUI', lines: ['메인 메뉴 막대'] },
    { id: 'D', x: 590, y: 146, w: 300, h: 56, color: 'green', title: 'OnGUI', lines: ['열린 위젯 GenerateGUI → OnGenerateGUI'] },
    { id: 'E', x: 40, y: 282, w: 260, h: 66, color: 'blue', title: 'HGUIModule::UpdateWidgets', lines: ['열린 위젯 OnUpdate 호출'] },
    { id: 'F', x: 370, y: 282, w: 250, h: 66, color: 'gray', title: 'ImGui::Render', lines: ['프레임버퍼 OnUpdate 이벤트에서 호출', 'DX12 그리기'] },
    { id: 'G', x: 670, y: 282, w: 250, h: 66, color: 'purple', title: 'PDX12GUIBackend::OnPresent', lines: ['Present 이벤트에서 호출', '별도 OS 창(멀티 뷰포트) 그리기'] }
  ],
  edges: [
    { from: 'A', to: 'B', fs: 'r', ts: 'l' },
    { from: 'B', to: 'C', fs: 'r', ts: 'l', mx: 560 },
    { from: 'B', to: 'D', fs: 'r', ts: 'l', mx: 560 },
    { from: 'D', to: 'E', fs: 'b', ts: 't', my: 236 },
    { from: 'E', to: 'F', fs: 'r', ts: 'l' },
    { from: 'F', to: 'G', fs: 'r', ts: 'l' }
  ]
};
