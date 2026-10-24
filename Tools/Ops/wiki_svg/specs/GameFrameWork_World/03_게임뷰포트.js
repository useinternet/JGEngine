// 2-1. Render와 HandleInput — 왼쪽 = 그리기 차례, 오른쪽 = 입력 라우팅 차례(게임 UI 먼저 → 월드 클릭 → 호스트)
// 코드: Viewport/GameViewport.h · .cpp(Render · HandleInput), UI/GameUIManager.h(HandlePointer · HandleBackAction · HandleTextInput · HandleKey),
//       Actors/GameplayControllerActor.h(HandleClick), Graphics Classes/SceneRenderer.h(Render · Resize)
module.exports = {
  file: 'GameFrameWork_World_03_게임뷰포트',
  alt: 'Render와 HandleInput — PGameViewport::Render는 크기가 바뀌었으면 렌더러 크기를 맞추고 활성 월드를 활성 카메라로 PSceneRenderer에 그린 뒤 디스플레이 텍스처 위에 게임 UI를 그리며, HandleInput은 포인터를 게임 UI에 먼저 넘기고, UI가 받지 않은 왼쪽 누름은 카메라 광선으로 바꿔 모든 컨트롤러의 HandleClick을 호출하며, Esc는 뒤로가기, 글자 · 키는 포커스된 입력란으로 보내고, 받지 않은 입력은 호스트가 카메라 조작에 쓴다',
  type: 'flow',
  w: 1000,
  h: 600,
  groups: [
    { x: 20, y: 20, w: 440, h: 560, label: 'Render — 호스트가 매 프레임' },
    { x: 480, y: 20, w: 500, h: 420, label: 'HandleInput(HInputEvent)' }
  ],
  nodes: [
    { id: 'R1', x: 50, y: 60, w: 380, h: 56, color: 'gray', title: '크기 변경 반영', lines: ['PSceneRenderer::Resize'] },
    { id: 'R2', x: 50, y: 150, w: 380, h: 70, color: 'green', title: 'PSceneRenderer::Render', lines: ['활성 월드의 PScene', '활성 카메라로 디스플레이 텍스처에'] },
    { id: 'R3', x: 50, y: 254, w: 380, h: 56, color: 'purple', title: 'PGameUIManager::Render', lines: ['디스플레이 텍스처 위에 게임 UI'] },
    { id: 'R4', x: 50, y: 344, w: 380, h: 70, color: 'blue', title: 'GetDisplayTexture', lines: ['에디터: ImGui 창에 표시', 'GameHost: 최종 텍스처로 제출'] },
    { id: 'I1', x: 510, y: 60, w: 440, h: 56, color: 'purple', title: '게임 UI 먼저', lines: ['HandlePointer · 위 레이어부터, Menu 위젯에서 멈춤'] },
    { id: 'I2', x: 510, y: 150, w: 440, h: 56, color: 'purple', title: 'Esc · 글자 · 편집 키', lines: ['뒤로가기 · 포커스된 입력란으로'] },
    { id: 'I3', x: 510, y: 240, w: 440, h: 70, color: 'green', title: 'UI가 받지 않은 왼쪽 누름', lines: ['활성 카메라 광선 →', '모든 컨트롤러 HandleClick(ray)'] },
    { id: 'I4', x: 510, y: 344, w: 440, h: 56, color: 'gray', title: '받지 않은 입력', lines: ['false 반환 — 호스트가 궤도 · 줌에 사용'] },
    { id: 'NOTE', x: 50, y: 444, w: 380, h: 110, color: 'white', shape: 'note', title: '헤드리스(그래픽 없음)', lines: ['렌더러를 만들지 않고 입력 라우팅만 한다', '— JGConsole 자체 시험'] }
  ],
  edges: [
    { from: 'R1', to: 'R2' },
    { from: 'R2', to: 'R3' },
    { from: 'R3', to: 'R4' },
    { from: 'I1', to: 'I2' },
    { from: 'I2', to: 'I3' },
    { from: 'I3', to: 'I4' }
  ]
};
