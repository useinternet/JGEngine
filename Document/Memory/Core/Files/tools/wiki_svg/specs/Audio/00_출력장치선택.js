// 출력 장치 고르기 — 가운데 줄 = 실제 장치를 여는 경로, 오른쪽 = 무음으로 가는 갈래(Core_06 과 같은 짜임)
module.exports = {
  file: 'Audio_00_출력장치선택',
  alt: '출력 장치 고르기 — HAudioModule::StartupModule이 GAudioGlobalSystem을 등록 · 초기화하고, 창이 없거나(JGConsole) -nosound면 무음 장치 NullDevice를, 아니면 기본 장치(WASAPI → DirectSound → WinMM)를 열어 실제 장치가 열리면 출력하며 WM_ACTIVATEAPP을 구독하고, 열리지 않으면 Warning 로그와 함께 무음으로 동작한다',
  type: 'flow',
  w: 680,
  h: 670,
  nodes: [
    { id: 'A', x: 60, y: 20, w: 300, h: 44, color: 'purple', title: 'HAudioModule::StartupModule' },
    { id: 'B', x: 60, y: 100, w: 300, h: 56, color: 'green', title: 'GAudioGlobalSystem', lines: ['등록 → Initialize'] },
    { id: 'C', x: 90, y: 190, w: 240, h: 100, color: 'white', shape: 'diamond', title: '창 없음(JGConsole)', lines: ['또는 -nosound인가'] },
    { id: 'N', x: 400, y: 212, w: 240, h: 56, color: 'gray', title: 'NullDevice', lines: ['무음 장치 · Info 로그'] },
    { id: 'D', x: 60, y: 340, w: 300, h: 70, color: 'blue', title: 'Device', lines: ['기본 장치', 'WASAPI → DirectSound → WinMM'] },
    { id: 'E', x: 100, y: 450, w: 220, h: 90, color: 'white', shape: 'diamond', title: '실제 장치가 열렸는가' },
    { id: 'W', x: 400, y: 467, w: 240, h: 56, color: 'yellow', title: '무음으로 동작', lines: ['Warning 로그'] },
    { id: 'O', x: 60, y: 590, w: 300, h: 56, color: 'green', title: '실제 출력', lines: ['WM_ACTIVATEAPP 구독'] }
  ],
  edges: [
    { from: 'A', to: 'B' },
    { from: 'B', to: 'C' },
    { from: 'C', to: 'N', fs: 'r', ts: 'l', label: '예' },
    { from: 'C', to: 'D', fs: 'b', ts: 't', label: '아니오' },
    { from: 'D', to: 'E' },
    { from: 'E', to: 'W', fs: 'r', ts: 'l', label: '아니오' },
    { from: 'E', to: 'O', fs: 'b', ts: 't', label: '예' }
  ]
};
