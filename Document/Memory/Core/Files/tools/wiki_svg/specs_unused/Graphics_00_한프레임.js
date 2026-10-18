// 한 프레임 (Graphics 시점) — 묶음 = 메인 스레드 실행 단계. 줄마다 왼쪽 → 오른쪽, 줄 끝에서 다음 줄 처음으로 내려간다.
// Update · End 단계는 상자가 하나씩이라 한 줄에 나란히 두었다.
module.exports = {
  file: 'Graphics_00_한프레임',
  alt: '한 프레임 (Graphics 시점) — GraphicsBegin의 BeginFrame(2프레임 전 펜스 대기 · 미룬 해제 · 끝난 리드백 콜백), Update의 PSceneRenderer::Render 기록, End의 GameHost SubmitFinalTexture, GraphicsEnd의 EndFrame(업로드 → 백 버퍼 → 리드백 기록, 우선순위 순 제출 · Signal, Present), UpdateWindow의 메시지 펌프 · 스왑체인 Resize 순서',
  type: 'flow',
  w: 960,
  h: 582,
  groups: [
    { x: 20, y: 20, w: 920, h: 124, label: 'GraphicsBegin 단계' },
    { x: 20, y: 164, w: 430, h: 124, label: 'Update 단계' },
    { x: 470, y: 164, w: 470, h: 124, label: 'End 단계' },
    { x: 20, y: 308, w: 920, h: 124, label: 'GraphicsEnd 단계' },
    { x: 20, y: 452, w: 920, h: 110, label: 'UpdateWindow 단계' }
  ],
  nodes: [
    { id: 'A', x: 80, y: 56, w: 160, h: 70, color: 'blue', title: 'BeginFrame', lines: ['프레임 시작'] },
    { id: 'A1', x: 290, y: 56, w: 240, h: 70, color: 'purple', title: 'PCommandQueue::Begin', lines: ['2프레임 전 펜스만 대기', '미룬 해제 처리'] },
    { id: 'A2', x: 580, y: 56, w: 240, h: 70, color: 'purple', title: 'PResourceStagingManager', lines: ['BeginFrame · 끝난 리드백 콜백'] },
    { id: 'B', x: 80, y: 200, w: 320, h: 70, color: 'green', title: 'PSceneRenderer::Render', lines: ['위젯 · 게임 뷰포트가 호출', '커맨드 리스트에 바로 기록'] },
    { id: 'C', x: 510, y: 200, w: 380, h: 70, color: 'orange', title: 'SubmitFinalTexture', lines: ['GameHost가 게임 화면 Render 후 호출'] },
    { id: 'D', x: 90, y: 344, w: 150, h: 70, color: 'blue', title: 'EndFrame', lines: ['프레임 마무리'] },
    { id: 'D1', x: 276, y: 344, w: 220, h: 70, color: 'purple', title: '업로드 → 백 버퍼 → 리드백', lines: ['순서대로 기록', '백 버퍼: 복사 · ImGui'] },
    { id: 'D2', x: 532, y: 344, w: 200, h: 70, color: 'purple', title: 'PCommandQueue::End', lines: ['우선순위 순 제출 · Signal'] },
    { id: 'D3', x: 768, y: 344, w: 150, h: 70, color: 'blue', title: 'Present', lines: ['VSync 사용 시 간격 1'] },
    { id: 'E', x: 80, y: 488, w: 360, h: 56, color: 'gray', title: '메시지 펌프', lines: ['WM_SIZE → 스왑체인 Resize (프레임 밖)'] }
  ],
  edges: [
    { from: 'A', to: 'A1', fs: 'r', ts: 'l' },
    { from: 'A1', to: 'A2', fs: 'r', ts: 'l' },
    { from: 'A2', to: 'B', fs: 'b', ts: 't', my: 154 },
    { from: 'B', to: 'C', fs: 'r', ts: 'l' },
    { from: 'C', to: 'D', fs: 'b', ts: 't', my: 298 },
    { from: 'D', to: 'D1', fs: 'r', ts: 'l' },
    { from: 'D1', to: 'D2', fs: 'r', ts: 'l' },
    { from: 'D2', to: 'D3', fs: 'r', ts: 'l' },
    { from: 'D3', to: 'E', fs: 'b', ts: 't', my: 442 }
  ]
};
