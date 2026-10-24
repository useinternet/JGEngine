// 3. 실행 흐름 — 시작부터 종료까지
// 세 단계를 왼쪽에서 오른쪽 열로 두고(실행 모듈 결정 → Core 초기화 → 프레임 루프 · 종료), 각 열은 위에서 아래로 읽는다.
// 열 사이 이동은 열 사이 통로(x=463, x=739)로 올라가 다음 열 첫 상자로 들어간다.
module.exports = {
  file: 'JGEngine구조도_02_실행흐름',
  alt: '실행 흐름 — JGLauncher main이 DPI 인식을 활성화하고 -game 인자에 따라 실행 모듈을 JGEditor 또는 GameHost로 정한 뒤, GCoreSystem::Create가 베이스 시스템 등록 · 루트 결정 · codeGen · 실행 모듈 연결 · 시스템 Start를 차례로 수행하고, 프레임 루프가 WM_QUIT로 끝나면 GCoreSystem::Destroy가 역순으로 종료한다',
  type: 'flow',
  w: 1000,
  h: 622,
  groups: [
    { x: 15, y: 20, w: 435, h: 390, label: 'JGLauncher — 실행 모듈 결정' },
    { x: 476, y: 20, w: 250, h: 582, label: 'Core 초기화' },
    { x: 752, y: 20, w: 233, h: 368, label: '프레임 루프 · 종료' }
  ],
  nodes: [
    // 실행 모듈 결정
    { id: 'S', x: 100, y: 56, w: 260, h: 48, color: 'gray', title: 'JGLauncher.exe main()', lines: ['프로그램 진입점'] },
    { id: 'D', x: 105, y: 128, w: 250, h: 48, color: 'gray', title: 'HPlatform::EnableDpiAwareness()', lines: ['모니터별 DPI 인식 활성화'] },
    { id: 'A', x: 150, y: 200, w: 160, h: 80, color: 'white', shape: 'diamond', title: '-game 인자가 있는가' },
    { id: 'L1', x: 25, y: 320, w: 190, h: 50, color: 'purple', title: 'LaunchModule = JGEditor', lines: ['에디터 실행'] },
    { id: 'L2', x: 245, y: 320, w: 190, h: 50, color: 'blue', title: 'LaunchModule = GameHost', lines: ['런타임 모듈만 로드'] },
    // Core 초기화
    { id: 'C', x: 490, y: 56, w: 222, h: 50, color: 'green', title: 'GCoreSystem::Create(args)', lines: ['HCoreSystemArguments 전달'] },
    { id: 'C1', x: 490, y: 130, w: 222, h: 82, color: 'green', title: '베이스 시스템 8개 등록', lines: ['Log · Memory · Timer · Schedule', 'StringTable · ConsoleCommand', 'Module · Object'] },
    { id: 'C2', x: 490, y: 236, w: 222, h: 64, color: 'green', title: '루트 결정', lines: ['../../에 .jgproject가 있으면 프로젝트 모드'] },
    { id: 'C3', x: 490, y: 324, w: 222, h: 50, color: 'green', title: 'codeGen', lines: ['목록의 DLL마다 리플렉션 등록'] },
    { id: 'C4', x: 490, y: 398, w: 222, h: 50, color: 'purple', title: 'ConnectModule(LaunchModule)', lines: ['실행 모듈의 StartupModule 호출'] },
    { id: 'C5', x: 490, y: 472, w: 222, h: 50, color: 'purple', title: '엔진 모듈 · 게임 모듈 연결', lines: ['실행 모듈이 StartupModule 안에서'] },
    { id: 'C6', x: 490, y: 546, w: 222, h: 40, color: 'green', title: '모든 시스템 Start()' },
    // 프레임 루프 · 종료
    { id: 'F', x: 765, y: 56, w: 207, h: 50, color: 'blue', title: 'GCoreSystem::Update()', lines: ['while 루프 — 매 프레임'] },
    { id: 'X', x: 765, y: 146, w: 207, h: 50, color: 'orange', title: 'GCoreSystem::Destroy', lines: ['종료 처리'] },
    { id: 'X1', x: 765, y: 220, w: 207, h: 64, color: 'orange', title: '시스템 Destroy — 역순', lines: ['모듈은 연결 역순으로 ShutdownModule'] },
    { id: 'X2', x: 765, y: 308, w: 207, h: 64, color: 'orange', title: 'GC Flush', lines: ['→ Core 시스템 해제', '→ DLL FreeLibrary'] }
  ],
  edges: [
    { from: 'S', to: 'D', points: [[230, 104], [230, 128]] },
    { from: 'D', to: 'A', points: [[230, 176], [230, 200]] },
    { from: 'A', to: 'L1', points: [[150, 240], [120, 240], [120, 320]], label: '아니오', lpos: 'l' },
    { from: 'A', to: 'L2', points: [[310, 240], [340, 240], [340, 320]], label: '예' },
    { from: 'L1', to: 'C', points: [[120, 370], [120, 392], [463, 392], [463, 81], [490, 81]] },
    { from: 'L2', to: 'C', points: [[435, 345], [463, 345], [463, 81], [490, 81]] },
    { from: 'C', to: 'C1' },
    { from: 'C1', to: 'C2' },
    { from: 'C2', to: 'C3' },
    { from: 'C3', to: 'C4' },
    { from: 'C4', to: 'C5' },
    { from: 'C5', to: 'C6' },
    { from: 'C6', to: 'F', points: [[712, 566], [739, 566], [739, 81], [765, 81]] },
    { from: 'F', to: 'X', label: 'WM_QUIT' },
    { from: 'X', to: 'X1' },
    { from: 'X1', to: 'X2' }
  ]
};
