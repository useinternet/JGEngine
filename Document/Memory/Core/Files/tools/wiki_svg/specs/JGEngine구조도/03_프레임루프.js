// 3. 프레임 루프 한 바퀴 — GCoreSystem::Update() 의 전역 시스템 순서(왼쪽)와 Schedule이 도는 프레임 단계(오른쪽)
// Schedule → GraphicsBegin 은 오른쪽으로 나가고, 단계를 위에서 아래로 돈 뒤 UpdateWindow → ConsoleCommand 로 왼쪽으로 돌아온다(U 자).
module.exports = {
  file: 'JGEngine구조도_03_프레임루프',
  alt: '프레임 루프 한 바퀴 — GCoreSystem::Update가 Memory GC → Timer → Schedule 순으로 갱신하고, Schedule이 GraphicsBegin → Begin → Update → End → GraphicsEnd → UpdateWindow 프레임 단계를 실행한 뒤, ConsoleCommand가 Submit된 명령을 프레임 밖에서 실행한다',
  type: 'flow',
  w: 770,
  h: 666,
  groups: [
    { x: 20, y: 20, w: 320, h: 626, label: 'GCoreSystem::Update() — 등록 순서' },
    { x: 390, y: 168, w: 360, h: 478, label: '프레임 단계 — EMainThreadExecutionOrder' }
  ],
  nodes: [
    { id: 'M', x: 40, y: 56, w: 280, h: 50, color: 'green', title: 'GMemoryGlobalSystem', lines: ['GC — 지난 프레임에 놓인 객체 파괴'] },
    { id: 'T', x: 40, y: 130, w: 280, h: 50, color: 'green', title: 'GTimerGlobalSystem', lines: ['타이머 갱신'] },
    { id: 'SC', x: 40, y: 204, w: 280, h: 50, color: 'green', title: 'GScheduleGlobalSystem', lines: ['프레임 단계 작업 실행'] },
    { id: 'CC', x: 40, y: 580, w: 280, h: 50, color: 'green', title: 'GConsoleCommandGlobalSystem', lines: ['Submit된 명령 실행 — 프레임 밖'] },
    { id: 'GB', x: 410, y: 204, w: 320, h: 50, color: 'blue', title: 'GraphicsBegin', lines: ['Graphics BeginFrame · ImGui NewFrame'] },
    { id: 'B', x: 410, y: 278, w: 320, h: 40, color: 'blue', title: 'Begin' },
    { id: 'U', x: 410, y: 342, w: 320, h: 66, color: 'blue', title: 'Update', lines: ['GFW tick (세션 → 월드 → 게임 UI)', '위젯 OnUpdate'] },
    { id: 'E', x: 410, y: 432, w: 320, h: 50, color: 'blue', title: 'End', lines: ['GameHost renderFrame'] },
    { id: 'GE', x: 410, y: 506, w: 320, h: 50, color: 'blue', title: 'GraphicsEnd', lines: ['업로드 → 백 버퍼 → 제출 → Present'] },
    { id: 'W', x: 410, y: 580, w: 320, h: 50, color: 'blue', title: 'UpdateWindow', lines: ['메시지 펌프'] }
  ],
  edges: [
    { from: 'M', to: 'T' },
    { from: 'T', to: 'SC' },
    { from: 'SC', to: 'GB', fs: 'r', ts: 'l' },
    { from: 'GB', to: 'B' },
    { from: 'B', to: 'U' },
    { from: 'U', to: 'E' },
    { from: 'E', to: 'GE' },
    { from: 'GE', to: 'W' },
    { from: 'W', to: 'CC', fs: 'l', ts: 'r' }
  ]
};
