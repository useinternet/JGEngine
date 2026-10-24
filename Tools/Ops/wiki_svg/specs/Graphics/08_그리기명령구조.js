// 6-1. IJGGraphicsCommand와 PDX12GraphicsCommand — 그리기 인터페이스, 세 가지 Draw 인자, DX12 구현이 PSO를 구성해 커맨드 리스트에 기록
// 코드: JGGraphicsCommand.h, JGGraphicsDefine.h(HDrawArguments · HScreenDrawArguments · H2DDrawArguments · H2DDrawCommand), DirectX12/DX12GraphicsCommand.h/.cpp, GameFrameWorks/UI/GameUIRenderer.cpp
module.exports = {
  file: 'Graphics_08_그리기명령구조',
  alt: 'IJGGraphicsCommand와 PDX12GraphicsCommand — PSceneRenderer는 메시 · 화면 그리기, PGameUIRenderer는 2D 그리기를 호출하고, Draw는 HDrawArguments(메시) · HScreenDrawArguments(Screen 머터리얼) · H2DDrawArguments(정점 · 인덱스 · 텍스처별 명령)를 받으며, PDX12GraphicsCommand가 루트 서명과 PSO를 구성해 현재 프레임의 PGraphicsCommandList에 기록한다',
  type: 'class',
  w: 1040,
  h: 520,
  classes: [
    { id: 'CMD', x: 40, y: 140, w: 580, name: 'IJGGraphicsCommand', stereo: '그리기 명령 인터페이스 · PJGGraphicsAPI::GetGraphicsCommand()로 얻음', color: 'purple', members: ['+BeginDraw() · EndDraw()', '+SetRenderTarget(renderTarget)  // 렌더 텍스처 · 깊이 텍스처', '+SetRenderPassData(data)  // 카메라 행렬 · 해상도', '+Draw(HDrawArguments)  // 메시', '+Draw(HScreenDrawArguments)  // 화면 전체', '+Draw(H2DDrawArguments)  // 2D', '+ClearTexture() · ClearDepthTexture()'] },
    { id: 'DXC', x: 700, y: 140, w: 300, name: 'PDX12GraphicsCommand', stereo: 'DX12 구현', color: 'blue', members: ['-_rootSignature : PRootSignature', '-_graphicsPSO : PGraphicsPipelineState', '+Draw()  // PSO 구성 · 전역 캐시 조회'] },
    { id: 'A1', x: 40, y: 400, w: 260, name: 'HDrawArguments', stereo: '메시 그리기 인자', color: 'orange', members: ['Mesh : IMesh', 'ObjectCBData · MaterialOverrides'] },
    { id: 'A2', x: 320, y: 400, w: 240, name: 'HScreenDrawArguments', stereo: '화면 그리기 인자', color: 'orange', members: ['Material : Screen 머터리얼'] },
    { id: 'A3', x: 580, y: 400, w: 300, name: 'H2DDrawArguments', stereo: '2D 그리기 인자 · 내장 2D 셰이더', color: 'orange', members: ['Vertices · Indices', 'Commands : 텍스처 · 시저 영역'] }
  ],
  nodes: [
    { id: 'O1', x: 40, y: 30, w: 250, h: 56, color: 'gray', title: 'PSceneRenderer', lines: ['메시 · 화면 그리기'] },
    { id: 'O2', x: 320, y: 30, w: 300, h: 56, color: 'gray', title: 'PGameUIRenderer', lines: ['GameFrameWork 게임 UI · 2D 그리기'] },
    { id: 'KEY', x: 660, y: 30, w: 340, h: 90, color: 'white', shape: 'note', title: '범례', lines: ['—▷ 상속 · → 호출 · 기록', '- - → 인자', '+ public · - private'] },
    { id: 'GCL', x: 700, y: 300, w: 300, h: 56, color: 'gray', title: 'PGraphicsCommandList', lines: ['현재 프레임의 커맨드 리스트'] }
  ],
  edges: [
    { from: 'O1', to: 'CMD', fs: 'b', ts: 't', fo: 0, to_off: -165, label: 'Draw(메시 · 화면)' },
    { from: 'O2', to: 'CMD', fs: 'b', ts: 't', fo: 0, to_off: 140, label: 'Draw(2D)' },
    { from: 'DXC', to: 'CMD', fs: 'l', ts: 'r', fo: 0, to_off: -36, end: 'triangle' },
    { from: 'DXC', to: 'GCL', fs: 'b', ts: 't', label: '명령 기록' },
    { from: 'CMD', to: 'A1', fs: 'b', ts: 't', fo: -160, to_off: 0, dash: true },
    { from: 'CMD', to: 'A2', fs: 'b', ts: 't', fo: 110, to_off: 0, dash: true },
    { from: 'CMD', to: 'A3', fs: 'b', ts: 't', fo: 270, to_off: 0, my: 372, dash: true }
  ]
};
