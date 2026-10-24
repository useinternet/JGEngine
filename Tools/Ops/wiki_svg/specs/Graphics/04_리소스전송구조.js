// 3-1. PResourceStagingManager와 전송 요청 — 요청은 아무 때나 대기 목록에, 기록은 EndFrame의 정해진 우선순위 리스트에
// 코드: DirectX12/Classes/ResourceStagingManager.h/.cpp, CommandQueue.h(ECommandListPriority · ExecuteImmediate), DX12Texture.cpp · DX12VertexBuffer.cpp(업로드 요청), Classes/Texture.cpp(WriteJson 리드백)
module.exports = {
  file: 'Graphics_04_리소스전송구조',
  alt: 'PResourceStagingManager와 전송 요청 — GPU 리소스 생성과 리드백 사용처가 요청하면 PResourceStagingManager가 HUploadRequest · HReadbackRequest로 보관하고, EndFrame에서 업로드는 드로우 전에 실행되는 Upload 리스트, 리드백은 드로우 뒤의 Readback 리스트에 기록하며, 즉시 리드백은 전용 커맨드 리스트를 제출하고 GPU 완료를 기다린다',
  type: 'class',
  w: 1180,
  h: 520,
  classes: [
    { id: 'STG', x: 340, y: 150, w: 440, name: 'PResourceStagingManager', stereo: '리소스 전송 관리자 · PDirectX12API 소유', color: 'yellow', members: ['-_pendingUploads : HUploadRequest[]', '-_pendingReadbacks · _submittedReadbacks', '+RequestBufferUpload() · RequestTextureUpload()', '+RequestTextureReadback(texture, onComplete)', '+ReadbackTextureImmediate(texture, out)', '+BeginFrame()  // 끝난 리드백의 콜백 호출', '+RecordUploads() · RecordReadbacks()  // EndFrame'] },
    { id: 'UP', x: 20, y: 150, w: 280, name: 'HUploadRequest', stereo: '업로드 요청', color: 'blue', stack: 1, members: ['Dest : 대상 리소스', 'Staging : UPLOAD 힙 버퍼'] },
    { id: 'RB', x: 820, y: 150, w: 340, name: 'HReadbackRequest', stereo: '리드백 요청', color: 'blue', stack: 1, members: ['Texture · Staging : READBACK 힙 버퍼', 'OnComplete : 완료 콜백', 'FenceValue : 제출 펜스 값'] },
    { id: 'L1', x: 240, y: 410, w: 260, name: 'PCommandList', stereo: 'Upload 우선순위', color: 'purple', members: ['드로우보다 먼저 실행'] },
    { id: 'L2', x: 530, y: 410, w: 240, name: 'PCommandList', stereo: 'Readback 우선순위', color: 'purple', members: ['드로우 뒤에 실행'] },
    { id: 'L3', x: 820, y: 410, w: 300, name: '전용 PCommandList', stereo: '즉시 리드백', color: 'purple', members: ['제출 후 GPU 완료 대기'] }
  ],
  nodes: [
    { id: 'C1', x: 340, y: 30, w: 230, h: 60, color: 'gray', title: 'GPU 리소스 생성', lines: ['PDX12Texture · 정점 버퍼 등'] },
    { id: 'C2', x: 590, y: 30, w: 190, h: 60, color: 'gray', title: '리드백 사용처', lines: ['텍스처 에셋 저장 · 렌더 자체 시험'] },
    { id: 'KEY', x: 20, y: 300, w: 280, h: 80, color: 'white', shape: 'note', title: '범례', lines: ['◇— 보관 · → 요청 · 기록', '겹친 카드 = 여러 개'] }
  ],
  edges: [
    { from: 'C1', to: 'STG', fs: 'b', ts: 't', fo: 0, to_off: -105, label: '업로드 요청' },
    { from: 'C2', to: 'STG', fs: 'b', ts: 't', fo: 0, to_off: 125, label: '리드백 요청' },
    { from: 'STG', to: 'UP', fs: 'l', ts: 'r', fo: -45, to_off: 0, start: 'diamond', end: 'none', label: '1 : N' },
    { from: 'STG', to: 'RB', fs: 'r', ts: 'l', fo: -36, to_off: 0, start: 'diamond', end: 'none', label: '1 : N' },
    { from: 'STG', to: 'L1', fs: 'b', ts: 't', fo: -190, to_off: 0, label: 'RecordUploads' },
    { from: 'STG', to: 'L2', fs: 'b', ts: 't', fo: 90, to_off: 0, label: 'RecordReadbacks' },
    { points: [[780, 300], [970, 300], [970, 410]], label: 'ExecuteImmediate', lx: 970, ly: 355 }
  ]
};
