// 3-1. JGSceneViewport와 PGameViewport — 에디터 창이 GameFrameWork 게임 뷰포트를 그리고(Render) · 보여 주고(디스플레이 텍스처) · 입력을 넘긴다(HandleInput).
//      게임이 받지 않은 입력은 에디터 카메라 조작에 쓴다. GameHost 는 같은 게임 뷰포트를 창 전체로 쓴다.
//      돌아오는 화살표(텍스처 · 받지 않은 입력)는 묶음 사이가 좁아 글을 상자 안에 적었다.
// 코드(구조 개편 뒤): Editor/JGEditor/Widgets/SceneViewport.h · .cpp, Runtime/GameFrameWork/Viewport/GameViewport.h, Runtime/GameHost/GameHostModule.cpp
module.exports = {
  file: 'JGEditor_04_씬뷰포트',
  alt: 'JGSceneViewport와 PGameViewport — JGSceneViewport는 OnUpdate에서 PGameViewport::Render로 월드와 게임 UI를 디스플레이 텍스처에 그리고, OnGenerateGUI에서 그 텍스처를 이미지로 표시하며, ImGui 입력을 HInputEvent로 바꿔 HandleInput에 넘기고, 게임이 받지 않은 입력은 카메라 궤도 · 줌에 쓰며, 표시 크기가 멈추면 렌더 크기를 맞추고, GameHost는 같은 PGameViewport를 창 전체로 사용한다',
  type: 'flow',
  w: 1140,
  h: 560,
  groups: [
    { x: 20, y: 20, w: 400, h: 520, label: 'JGSceneViewport (JGEditor 창)', color: 'green' },
    { x: 520, y: 20, w: 320, h: 520, label: 'PGameViewport (GameFrameWork)', color: 'blue' },
    { x: 920, y: 20, w: 200, h: 520, label: 'GameHost', color: 'purple' }
  ],
  nodes: [
    { id: 'A1', x: 40, y: 60, w: 360, h: 60, color: 'green', title: 'OnUpdate', lines: ['매 프레임 Render 호출'] },
    { id: 'A2', x: 40, y: 160, w: 360, h: 60, color: 'green', title: 'OnGenerateGUI', lines: ['디스플레이 텍스처를 창에 맞춰 이미지로 표시'] },
    { id: 'A3', x: 40, y: 260, w: 360, h: 60, color: 'green', title: '입력 변환', lines: ['ImGui 입력 → HInputEvent(뷰포트 픽셀)'] },
    { id: 'A4', x: 40, y: 360, w: 360, h: 60, color: 'green', title: '에디터 조작', lines: ['받지 않은 입력(대상 None) → 궤도 · 줌'] },
    { id: 'A5', x: 40, y: 460, w: 360, h: 60, color: 'green', title: '렌더 크기 맞춤', lines: ['표시 크기가 멈춘 뒤 Resize'] },
    { id: 'B1', x: 540, y: 60, w: 280, h: 90, color: 'blue', title: 'Render', lines: ['PWorld → PSceneRenderer', '→ 디스플레이 텍스처', '→ 게임 UI'] },
    { id: 'B2', x: 540, y: 250, w: 280, h: 90, color: 'blue', title: 'HandleInput', lines: ['게임 UI 먼저', '나머지 클릭 → 카메라 광선', '→ 컨트롤러 HandleClick'] },
    { id: 'B3', x: 540, y: 455, w: 280, h: 70, color: 'blue', title: 'Resize', lines: ['렌더러 출력 크기 변경'] },
    { id: 'C1', x: 935, y: 60, w: 170, h: 90, color: 'purple', title: 'HGameHostModule', lines: ['창 전체가 뷰포트', '화면에 제출'] },
    { id: 'C2', x: 935, y: 250, w: 170, h: 90, color: 'purple', title: '창 입력', lines: ['창 픽셀 그대로', 'HandleInput'] }
  ],
  edges: [
    { from: 'A1', to: 'B1', fs: 'r', ts: 'l', fo: 0, to_off: -15, label: 'Render' },
    { from: 'B1', to: 'A2', fs: 'l', ts: 'r', fo: 25, to_off: 0, via: [[470, 130], [470, 190]] },
    { from: 'A3', to: 'B2', fs: 'r', ts: 'l', fo: 0, to_off: -15, label: 'HandleInput' },
    { from: 'B2', to: 'A4', fs: 'l', ts: 'r', fo: 25, to_off: 0, via: [[470, 320], [470, 390]] },
    { from: 'A5', to: 'B3', fs: 'r', ts: 'l', label: 'Resize' },
    { from: 'C1', to: 'B1', fs: 'l', ts: 'r', label: '같은 뷰포트' },
    { from: 'C2', to: 'B2', fs: 'l', ts: 'r' }
  ]
};
