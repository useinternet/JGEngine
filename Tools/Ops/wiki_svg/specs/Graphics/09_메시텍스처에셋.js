// 7-1. PStaticMesh와 HRenderSubMesh — 렌더 메시(IMesh 구현)와 GPU 텍스처. CPU 사본 · 직렬화는 GameFrameWork 의 메시 · 텍스처 에셋이 맡고, 에셋이 이 GPU 자원을 생성한다
// 2026-10-24 모듈 정리(H-077 B) 뒤 다시 그림: 옛 그림(JGAsset · JGStaticMesh · JGTexture · FBX 가져오기)은 GameFrameWork 문서로 넘김
// 코드: Classes/StaticMesh.h/.cpp(PStaticMesh · HRenderSubMesh), Classes/Mesh.h(IMesh), Classes/Texture.h(IRawTexture), JGGraphicsAPI.h(CreateRawTexture · ReadbackTextureImmediate),
//       GameFrameWork/Asset/StaticMeshAsset.cpp(JGStaticMesh::GetMesh → SetSubMeshes), GameFrameWork/Asset/TextureAsset.cpp(JGTexture)
module.exports = {
  file: 'Graphics_09_메시텍스처에셋',
  alt: 'PStaticMesh와 HRenderSubMesh — PStaticMesh는 IMesh를 구현하고 서브메시(HRenderSubMesh)마다 GPU 버퍼와 머터리얼을 보관하며, GameFrameWork의 메시 에셋 JGStaticMesh가 GetMesh()에서 SetSubMeshes로 렌더 메시를 생성하고, 텍스처 에셋 JGTexture는 PJGGraphicsAPI로 생성한 IRawTexture를 소유하며 저장 시 리드백한다',
  type: 'class',
  w: 1100,
  h: 530,
  groups: [
    { x: 20, y: 20, w: 1060, h: 104, label: 'GameFrameWork (메시 · 텍스처 에셋)', dash: true }
  ],
  classes: [
    { id: 'PSM', x: 40, y: 190, w: 360, name: 'PStaticMesh', stereo: '렌더 메시 · IMesh 구현', color: 'blue', members: ['-_subMeshes : HRenderSubMesh[]', '+SetSubMeshes(subMeshes)', '+GetMaterial(slot)  // 비면 기본 머터리얼'] },
    { id: 'IMS', x: 420, y: 190, w: 270, name: 'IMesh', stereo: '렌더 메시 인터페이스', color: 'purple', members: ['+GetVertexBuffer() · GetIndexBuffer()', '+GetMaterial() · GetInputLayout()'] },
    { id: 'SUB', x: 40, y: 390, w: 360, name: 'HRenderSubMesh', stereo: '서브메시 · GPU 자원만', color: 'blue', stack: 1, members: ['VertexBuffer · IndexBuffer  // GPU 버퍼', 'Material'] },
    { id: 'RTX', x: 760, y: 330, w: 300, name: 'IRawTexture', stereo: 'GPU 텍스처', color: 'blue', members: ['+GetTextureID() · GetTextureInfo()'] }
  ],
  nodes: [
    { id: 'SMA', x: 40, y: 54, w: 360, h: 56, color: 'gray', title: 'JGStaticMesh', lines: ['메시 에셋 · CPU 사본 · 직렬화'] },
    { id: 'TXA', x: 760, y: 54, w: 300, h: 56, color: 'gray', title: 'JGTexture', lines: ['텍스처 에셋 · 직렬화'] },
    { id: 'GAPI', x: 760, y: 200, w: 300, h: 56, color: 'gray', title: 'PJGGraphicsAPI', lines: ['그래픽스 API'] },
    { id: 'KEY', x: 440, y: 410, w: 300, h: 80, color: 'white', shape: 'note', title: '범례', lines: ['◇— 소유 · —▷ 상속 · → 생성 · 사용', '겹친 카드 = 여러 개 · + public · - private'] }
  ],
  edges: [
    { from: 'SMA', to: 'PSM', fs: 'b', ts: 't', label: 'GetMesh() · SetSubMeshes' },
    { from: 'PSM', to: 'IMS', fs: 'r', ts: 'l', fo: -9, to_off: 0, end: 'triangle' },
    { from: 'PSM', to: 'SUB', fs: 'b', ts: 't', start: 'diamond', end: 'none', label: '1 : N' },
    { from: 'TXA', to: 'GAPI', fs: 'b', ts: 't', label: 'CreateRawTexture · 저장 시 리드백', lpos: 'l', lw: 260 },
    { from: 'GAPI', to: 'RTX', fs: 'b', ts: 't', label: '생성' },
    { points: [[760, 96], [735, 96], [735, 360], [760, 360]], start: 'diamond', end: 'none', label: '소유', lx: 735, ly: 300, lpos: 'l' }
  ]
};
