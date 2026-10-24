// 0-1. 시작 순서와 프레임 — 왼쪽 = StartupModule(옛 GameHost_00_시작순서, 엔진 모듈 목록을 Graphics → GameFrameWork 로), 오른쪽 = End 단계의 renderFrame과 입력 전달
// 코드: Runtime/GameHost/GameHostModule.h · .cpp(loadSettings · createWindow · connectModules · GAME_HOST_ENGINE_MODULES · renderFrame · onResize · onInput)
module.exports = {
  file: 'GameFrameWork_GameHost_00_GameHost시작과프레임',
  alt: '시작 순서와 프레임 — HGameHostModule::StartupModule은 에디터 빌드면 .jgproject, 패키지면 Bin/JGPackage.json에서 설정을 읽고 명령줄 인자를 우선 적용한 뒤, 창 생성 → 엔진 모듈(Graphics → GameFrameWork) 연결 → 게임 모듈 연결 → VSync 설정 · PGameViewport 생성 → OnResize · OnInput 구독과 renderFrame 예약 순으로 진행하고, renderFrame은 End 단계에서 뷰포트 크기를 맞추고 PGameViewport::Render 결과를 SubmitFinalTexture로 제출하며, 창 입력은 PGameViewport::HandleInput으로 전달한다',
  type: 'flow',
  w: 1100,
  h: 880,
  groups: [
    { x: 20, y: 20, w: 620, h: 840, label: 'StartupModule' },
    { x: 660, y: 20, w: 420, h: 460, label: 'renderFrame — End 단계' },
    { x: 660, y: 500, w: 420, h: 150, label: 'onInput — 창 입력' }
  ],
  nodes: [
    { id: 'A', x: 210, y: 60, w: 240, h: 54, color: 'blue', title: 'loadSettings', lines: ['설정 로드'] },
    { id: 'A1', x: 220, y: 140, w: 220, h: 80, color: 'white', shape: 'diamond', title: '에디터 빌드인가' },
    { id: 'A2', x: 40, y: 260, w: 260, h: 60, color: 'yellow', title: '.jgproject', lines: ['GameModules + Package.Window'] },
    { id: 'A3', x: 360, y: 260, w: 260, h: 60, color: 'yellow', title: 'Bin/JGPackage.json', lines: ['패키지 서술자'] },
    { id: 'B', x: 110, y: 360, w: 440, h: 60, color: 'blue', title: '명령줄 인자 우선 적용', lines: ['-windowed · -fullscreen · -res=WxH · -novsync'] },
    { id: 'C', x: 130, y: 450, w: 400, h: 56, color: 'green', title: 'createWindow', lines: ['게임 창 생성'] },
    { id: 'D', x: 130, y: 536, w: 400, h: 56, color: 'purple', title: '엔진 모듈 연결', lines: ['Graphics → GameFrameWork'] },
    { id: 'E', x: 130, y: 622, w: 400, h: 56, color: 'purple', title: '게임 모듈 연결', lines: ['에디터 모듈은 연결하지 않음'] },
    { id: 'F', x: 130, y: 708, w: 400, h: 56, color: 'green', title: 'VSync 설정 · PGameViewport 생성', lines: ['뷰포트 크기 = 창 크기'] },
    { id: 'G', x: 130, y: 794, w: 400, h: 56, color: 'orange', title: 'OnResize · OnInput 구독', lines: ['End 단계에 renderFrame 예약'] },
    { id: 'R1', x: 690, y: 60, w: 360, h: 56, color: 'gray', title: '크기 변경 반영', lines: ['창 크기 변경이 멈추면 뷰포트 Resize'] },
    { id: 'R2', x: 690, y: 160, w: 360, h: 70, color: 'green', title: 'PGameViewport::Render', lines: ['월드 → 디스플레이 텍스처', '→ 게임 UI'] },
    { id: 'R3', x: 690, y: 274, w: 360, h: 70, color: 'blue', title: 'SubmitFinalTexture', lines: ['Graphics가 GraphicsEnd 단계에', '백 버퍼로 복사 · Present'] },
    { id: 'R4', x: 690, y: 388, w: 360, h: 56, color: 'purple', title: 'IME 조합 창 위치', lines: ['게임 입력란에 포커스가 있을 때'] },
    { id: 'I1', x: 690, y: 540, w: 360, h: 70, color: 'orange', title: 'HInputEvent', lines: ['PGameViewport::HandleInput', '게임 UI 먼저 → 월드 클릭'] }
  ],
  edges: [
    { from: 'A', to: 'A1' },
    { from: 'A1', to: 'A2', fs: 'l', ts: 't', label: '예 — JGLauncher -game', lpos: 'l', lx: 170, ly: 225 },
    { from: 'A1', to: 'A3', fs: 'r', ts: 't', label: '아니오 — 패키지', lx: 490, ly: 225 },
    { from: 'A2', to: 'B', fs: 'b', ts: 't', to_off: -160 },
    { from: 'A3', to: 'B', fs: 'b', ts: 't', to_off: 160 },
    { from: 'B', to: 'C' },
    { from: 'C', to: 'D' },
    { from: 'D', to: 'E' },
    { from: 'E', to: 'F' },
    { from: 'F', to: 'G' },
    { from: 'R1', to: 'R2' },
    { from: 'R2', to: 'R3' },
    { from: 'R3', to: 'R4' }
  ]
};
