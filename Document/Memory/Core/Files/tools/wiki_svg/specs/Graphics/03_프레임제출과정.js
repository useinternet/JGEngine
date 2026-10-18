// 2-2. 프레임 제출 과정 — 묶음 = 메인 스레드 실행 단계. 줄마다 왼쪽 → 오른쪽, 줄 끝에서 다음 줄 처음으로 내려간다.
// 2026-10-18 옛 그림(specs_unused/Graphics_00_한프레임.js)을 다시 그림 — 수치 · 예외 글을 빼고 제출 우선순위를 더함
// 코드: JGGraphics.cpp(ScheduleByFrame), DirectX12/DirectX12API.cpp(BeginFrame · EndFrame), Classes/CommandQueue.h(ECommandListPriority), DX12FrameBuffer.cpp(Update)
module.exports = {
  file: 'Graphics_03_프레임제출과정',
  alt: '프레임 제출 과정 — GraphicsBegin 단계의 BeginFrame(이전 프레임 완료 대기 · 미룬 해제, 끝난 리드백 콜백), Update · End 단계의 그리기 기록과 최종 텍스처 제출, GraphicsEnd 단계의 EndFrame(업로드 · 백 버퍼 · 리드백 기록, 우선순위 순 제출, Present), UpdateWindow 단계의 스왑체인 크기 변경 순서',
  type: 'flow',
  w: 960,
  h: 660,
  groups: [
    { x: 20, y: 20, w: 920, h: 124, label: 'GraphicsBegin 단계' },
    { x: 20, y: 164, w: 430, h: 124, label: 'Update 단계' },
    { x: 470, y: 164, w: 470, h: 124, label: 'End 단계' },
    { x: 20, y: 308, w: 920, h: 124, label: 'GraphicsEnd 단계' },
    { x: 20, y: 452, w: 920, h: 110, label: 'UpdateWindow 단계' }
  ],
  nodes: [
    { id: 'A', x: 80, y: 56, w: 160, h: 70, color: 'blue', title: 'BeginFrame', lines: ['프레임 시작'] },
    { id: 'A1', x: 290, y: 56, w: 260, h: 70, color: 'purple', title: 'PCommandQueue::Begin', lines: ['같은 인덱스의 이전 프레임 완료 대기', '미룬 해제 처리'] },
    { id: 'A2', x: 600, y: 56, w: 280, h: 70, color: 'purple', title: 'PResourceStagingManager', lines: ['끝난 리드백의 완료 콜백 호출'] },
    { id: 'B', x: 80, y: 200, w: 320, h: 70, color: 'green', title: '그리기 기록', lines: ['PSceneRenderer::Render 등', 'Default 커맨드 리스트에 기록'] },
    { id: 'C', x: 510, y: 200, w: 380, h: 70, color: 'orange', title: 'SubmitFinalTexture', lines: ['GameHost가 게임 화면 텍스처 제출'] },
    { id: 'D', x: 100, y: 344, w: 130, h: 70, color: 'blue', title: 'EndFrame', lines: ['프레임 마무리'] },
    { id: 'D1', x: 260, y: 344, w: 260, h: 70, color: 'purple', title: '업로드 → 백 버퍼 → 리드백', lines: ['각 우선순위 리스트에 기록', '백 버퍼: 텍스처 복사 · ImGui'] },
    { id: 'D2', x: 550, y: 344, w: 220, h: 70, color: 'purple', title: 'PCommandQueue::End', lines: ['우선순위 순 제출 · Signal'] },
    { id: 'D3', x: 800, y: 344, w: 120, h: 70, color: 'blue', title: 'Present', lines: ['화면 표시'] },
    { id: 'E', x: 80, y: 488, w: 400, h: 56, color: 'gray', title: '메시지 펌프', lines: ['WM_SIZE → 스왑체인 크기 변경'] },
    { id: 'P', x: 20, y: 590, w: 920, h: 50, color: 'white', shape: 'note', title: '제출 순서 (ECommandListPriority)', lines: ['Upload(업로드) → Default(그리기) → Readback(리드백) → Present(백 버퍼)'] }
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
