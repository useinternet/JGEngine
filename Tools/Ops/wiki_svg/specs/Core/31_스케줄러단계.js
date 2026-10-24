// 0-2. GScheduleGlobalSystem (스케줄러) — 프레임 단계 순서와 단계마다 작업을 등록하는 모듈, 작업 스레드
// 코드: Core/Thread/MainThreadExecutionOrder.h, Core/Thread/Scheduler.h/.cpp, Graphics/JGGraphics.cpp, GUI/Backends/DX12GUIBackend.cpp,
//       GUI/GUIModule.cpp, GameFrameWorks/Core/GameFrameWorksModule.cpp, GameHost/GameHostModule.cpp, Core/Platform/Windows/WindowsJWindow.cpp
module.exports = {
  file: 'Core_31_스케줄러단계',
  alt: 'GScheduleGlobalSystem (스케줄러) — GScheduleGlobalSystem은 GraphicsBegin · Begin · Update · End · GraphicsEnd · UpdateWindow 단계 순서로 메인 스레드 작업을 실행하고, Graphics · GUI · GameFrameWorks · GameHost · 창이 각 단계에 작업을 등록하며, ScheduleAsync는 작업을 작업 스레드(PThread)에 넣는다',
  type: 'class',
  w: 900,
  h: 600,
  groups: [
    { x: 300, y: 20, w: 280, h: 440, label: 'EMainThreadExecutionOrder', color: 'blue' }
  ],
  classes: [
    { id: 'GS', x: 15, y: 60, w: 250, name: 'GScheduleGlobalSystem', stereo: '스케줄러', color: 'green', members: ['+ScheduleByFrame() · Schedule()', '+ScheduleAsync()'] },
    { id: 'PT', x: 330, y: 500, w: 220, name: 'PThread', stereo: '작업 스레드 · ENamedThread', color: 'yellow', stack: 2 }
  ],
  nodes: [
    { id: 'GB', x: 330, y: 60, w: 220, h: 44, color: 'blue', title: 'GraphicsBegin' },
    { id: 'BG', x: 330, y: 124, w: 220, h: 44, color: 'blue', title: 'Begin' },
    { id: 'UP', x: 330, y: 188, w: 220, h: 44, color: 'blue', title: 'Update' },
    { id: 'EN', x: 330, y: 252, w: 220, h: 44, color: 'blue', title: 'End' },
    { id: 'GE', x: 330, y: 316, w: 220, h: 44, color: 'blue', title: 'GraphicsEnd' },
    { id: 'UW', x: 330, y: 380, w: 220, h: 44, color: 'blue', title: 'UpdateWindow' },
    { id: 'M1', x: 650, y: 57, w: 230, h: 50, color: 'purple', title: 'Graphics · GUI 백엔드', lines: ['BeginFrame · NewFrame'] },
    { id: 'M3', x: 650, y: 185, w: 230, h: 50, color: 'purple', title: 'GUI · GameFrameWorks', lines: ['UpdateWidgets · tick'] },
    { id: 'M4', x: 650, y: 249, w: 230, h: 50, color: 'purple', title: 'GameHost', lines: ['renderFrame'] },
    { id: 'M5', x: 650, y: 313, w: 230, h: 50, color: 'purple', title: 'Graphics', lines: ['EndFrame'] },
    { id: 'M6', x: 650, y: 377, w: 230, h: 50, color: 'purple', title: 'PWindowsJWindow', lines: ['창 메시지 처리'] }
  ],
  edges: [
    { from: 'GB', to: 'BG' },
    { from: 'BG', to: 'UP' },
    { from: 'UP', to: 'EN' },
    { from: 'EN', to: 'GE' },
    { from: 'GE', to: 'UW' },
    { from: 'M1', to: 'GB', fs: 'l', ts: 'r', dash: true, label: '등록' },
    { from: 'M3', to: 'UP', fs: 'l', ts: 'r', dash: true },
    { from: 'M4', to: 'EN', fs: 'l', ts: 'r', dash: true },
    { from: 'M5', to: 'GE', fs: 'l', ts: 'r', dash: true },
    { from: 'M6', to: 'UW', fs: 'l', ts: 'r', dash: true },
    { points: [[265, 112], [300, 112]] },
    { from: 'GS', to: 'PT', fs: 'b', ts: 'l', label: 'ScheduleAsync' }
  ]
};
