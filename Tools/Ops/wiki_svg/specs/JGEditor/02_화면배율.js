// 1-2. 창과 화면 배율 — 시작: JGLauncher DPI 인식 → 창 크기 × 배율 → GUI 백엔드 글꼴 · 스타일 배율 / 모니터 이동: WM_DPICHANGED → 다음 프레임에서 다시 생성
// 코드(구조 개편 뒤): Programs/JGLauncher/Main.cpp(EnableDpiAwareness), Core/Platform/Platform.cpp(GetDpiScale),
//       Editor/JGEditor/JGEditor.cpp(StartupModule 창 크기), Runtime/GUI/Backends/DX12GUIBackend.cpp(ApplyDpiScale · OnWndProc WM_DPICHANGED · NewFrame)
module.exports = {
  file: 'JGEditor_02_화면배율',
  alt: '창과 화면 배율 — JGLauncher가 시작 시 프로세스를 모니터별 DPI 인식으로 설정하고, HJGEditorModule이 기본 창 크기에 배율을 곱해 창을 생성하며, GUI 백엔드는 같은 배율로 글꼴과 스타일 크기를 적용하고, 창이 배율이 다른 모니터로 이동하면 WM_DPICHANGED로 받은 배율로 다음 프레임에서 글꼴 아틀라스와 스타일을 다시 생성한다',
  type: 'flow',
  w: 860,
  h: 380,
  groups: [
    { x: 20, y: 20, w: 380, h: 340, label: '시작' },
    { x: 440, y: 20, w: 400, h: 340, label: '배율이 다른 모니터로 이동' }
  ],
  nodes: [
    { id: 'L', x: 40, y: 60, w: 340, h: 60, color: 'blue', title: 'JGLauncher', lines: ['프로세스 DPI 인식 — Per-Monitor V2'] },
    { id: 'E', x: 40, y: 170, w: 340, h: 60, color: 'blue', title: 'HJGEditorModule', lines: ['창 크기 = 기본 크기 × DPI 배율'] },
    { id: 'B', x: 40, y: 280, w: 340, h: 60, color: 'purple', title: 'PDX12GUIBackend', lines: ['ApplyDpiScale — 글꼴 · 스타일 크기'] },
    { id: 'W', x: 460, y: 60, w: 360, h: 60, color: 'gray', title: 'WM_DPICHANGED', lines: ['새 배율을 PendingDpiScale에 저장'] },
    { id: 'N', x: 460, y: 170, w: 360, h: 60, color: 'purple', title: '다음 NewFrame', lines: ['프레임 밖에서 배율 적용'] },
    { id: 'R', x: 460, y: 280, w: 360, h: 60, color: 'purple', title: 'ApplyDpiScale', lines: ['글꼴 아틀라스 · 스타일 다시 생성'] }
  ],
  edges: [
    { from: 'L', to: 'E', label: '실행 모듈 연결' },
    { from: 'E', to: 'B', label: 'GUI 초기화 — 창 배율 읽기' },
    { from: 'W', to: 'N' },
    { from: 'N', to: 'R' }
  ]
};
