// 7-1. JGStaticMesh와 JGTexture — 에셋(JGAsset, 직렬화)과 렌더용 GPU 리소스(PStaticMesh · IRawTexture)의 분리, FBX 임포터가 두 에셋을 생성
// 코드: Classes/StaticMesh.h/.cpp(JGStaticMesh · HStaticSubMesh · PStaticMesh · HRenderSubMesh), Classes/Texture.h/.cpp(JGTexture), Importers/FBXAssetImporter.h/.cpp, Asset/Asset.h(JGAsset) · AssetImporter.h
module.exports = {
  file: 'Graphics_09_메시텍스처에셋',
  alt: 'JGStaticMesh와 JGTexture — 두 에셋은 JGAsset을 상속해 .jgasset으로 직렬화되고, JGStaticMesh는 서브메시(HStaticSubMesh)마다 CPU 사본과 GPU 버퍼를 보관하며 GetMesh()로 렌더 메시 PStaticMesh를 생성하고, JGTexture는 GPU 텍스처 IRawTexture를 소유하며, JGFBXAssetImporter가 FBX 파일에서 두 에셋을 생성한다',
  type: 'class',
  w: 1100,
  h: 600,
  classes: [
    { id: 'AST', x: 300, y: 30, w: 500, name: 'JGAsset', stereo: '에셋 기반 클래스 · Asset 모듈', color: 'purple', members: ['리플렉션 직렬화 → .jgasset (JSON)'] },
    { id: 'SM', x: 40, y: 170, w: 400, name: 'JGStaticMesh', stereo: '정적 메시 에셋', color: 'green', members: ['JGPROPERTY _subMeshes : HStaticSubMesh[]', 'JGPROPERTY _bounds  // 경계 상자', '-_mesh : PStaticMesh', '+SetData() · SetMaterial()', '+GetMesh()  // 렌더 메시 지연 생성'] },
    { id: 'IMP', x: 480, y: 170, w: 260, name: 'JGFBXAssetImporter', stereo: 'FBX 임포터 · JGAssetImporter 상속', color: 'yellow', members: ['+Import(args)'] },
    { id: 'TEX', x: 780, y: 170, w: 280, name: 'JGTexture', stereo: '텍스처 에셋', color: 'green', members: ['-_texture : IRawTexture', '+GetRawTexture()'] },
    { id: 'SUB', x: 40, y: 380, w: 400, name: 'HStaticSubMesh', stereo: '서브메시 · 직렬화 대상', color: 'green', stack: 1, members: ['Vertices · Indices  // CPU 사본', 'VertexBuffer · IndexBuffer  // GPU 버퍼', 'Material'] },
    { id: 'PSM', x: 480, y: 380, w: 260, name: 'PStaticMesh', stereo: '렌더 메시 · IMesh 구현', color: 'blue', members: ['HRenderSubMesh[]  // GPU 버퍼 · 머터리얼'] },
    { id: 'RT', x: 780, y: 380, w: 280, name: 'IRawTexture', stereo: 'GPU 텍스처', color: 'blue' }
  ],
  nodes: [
    { id: 'KEY', x: 480, y: 500, w: 580, h: 80, color: 'white', shape: 'note', title: '범례', lines: ['◇— 소유 · —▷ 상속 · → 생성', '겹친 카드 = 여러 개 · + public · - private'] }
  ],
  edges: [
    { from: 'SM', to: 'AST', fs: 't', ts: 'b', fo: 110, to_off: -200, end: 'triangle' },
    { from: 'TEX', to: 'AST', fs: 't', ts: 'b', fo: -130, to_off: 240, end: 'triangle' },
    { from: 'IMP', to: 'SM', fs: 'l', ts: 'r', fo: 0, to_off: -36, label: '생성' },
    { from: 'IMP', to: 'TEX', fs: 'r', ts: 'l', fo: 0, to_off: -9, label: '생성' },
    { from: 'SM', to: 'SUB', fs: 'b', ts: 't', start: 'diamond', end: 'none', label: '1 : N' },
    { points: [[440, 290], [610, 290], [610, 380]], label: 'GetMesh()로 생성', lx: 610, ly: 335 },
    { from: 'TEX', to: 'RT', fs: 'b', ts: 't', start: 'diamond', end: 'none', label: '소유' }
  ]
};
