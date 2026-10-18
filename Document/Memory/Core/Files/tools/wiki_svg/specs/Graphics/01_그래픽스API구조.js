// 1-1. HJGGraphicsModule과 PJGGraphicsAPI — 모듈이 API를 생성 · 소유하고 프레임 함수를 예약, API가 만드는 GPU 리소스는 IJGGraphicsObject 인터페이스
// 아래 여섯 인터페이스는 두 줄로 놓고 가운데 세로줄(TRUNK)로 IJGGraphicsObject 상속을 잇는다(Core_00 과 같은 방식).
// 코드: Runtime/Graphics/JGGraphics.h/.cpp, JGGraphicsAPI.h, JGGraphicsObject.h, Classes/Texture.h · Mesh.h · Material.h · FrameBuffer.h · VertexBuffer.h · IndexBuffer.h · ConstantBuffer.h · StructuredBuffer.h
const TRUNK = 515;
const PARENT_BOTTOM = 542;
const kids = [
  { id: 'TEX', name: 'IRawTexture', stereo: '텍스처' },
  { id: 'VB', name: 'IVertexBuffer · IIndexBuffer', stereo: '정점 · 인덱스 버퍼' },
  { id: 'CB', name: 'IConstantBuffer · IStructuredBuffer', stereo: '상수 · 구조화 버퍼' },
  { id: 'MAT', name: 'IRawMaterial', stereo: '머터리얼 (4번)' },
  { id: 'MESH', name: 'IMesh', stereo: '렌더 메시 (7번)' },
  { id: 'FB', name: 'IFrameBuffer', stereo: '프레임 버퍼 (8번)' }
].map((c, i) => Object.assign({
  x: i < 3 ? 180 : 560,
  y: 600 + (i % 3) * 70,
  w: 290,
  color: 'purple'
}, c));

module.exports = {
  file: 'Graphics_01_그래픽스API구조',
  alt: 'HJGGraphicsModule과 PJGGraphicsAPI — HJGGraphicsModule은 PJGGraphicsAPI(구현 PDirectX12API)를 생성 · 소유하고 BeginFrame · EndFrame을 프레임 단계에 예약하며, 다른 모듈은 GetGraphicsAPI()로 API에 접근하고, API가 생성하는 GPU 리소스는 IJGGraphicsObject를 상속한 인터페이스(텍스처, 버퍼, 머터리얼, 메시, 프레임 버퍼)다',
  type: 'class',
  w: 980,
  h: 890,
  classes: [
    { id: 'MOD', x: 40, y: 30, w: 330, name: 'HJGGraphicsModule', stereo: 'Graphics 모듈 클래스', color: 'blue', members: ['-_graphicsAPI : PJGGraphicsAPI', '#StartupModule()  // API 생성 · 초기화', '#ShutdownModule()', '-BeginFrame()  // GraphicsBegin 단계', '-EndFrame()  // GraphicsEnd 단계'] },
    { id: 'API', x: 460, y: 30, w: 480, name: 'PJGGraphicsAPI', stereo: '그래픽스 API · 추상 클래스', color: 'blue', members: ['#Initialize() · Destroy()', '#BeginFrame() · EndFrame()', '#OnWindowResized(w, h)', '+CreateRawTexture() · CreateVertexBuffer() · CreateIndexBuffer()', '+CreateRawMaterial()', '+CreateStaticMesh() · CreateTexture()  // 에셋', '+GetGraphicsCommand()', '+RequestTextureReadback() · ReadbackTextureImmediate()', '+SubmitFinalTexture() · SetVSyncEnabled()'] },
    { id: 'DX', x: 560, y: 300, w: 280, name: 'PDirectX12API', stereo: 'DX12 백엔드 구현 · 2번', color: 'yellow' },
    { id: 'OBJ', x: 355, y: 470, w: 320, name: 'IJGGraphicsObject', stereo: 'GPU 리소스 기반 인터페이스 · IMemoryObject 상속', color: 'purple', members: ['+GetName() · SetName()'] }
  ].concat(kids),
  nodes: [
    { id: 'SCH', x: 40, y: 230, w: 330, h: 56, color: 'gray', title: 'GScheduleGlobalSystem', lines: ['Core · 프레임 단계별 작업 예약'] },
    { id: 'GET', x: 40, y: 330, w: 330, h: 56, color: 'yellow', title: 'GetGraphicsAPI()', lines: ['전역 함수 · 모듈의 API 반환'] },
    { id: 'KEY', x: 40, y: 420, w: 280, h: 96, color: 'white', shape: 'note', title: '범례', lines: ['◇— 소유 · —▷ 상속 · → 생성 · 참조', '- - → 예약', '+ public · # protected · - private'] },
    { id: 'IMPL', x: 180, y: 810, w: 670, h: 56, color: 'white', shape: 'note', title: '구현 클래스', lines: ['PDX12Texture · PDX12VertexBuffer · PDX12Material · PDX12FrameBuffer 등(DX12), PStaticMesh(IMesh)'] }
  ],
  edges: [
    { from: 'MOD', to: 'API', fs: 'r', ts: 'l', fo: 36, to_off: 0, start: 'diamond', end: 'none', label: '생성 · 소유' },
    { from: 'MOD', to: 'SCH', fs: 'b', ts: 't', dash: true, label: 'BeginFrame · EndFrame 예약' },
    { points: [[370, 358], [430, 358], [430, 220], [460, 220]], label: '참조', lx: 430, ly: 290, lpos: 'l' },
    { from: 'DX', to: 'API', fs: 't', ts: 'b', end: 'triangle' },
    { from: 'API', to: 'OBJ', fs: 'b', ts: 't', fo: -185, to_off: 0, label: '생성 · 반환', lx: 515, ly: 420 }
  ].concat(kids.map(c => {
    const left = c.x < TRUNK;
    const cy = c.y + 22;
    return { points: [[left ? c.x + c.w : c.x, cy], [TRUNK, cy], [TRUNK, PARENT_BOTTOM]], end: 'triangle' };
  }))
};
