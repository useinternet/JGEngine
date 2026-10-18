// 8-1. IFrameBuffer와 PDX12FrameBuffer — 창 크기 변경 경로, GameHost의 최종 텍스처 제출, GUI 백엔드의 백 버퍼 이벤트 구독
// 코드: JGGraphics.cpp(OnWindowResize 구독), JGGraphicsAPI.h, Classes/FrameBuffer.h, DirectX12/DX12FrameBuffer.h/.cpp, DirectX12API.cpp(OnWindowResized · SubmitFinalTexture), GameHost/GameHostModule.cpp, GUI/Backends/DX12GUIBackend.cpp
module.exports = {
  file: 'Graphics_10_화면출력구조',
  alt: 'IFrameBuffer와 PDX12FrameBuffer — Core 창 이벤트 OnResize를 HJGGraphicsModule이 구독해 PJGGraphicsAPI::OnWindowResized로 스왑체인 크기를 바꾸고, GameHost는 SubmitFinalTexture로 최종 텍스처를 제출하며, PDX12FrameBuffer는 Update에서 제출된 텍스처를 백 버퍼에 복사한 뒤 OnUpdate · OnPresent 이벤트로 GUI 백엔드가 ImGui를 기록하게 한다',
  type: 'class',
  w: 1060,
  h: 510,
  classes: [
    { id: 'MOD', x: 40, y: 150, w: 240, name: 'HJGGraphicsModule', stereo: 'Graphics 모듈 클래스', color: 'blue', members: ['-OnWindowResize(w, h)'] },
    { id: 'API', x: 400, y: 150, w: 340, name: 'PJGGraphicsAPI', stereo: '그래픽스 API · 구현 PDirectX12API', color: 'blue', members: ['#OnWindowResized(w, h)', '+SubmitFinalTexture(texture)', '+SetVSyncEnabled(enabled)'] },
    { id: 'FB', x: 400, y: 330, w: 340, name: 'PDX12FrameBuffer', stereo: 'DX12 스왑체인 · PDirectX12API 소유', color: 'yellow', members: ['-_dx12SwapChain · _dx12BackBuffers', '-_submittedTexture', '+Update()  // 텍스처 복사 → OnUpdate', '+Present()  // → OnPresent', '+Resize(w, h)'] },
    { id: 'IFB', x: 780, y: 380, w: 240, name: 'IFrameBuffer', stereo: '프레임 버퍼 인터페이스', color: 'purple', members: ['+SubmitTexture() · Resize()'] }
  ],
  nodes: [
    { id: 'WIN', x: 40, y: 30, w: 240, h: 56, color: 'gray', title: 'HWindowCallBacks', lines: ['Core 창 이벤트 · OnResize'] },
    { id: 'GH', x: 440, y: 30, w: 260, h: 56, color: 'gray', title: 'GameHost', lines: ['게임 화면 Render 후 제출'] },
    { id: 'GUI', x: 760, y: 30, w: 260, h: 56, color: 'gray', title: 'PDX12GUIBackend', lines: ['GUI 모듈 · ImGui 기록'] },
    { id: 'KEY', x: 40, y: 330, w: 300, h: 96, color: 'white', shape: 'note', title: '범례', lines: ['—▷ 상속 · → 호출', '- - → 이벤트 구독', '+ public · # protected · - private'] }
  ],
  edges: [
    { from: 'WIN', to: 'MOD', fs: 'b', ts: 't', dash: true, label: 'OnResize 구독' },
    { from: 'MOD', to: 'API', fs: 'r', ts: 'l', fo: 0, to_off: -18, label: 'OnWindowResized' },
    { from: 'GH', to: 'API', fs: 'b', ts: 't', label: 'SubmitFinalTexture' },
    { from: 'API', to: 'FB', fs: 'b', ts: 't', label: 'Resize · SubmitTexture' },
    { from: 'FB', to: 'IFB', fs: 'r', ts: 'l', fo: 14, to_off: 0, end: 'triangle' },
    { points: [[890, 86], [890, 350], [740, 350]], dash: true, label: 'OnUpdate · OnPresent 구독', lx: 890, ly: 200 }
  ]
};
