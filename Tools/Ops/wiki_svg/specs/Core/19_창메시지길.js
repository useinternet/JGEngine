// 9-2. 창 메시지 흐름
// Mermaid 의 첫 상자 "Windows 메시지 → Core 창 함수 WndProc" 는 글 속 화살표 그대로 두 상자(Windows 메시지 → Core 창 프로시저 WndProc)로 나눠 그렸다.
module.exports = {
  file: 'Core_19_창메시지길',
  alt: '윈도우 메시지 처리 — Windows 메시지는 Core 창 프로시저 WndProc을 거쳐 WndProc 이벤트(GUI · Audio), OnResize 이벤트(Graphics · GameHost), 입력 변환기 → OnInput 이벤트(GameHost, 구독자가 있을 때만), WM_QUIT(프레임 루프 종료)로 분기된다',
  type: 'flow',
  w: 920,
  h: 540,
  nodes: [
    { id: 'WIN', x: 30, y: 230, w: 120, h: 60, color: 'gray', title: 'Windows 메시지' },
    { id: 'WP', x: 190, y: 230, w: 160, h: 60, color: 'blue', title: 'WndProc', lines: ['Core 창 프로시저'] },
    { id: 'E1', x: 410, y: 60, w: 200, h: 60, color: 'green', title: 'WndProc 이벤트', lines: ['모든 메시지'] },
    { id: 'E2', x: 410, y: 230, w: 200, h: 60, color: 'green', title: 'OnResize 이벤트', lines: ['WM_SIZE'] },
    { id: 'TR', x: 410, y: 360, w: 200, h: 60, color: 'green', title: '입력 변환기 → OnInput 이벤트', lines: ['구독자가 있을 때만 동작'] },
    { id: 'QUIT', x: 410, y: 450, w: 200, h: 60, color: 'gray', title: 'WM_QUIT', lines: ['프레임 루프 종료'] },
    { id: 'GUI', x: 670, y: 15, w: 220, h: 70, color: 'purple', title: 'GUI', lines: ['ImGui에 메시지 전달', '화면 배율 변경 기록'] },
    { id: 'AU', x: 670, y: 100, w: 220, h: 60, color: 'purple', title: 'Audio', lines: ['창 비활성 시 음량 감소 · 음소거'] },
    { id: 'GFX', x: 670, y: 190, w: 220, h: 60, color: 'purple', title: 'Graphics', lines: ['스왑체인 크기 변경'] },
    { id: 'HOST1', x: 670, y: 270, w: 220, h: 60, color: 'purple', title: 'GameHost', lines: ['뷰포트 크기 변경'] },
    { id: 'HOST2', x: 670, y: 360, w: 220, h: 60, color: 'purple', title: 'GameHost', lines: ['게임 입력 처리 (게임 UI 우선)'] }
  ],
  edges: [
    { from: 'WIN', to: 'WP', fs: 'r', ts: 'l' },
    { from: 'WP', to: 'E1', fs: 'r', ts: 'l', mx: 380 },
    { from: 'WP', to: 'E2', fs: 'r', ts: 'l', mx: 380 },
    { from: 'WP', to: 'TR', fs: 'r', ts: 'l', mx: 380 },
    { from: 'WP', to: 'QUIT', fs: 'r', ts: 'l', mx: 380 },
    { from: 'E1', to: 'GUI', fs: 'r', ts: 'l', mx: 640 },
    { from: 'E1', to: 'AU', fs: 'r', ts: 'l', mx: 640 },
    { from: 'E2', to: 'GFX', fs: 'r', ts: 'l', mx: 640 },
    { from: 'E2', to: 'HOST1', fs: 'r', ts: 'l', mx: 640 },
    { from: 'TR', to: 'HOST2', fs: 'r', ts: 'l' }
  ]
};
