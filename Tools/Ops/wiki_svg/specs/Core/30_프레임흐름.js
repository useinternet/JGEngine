// 0-1. 시작, 프레임, 종료 — Update 는 전역 시스템을 등록 순서대로 갱신하고, 루프가 끝나면 Destroy 가 역순으로 내린다
// 코드: Core/CoreSystem.cpp(Create · Update · Destroy · RequestExit), Core/Platform/Windows/WindowsJWindow.cpp(WM_QUIT)
module.exports = {
  file: 'Core_30_프레임흐름',
  alt: '시작, 프레임, 종료 — GCoreSystem::Create가 전역 시스템을 등록하고 실행 모듈을 연결한 뒤 Start를 호출하고, Update는 로그 · 메모리 · 타이머 · 스케줄 · 문자열 테이블 · 콘솔 명령 · 모듈 · 오브젝트 시스템과 모듈이 등록한 전역 시스템을 등록 순서대로 갱신하며, bIsRunning이 거짓이 되면 Destroy가 등록 역순으로 해제한다',
  type: 'flow',
  w: 820,
  h: 930,
  groups: [
    { x: 150, y: 110, w: 480, h: 600, label: 'GCoreSystem::Update', color: 'blue' }
  ],
  nodes: [
    { id: 'C', x: 240, y: 20, w: 300, h: 60, color: 'blue', title: 'GCoreSystem::Create', lines: ['전역 시스템 등록 · 실행 모듈 연결 · Start 호출'] },
    { id: 'S1', x: 190, y: 150, w: 400, h: 48, color: 'green', title: 'GLogGlobalSystem', lines: ['로그'] },
    { id: 'S2', x: 190, y: 212, w: 400, h: 48, color: 'green', title: 'GMemoryGlobalSystem', lines: ['GC'] },
    { id: 'S3', x: 190, y: 274, w: 400, h: 48, color: 'green', title: 'GTimerGlobalSystem', lines: ['타이머 갱신'] },
    { id: 'S4', x: 190, y: 336, w: 400, h: 48, color: 'yellow', title: 'GScheduleGlobalSystem', lines: ['프레임 단계 순서로 작업 실행'] },
    { id: 'S5', x: 190, y: 398, w: 400, h: 48, color: 'green', title: 'GStringTable', lines: ['쓰이지 않는 문자열 정리'] },
    { id: 'S6', x: 190, y: 460, w: 400, h: 48, color: 'green', title: 'GConsoleCommandGlobalSystem', lines: ['Submit 대기열 실행'] },
    { id: 'S7', x: 190, y: 522, w: 400, h: 48, color: 'green', title: 'GModuleGlobalSystem', lines: ['모듈 수명 관리'] },
    { id: 'S8', x: 190, y: 584, w: 400, h: 48, color: 'green', title: 'GObjectGlobalSystem', lines: ['리플렉션 정보 보관'] },
    { id: 'S9', x: 190, y: 646, w: 400, h: 48, color: 'purple', title: '모듈이 등록한 전역 시스템', lines: ['StartupModule에서 등록'] },
    { id: 'Q', x: 290, y: 740, w: 200, h: 70, color: 'white', shape: 'diamond', title: 'bIsRunning' },
    { id: 'N', x: 20, y: 745, w: 220, h: 60, color: 'white', shape: 'note', title: 'RequestExit · WM_QUIT', lines: ['bIsRunning = false'] },
    { id: 'D', x: 240, y: 850, w: 300, h: 60, color: 'blue', title: 'GCoreSystem::Destroy', lines: ['등록 역순으로 해제 · DLL 해제'] }
  ],
  edges: [
    { from: 'C', to: 'S1' },
    { from: 'S1', to: 'S2' },
    { from: 'S2', to: 'S3' },
    { from: 'S3', to: 'S4' },
    { from: 'S4', to: 'S5' },
    { from: 'S5', to: 'S6' },
    { from: 'S6', to: 'S7' },
    { from: 'S7', to: 'S8' },
    { from: 'S8', to: 'S9' },
    { from: 'S9', to: 'Q' },
    { from: 'Q', to: 'S1', fs: 'r', ts: 'r', points: [[490, 775], [670, 775], [670, 174], [590, 174]], label: '참 — 다음 프레임', lx: 670, ly: 470 },
    { from: 'N', to: 'Q', fs: 'r', ts: 'l', dash: true },
    { from: 'Q', to: 'D', label: '거짓' }
  ],
  texts: [
    { x: 616, y: 131, text: '등록 순서대로 갱신', size: 11, anchor: 'end' }
  ]
};
