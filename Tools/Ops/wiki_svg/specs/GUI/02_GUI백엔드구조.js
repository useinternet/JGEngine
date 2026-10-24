// 2-1. PGUIBackend과 PDX12GUIBackend — 추상 백엔드와 DX12 · Win32 구현, 구독 · 예약하는 엔진 이벤트, 사용하는 ImGui · DX12 API
// 코드: Runtime/GUI/Backends/GUIBackend.h · GUIBackend.cpp · DX12GUIBackend.h · DX12GUIBackend.cpp, Graphics/DirectX12/DX12FrameBuffer.cpp
module.exports = {
  file: 'GUI_02_GUI백엔드구조',
  alt: 'PGUIBackend과 PDX12GUIBackend — PGUIBackend는 OnGUI · OnMainMenuGUI 이벤트와 화면 배율을 가진 추상 클래스이고, PDX12GUIBackend는 ImGui · ImPlot 컨텍스트와 SRV 힙을 소유하며, GraphicsBegin 단계에 NewFrame을 예약하고 프레임 버퍼 OnUpdate · OnPresent 이벤트와 창 WndProc 이벤트를 구독하고, PDirectX12API로 D3D12 장치에 접근한다',
  type: 'class',
  w: 940,
  h: 540,
  classes: [
    { id: 'BASE', x: 40, y: 40, w: 300, name: 'PGUIBackend', stereo: 'GUI 백엔드 · 추상 클래스', color: 'yellow', members: ['+OnGUI · OnMainMenuGUI : 이벤트', '+Initialize() · Shutdown()', '+GPUAllocate(TextureID)', '+GetDpiScale()'] },
    { id: 'DX', x: 40, y: 230, w: 300, name: 'PDX12GUIBackend', stereo: 'DX12 · Win32 구현', color: 'yellow', members: ['-SrvDescriptorHeap · CommandList', '-BaseStyle · PendingDpiScale', '+NewFrame()', '+GPUAllocate(TextureID)', '-OnUpdate() · OnPresent()', '-OnWndProc() · ApplyDpiScale()'] }
  ],
  nodes: [
    { id: 'KEY', x: 480, y: 40, w: 420, h: 96, color: 'white', shape: 'note', title: '범례', lines: ['—▷ 상속 · → 사용 · 호출', '- - → 프레임 단계 예약 · 이벤트 구독', '+ public · - private'] },
    { id: 'SCH', x: 480, y: 232, w: 420, h: 48, color: 'gray', title: 'GScheduleGlobalSystem (Core)', lines: ['GraphicsBegin 단계 → NewFrame'] },
    { id: 'FB', x: 480, y: 292, w: 420, h: 48, color: 'blue', title: 'PDX12FrameBuffer (Graphics)', lines: ['OnUpdate → ImGui 그리기 · OnPresent → 별도 OS 창'] },
    { id: 'WND', x: 480, y: 352, w: 420, h: 48, color: 'gray', title: 'WndProc 이벤트 (Core 창)', lines: ['ImGui 입력 · WM_DPICHANGED'] },
    { id: 'IMG', x: 40, y: 450, w: 300, h: 60, color: 'gray', title: 'Dear ImGui · ImPlot', lines: ['컨텍스트 · 글꼴 아틀라스 · 스타일'] },
    { id: 'API', x: 480, y: 450, w: 420, h: 60, color: 'blue', title: 'PDirectX12API (Graphics)', lines: ['D3D12 장치 · 프레임 인덱스 · GPU 대기'] }
  ],
  edges: [
    { points: [[190, 230], [190, 166]], end: 'triangle' },
    { points: [[340, 256], [480, 256]], dash: true, label: '예약' },
    { points: [[340, 316], [480, 316]], dash: true, label: '구독' },
    { points: [[340, 376], [480, 376]], dash: true, label: '구독' },
    { points: [[190, 392], [190, 450]], label: '생성 · 호출' },
    { points: [[300, 392], [300, 425], [690, 425], [690, 450]], label: '사용', lx: 495, ly: 425 }
  ]
};
