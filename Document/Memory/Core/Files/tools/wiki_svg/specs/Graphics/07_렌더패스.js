// 5-2. 렌더 패스 — PSceneRenderer::Render 의 세 패스(왼쪽 = 패스, 오른쪽 = 결과 텍스처)와 디스플레이 텍스처 사용처
// 코드: Classes/SceneRenderer.cpp(renderGeometryPass · renderCompositePass · renderDisplayPass), Editor/JGEditor/Widgets/SceneViewport.cpp, GameFrameWorks/UI/GameUIRenderer.cpp, GameHost/GameHostModule.cpp(SubmitFinalTexture)
module.exports = {
  file: 'Graphics_07_렌더패스',
  alt: '렌더 패스 — PSceneRenderer::Render는 지오메트리 패스(Surface 머터리얼로 메시 배치를 G버퍼 · 깊이 버퍼에), 합성 패스(Screen 머터리얼로 G버퍼를 선형 색 출력 텍스처에), 디스플레이 패스(sRGB 인코딩으로 디스플레이 텍스처에) 순으로 그리고, 디스플레이 텍스처는 에디터 씬 뷰포트 · 게임 UI · GameHost의 최종 텍스처 제출에 사용한다',
  type: 'flow',
  w: 980,
  h: 490,
  groups: [
    { x: 280, y: 20, w: 680, h: 320, label: 'PSceneRenderer::Render' }
  ],
  nodes: [
    { id: 'IN', x: 40, y: 60, w: 200, h: 60, color: 'gray', title: 'PScene', lines: ['메시 배치 · 카메라'] },
    { id: 'P1', x: 300, y: 60, w: 280, h: 60, color: 'blue', title: '지오메트리 패스', lines: ['Surface 머터리얼로 메시마다 그리기'] },
    { id: 'T1', x: 640, y: 60, w: 300, h: 60, color: 'yellow', title: 'G버퍼 · 깊이 버퍼', lines: ['Albedo · Normal · Specular · Depth'] },
    { id: 'P2', x: 300, y: 160, w: 280, h: 60, color: 'blue', title: '합성 패스', lines: ['Screen 머터리얼로 G버퍼 합성'] },
    { id: 'T2', x: 640, y: 160, w: 300, h: 60, color: 'yellow', title: '출력 텍스처', lines: ['선형 색 · 리드백 · 후처리용'] },
    { id: 'P3', x: 300, y: 260, w: 280, h: 60, color: 'blue', title: '디스플레이 패스', lines: ['sRGB 인코딩'] },
    { id: 'T3', x: 640, y: 260, w: 300, h: 60, color: 'yellow', title: '디스플레이 텍스처', lines: ['화면 표시용'] },
    { id: 'C1', x: 200, y: 400, w: 230, h: 60, color: 'purple', title: '에디터 씬 뷰포트', lines: ['GUI 이미지로 표시'] },
    { id: 'C2', x: 460, y: 400, w: 230, h: 60, color: 'purple', title: '게임 UI', lines: ['2D 그리기로 위에 렌더링'] },
    { id: 'C3', x: 720, y: 400, w: 240, h: 60, color: 'purple', title: 'GameHost', lines: ['SubmitFinalTexture → 백 버퍼'] }
  ],
  edges: [
    { from: 'IN', to: 'P1', fs: 'r', ts: 'l' },
    { from: 'P1', to: 'T1', fs: 'r', ts: 'l' },
    { from: 'T1', to: 'P2', fs: 'b', ts: 't', my: 140 },
    { from: 'P2', to: 'T2', fs: 'r', ts: 'l' },
    { from: 'T2', to: 'P3', fs: 'b', ts: 't', my: 240 },
    { from: 'P3', to: 'T3', fs: 'r', ts: 'l' },
    { from: 'T3', to: 'C1', fs: 'b', ts: 't', fo: -120, to_off: 0, my: 360 },
    { from: 'T3', to: 'C2', fs: 'b', ts: 't', fo: -60, to_off: 0, my: 375 },
    { from: 'T3', to: 'C3', fs: 'b', ts: 't', fo: 50, to_off: 0 }
  ]
};
