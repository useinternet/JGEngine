// 0. Graphics 모듈 — 사용하는 모듈 / Graphics.dll 세 계층(장면 · 에셋, API · 인터페이스, DX12 백엔드) / 의존 모듈
// 2026-10-18 사용자 위키 피드백(00.Core 짜임으로 다시 쓰기): 구조 그림 + 짧은 개념 설명, 사소한 세부 금지
// 코드: Runtime/Graphics/JGGraphics.h · JGGraphicsAPI.h · JGGraphicsCommand.h, Classes/Scene.h · SceneRenderer.h · StaticMesh.h · Texture.h · Material.h · ShaderLibrary.h,
//       DirectX12/DirectX12API.h · DX12FrameBuffer.h, DirectX12/Classes/CommandQueue.h · ResourceStagingManager.h, Importers/FBXAssetImporter.h
module.exports = {
  file: 'Graphics_00_Graphics모듈',
  alt: 'Graphics 모듈 — Graphics.dll은 장면 · 에셋 계층(PScene, PSceneRenderer, JGStaticMesh, JGTexture, JGFBXAssetImporter), 그래픽스 API · 인터페이스 계층(HJGGraphicsModule, PJGGraphicsAPI, IJGGraphicsCommand, IRawMaterial, GPU 리소스 인터페이스, GShaderLibrary), DX12 백엔드 계층(PDirectX12API와 PCommandQueue, PResourceStagingManager, PDX12FrameBuffer)으로 나뉘며, GameFrameWorks · JGEditor · GameHost · GUI가 사용하고 Core와 Asset에 의존한다',
  type: 'class',
  w: 1080,
  h: 1090,
  groups: [
    { x: 20, y: 20, w: 1040, h: 110, label: 'Graphics를 사용하는 모듈', color: 'purple' },
    { x: 20, y: 170, w: 1040, h: 750, label: 'Graphics 모듈 (Graphics.dll)' },
    { x: 40, y: 210, w: 1000, h: 130, label: '장면 · 에셋', dash: true },
    { x: 40, y: 380, w: 1000, h: 290, label: '그래픽스 API · 인터페이스', dash: true },
    { x: 40, y: 710, w: 1000, h: 190, label: 'DX12 백엔드', dash: true },
    { x: 20, y: 960, w: 700, h: 110, label: 'Graphics가 의존하는 모듈' }
  ],
  classes: [
    // 장면 · 에셋
    { id: 'SCN', x: 60, y: 248, w: 210, name: 'PScene', stereo: '장면 데이터', color: 'green', members: ['+CreateMesh() 등'] },
    { id: 'SR', x: 310, y: 248, w: 240, name: 'PSceneRenderer', stereo: '장면 렌더러', color: 'green', members: ['+Render(scene, camera)'] },
    { id: 'AST', x: 590, y: 248, w: 210, name: 'JGStaticMesh · JGTexture', stereo: '메시 · 텍스처 에셋', color: 'green', members: ['+GetMesh() · GetRawTexture()'] },
    { id: 'IMP', x: 840, y: 248, w: 180, name: 'JGFBXAssetImporter', stereo: 'FBX 임포터', color: 'green', members: ['+Import(args)'] },
    // 그래픽스 API · 인터페이스
    { id: 'MOD', x: 60, y: 420, w: 210, name: 'HJGGraphicsModule', stereo: '모듈 클래스', color: 'blue', members: ['#StartupModule()', '-BeginFrame() · EndFrame()'] },
    { id: 'API', x: 310, y: 420, w: 240, name: 'PJGGraphicsAPI', stereo: '그래픽스 API · 추상 클래스', color: 'blue', members: ['+CreateRawTexture() 등', '+GetGraphicsCommand()', '+SubmitFinalTexture()'] },
    { id: 'CMD', x: 590, y: 420, w: 210, name: 'IJGGraphicsCommand', stereo: '그리기 명령', color: 'blue', members: ['+SetRenderTarget()', '+Draw()  // 메시 · 화면 · 2D'] },
    { id: 'MAT', x: 840, y: 420, w: 180, name: 'IRawMaterial', stereo: '머터리얼', color: 'blue', members: ['+Set*() · Compile()'] },
    { id: 'RES', x: 60, y: 560, w: 210, name: 'IRawTexture · IMesh 등', stereo: 'GPU 리소스 인터페이스', color: 'blue', members: ['IJGGraphicsObject 상속'] },
    { id: 'SHL', x: 840, y: 560, w: 180, name: 'GShaderLibrary', stereo: '셰이더 템플릿', color: 'blue', members: ['+GetShaderCode(name)'] },
    // DX12 백엔드
    { id: 'CQ', x: 60, y: 750, w: 210, name: 'PCommandQueue', stereo: '커맨드 큐', color: 'yellow', members: ['+RequestCommandList()', '+Begin() · End()'] },
    { id: 'DX', x: 310, y: 750, w: 240, name: 'PDirectX12API', stereo: 'DX12 백엔드', color: 'yellow', members: ['-_dx12Device', '+CreateCommittedResource()', '+DestroyCommittedResource()'] },
    { id: 'STG', x: 590, y: 750, w: 210, name: 'PResourceStagingManager', stereo: '리소스 전송', color: 'yellow', members: ['+RequestTextureUpload() 등'] },
    { id: 'FB', x: 840, y: 750, w: 180, name: 'PDX12FrameBuffer', stereo: '스왑체인', color: 'yellow', members: ['+Present()'] }
  ],
  nodes: [
    { id: 'U1', x: 40, y: 56, w: 230, h: 56, color: 'purple', title: 'GameFrameWorks', lines: ['PWorld · PGameViewport · 게임 UI'] },
    { id: 'U2', x: 290, y: 56, w: 230, h: 56, color: 'purple', title: 'JGEditor · Devkit', lines: ['씬 뷰포트 · DevScene'] },
    { id: 'U3', x: 540, y: 56, w: 230, h: 56, color: 'purple', title: 'GameHost', lines: ['최종 화면 제출'] },
    { id: 'U4', x: 790, y: 56, w: 250, h: 56, color: 'purple', title: 'GUI', lines: ['ImGui DX12 백엔드'] },
    { id: 'CORE', x: 40, y: 996, w: 320, h: 56, color: 'gray', title: 'Core', lines: ['모듈 · 메모리 · 스케줄 · 창 이벤트'] },
    { id: 'ASSET', x: 380, y: 996, w: 320, h: 56, color: 'gray', title: 'Asset', lines: ['JGAsset · 에셋 경로'] },
    { id: 'KEY', x: 760, y: 960, w: 300, h: 110, color: 'white', shape: 'note', title: '범례', lines: ['◇— 소유 · —▷ 상속 · → 사용 · 생성', '- - → 이벤트 구독', '+ public · # protected · - private'] }
  ],
  edges: [
    // 모듈 사이
    { points: [[530, 130], [530, 170]], label: '사용' },
    { points: [[370, 920], [370, 960]], label: '의존' },
    { points: [[1040, 84], [1050, 84], [1050, 786], [1020, 786]], dash: true, label: 'OnUpdate · OnPresent 구독', lx: 1050, ly: 690, lpos: 'l' },
    // 장면 · 에셋
    { from: 'SR', to: 'SCN', fs: 'l', ts: 'r', label: '읽기' },
    { from: 'IMP', to: 'AST', fs: 'l', ts: 'r', label: '생성' },
    { from: 'SR', to: 'API', fs: 'b', ts: 't', label: 'GPU 리소스 생성 · 그리기 기록', lw: 200, lx: 430, ly: 370 },
    // 그래픽스 API · 인터페이스
    { from: 'MOD', to: 'API', fs: 'r', ts: 'l', fo: 0, to_off: -9, start: 'diamond', end: 'none', label: '소유' },
    { from: 'API', to: 'CMD', fs: 'r', ts: 'l', fo: -9, to_off: 0 },
    { from: 'API', to: 'RES', fs: 'b', ts: 'r', fo: -80, label: '생성', lx: 310, ly: 605 },
    { from: 'MAT', to: 'SHL', fs: 'b', ts: 't', label: '템플릿' },
    // DX12 백엔드
    { from: 'DX', to: 'API', fs: 't', ts: 'b', end: 'triangle' },
    { from: 'DX', to: 'CQ', fs: 'l', ts: 'r', fo: -9, to_off: 0, start: 'diamond', end: 'none', label: '소유' },
    { from: 'DX', to: 'STG', fs: 'r', ts: 'l', fo: -18, to_off: 0, start: 'diamond', end: 'none', label: '소유' },
    { points: [[470, 858], [470, 880], [930, 880], [930, 822]], start: 'diamond', end: 'none', label: '소유', lx: 700, ly: 880 }
  ]
};
