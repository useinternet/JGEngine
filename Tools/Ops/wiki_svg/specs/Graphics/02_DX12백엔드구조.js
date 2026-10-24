// 2-1. PDirectX12API와 PCommandQueue — DX12 백엔드가 소유하는 것, 커맨드 큐 → 프레임 컨텍스트 → 커맨드 리스트 → 상태 추적
// 코드: DirectX12/DirectX12API.h/.cpp(PDirectX12API · HDirectXAPI), DirectX12/Classes/CommandQueue.h/.cpp, CommandList.h, ResourceStateTracker.h, UploadAllocator.h, DynamicDescriptionAllocator.h
module.exports = {
  file: 'Graphics_02_DX12백엔드구조',
  alt: 'PDirectX12API와 PCommandQueue — PDirectX12API는 장치, 커맨드 큐, 리소스 전송 관리자, 디스크립터 할당자, PSO · 루트 서명 캐시, 프레임 버퍼를 소유하고 DX12 내부 클래스는 정적 클래스 HDirectXAPI로 접근하며, PCommandQueue는 프레임 인덱스별 HFrameContext에 우선순위별 PCommandList를 보관하고, PCommandList는 PResourceStateTracker와 업로드 · 디스크립터 할당자를 가진다',
  type: 'class',
  w: 1000,
  h: 740,
  classes: [
    { id: 'DX', x: 40, y: 30, w: 380, name: 'PDirectX12API', stereo: 'DX12 백엔드 · PJGGraphicsAPI 구현', color: 'yellow', members: ['-_dx12Device · _dx12Factory', '-_commandQueue : PCommandQueue', '-_resourceStagingManager', '-_csuAllocator · _rtvAllocator · _dsvAllocator', '-_rootSignatureCache · _graphicsPSOCache', '-_frameBuffer : PDX12FrameBuffer', '+CreateCommittedResource()', '+DestroyCommittedResource()  // 미룬 해제', '+RequestGraphicsCommandList()'] },
    { id: 'HDX', x: 40, y: 300, w: 380, name: 'HDirectXAPI', stereo: '정적 접근 클래스 · DX12 내부 클래스가 사용', color: 'yellow', members: ['+GetDevice() · GetCommandQueue()', '+RequestGraphicsCommandList()', '+DestroyCommittedResource()'] },
    { id: 'CQ', x: 560, y: 30, w: 400, name: 'PCommandQueue', stereo: '커맨드 큐 · 프레임 제출', color: 'blue', members: ['-_frames : HFrameContext[]', '-_deferredReleases : 미룬 해제 목록', '+RequestCommandList(type, priority)', '+Begin()  // 같은 인덱스의 이전 프레임 완료 대기', '+End()  // 우선순위 순 제출 · Signal', '+DeferRelease(resource)'] },
    { id: 'FC', x: 560, y: 250, w: 400, name: 'HFrameContext', stereo: '프레임 인덱스별 컨텍스트', color: 'blue', stack: 1, members: ['우선순위별 커맨드 리스트 · 펜스 값'] },
    { id: 'CL', x: 560, y: 400, w: 400, name: 'PCommandList', stereo: '커맨드 리스트', color: 'blue', stack: 2, members: ['-_dx12CommandList · _dx12Allocator', '-_resourceStateTracker : PResourceStateTracker', '-_uploadAllocator : PUploadAllocator', '-_dynamicDescriptionAllocator', '+TransitionBarrier() · CopyResource()'] },
    { id: 'GCL', x: 560, y: 620, w: 400, name: 'PGraphicsCommandList', stereo: '그래픽스 커맨드 리스트', color: 'blue', members: ['+BindRootSignature() · BindPipelineState()', '+BindVertexBuffer() · DrawIndexed()'] },
    { id: 'RST', x: 40, y: 460, w: 380, name: 'PResourceStateTracker', stereo: '리소스 상태 추적', color: 'purple', members: ['+TransitionBarrier()  // 리스트별 배리어', '+RegisterResource()  // 전역 상태 맵 (static)'] }
  ],
  nodes: [
    { id: 'KEY', x: 40, y: 610, w: 380, h: 96, color: 'white', shape: 'note', title: '범례', lines: ['◇— 소유 · —▷ 상속 · → 접근', '겹친 카드 = 여러 개', '+ public · - private'] }
  ],
  edges: [
    { from: 'DX', to: 'CQ', fs: 'r', ts: 'l', fo: 0, to_off: 27, start: 'diamond', end: 'none', label: '소유' },
    { from: 'HDX', to: 'DX', fs: 't', ts: 'b', label: '접근' },
    { from: 'CQ', to: 'FC', fs: 'b', ts: 't', start: 'diamond', end: 'none', label: '1 : N' },
    { points: [[760, 328], [760, 400]], start: 'diamond', end: 'none', label: '1 : N' },
    { points: [[760, 620], [760, 556]], end: 'triangle' },
    { from: 'CL', to: 'RST', fs: 'l', ts: 'r', fo: 0, to_off: -33, start: 'diamond', end: 'none', label: '소유' }
  ]
};
