// 1-1. JGStaticMesh, JGTexture (메시 · 텍스처 에셋) — 위 = 기반(임포터 · 에셋), 가운데 = FBX 임포터 · 메시 · 텍스처 에셋, 아래 = 에셋이 만드는 Graphics GPU 자원
// 옛 Graphics 문서 7절(메시 · 텍스처 에셋)이 2026-10-24 Asset 이전(H-077 B)으로 이 문서로 옴. Graphics 쪽 렌더 메시는 Graphics_09
// 코드: Asset/Asset.h(JGAsset), Asset/StaticMeshAsset.h · .cpp(HStaticSubMesh · JGStaticMesh · Create · GetMesh → SetSubMeshes), Asset/TextureAsset.h · .cpp(JGTexture · Create),
//       Asset/AssetImporter.h, Asset/Importers/FBXAssetImporter.h · .cpp(assimp)
module.exports = {
  file: 'GameFrameWork_Asset_01_메시텍스처에셋',
  alt: 'JGStaticMesh와 JGTexture — 두 에셋은 JGAsset을 상속해 JSON 형식의 .jgasset으로 저장되고, JGStaticMesh는 서브메시(HStaticSubMesh)마다 CPU 사본과 GPU 버퍼를 보관하며 GetMesh()로 Graphics의 렌더 메시 PStaticMesh를 생성하고, JGTexture는 PJGGraphicsAPI로 생성한 GPU 텍스처를 소유해 저장 시 리드백하며, JGAssetImporter를 상속한 JGFBXAssetImporter가 FBX 파일에서 메시 에셋과 임베디드 텍스처 에셋을 생성한다',
  type: 'class',
  w: 1100,
  h: 660,
  groups: [
    { x: 380, y: 490, w: 700, h: 150, label: 'Graphics', dash: true }
  ],
  classes: [
    { id: 'IMP', x: 40, y: 30, w: 300, name: 'JGAssetImporter', stereo: '에셋 임포터 기반', color: 'gray', members: ['+Import(args)'] },
    { id: 'AST', x: 560, y: 30, w: 300, name: 'JGAsset', stereo: '에셋 기반 · .jgasset', color: 'gray', members: ['_guid · AssetPath · Version', '#OnLoadAsset_Thread()'] },
    { id: 'FBX', x: 40, y: 220, w: 300, name: 'JGFBXAssetImporter', stereo: 'FBX 임포터 · assimp', color: 'purple', members: ['+Import(args)', '메시 · 임베디드 텍스처 저장'] },
    { id: 'SM', x: 400, y: 220, w: 300, name: 'JGStaticMesh', stereo: '정적 메시 에셋', color: 'purple', members: ['-_subMeshes : HStaticSubMesh[]', '+Create(args)  // 정적', '+GetMesh() · GetBounds()'] },
    { id: 'TX', x: 760, y: 220, w: 300, name: 'JGTexture', stereo: '텍스처 에셋', color: 'purple', members: ['-_rawTexture : IRawTexture', '+Create(args)  // 정적', '+GetRawTexture()'] }
  ],
  nodes: [
    { id: 'PSM', x: 400, y: 530, w: 300, h: 70, color: 'blue', title: 'PStaticMesh', lines: ['렌더 메시 · IMesh 구현', 'GPU 버퍼만 참조'] },
    { id: 'API', x: 760, y: 530, w: 300, h: 70, color: 'blue', title: 'PJGGraphicsAPI', lines: ['GPU 버퍼 · 텍스처 생성', '텍스처 리드백'] },
    { id: 'KEY', x: 40, y: 490, w: 300, h: 96, color: 'white', shape: 'note', title: '범례', lines: ['—▷ 상속 · → 생성 · 사용', '+ public · # protected · - private'] }
  ],
  edges: [
    { from: 'FBX', to: 'IMP', fs: 't', ts: 'b', end: 'triangle' },
    { from: 'SM', to: 'AST', fs: 't', ts: 'b', fo: 0, to_off: -100, end: 'triangle' },
    { from: 'TX', to: 'AST', fs: 't', ts: 'b', fo: 0, to_off: 100, end: 'triangle' },
    { points: [[340, 280], [400, 280]], label: '생성' },
    { points: [[190, 310], [190, 440], [960, 440], [960, 328]], label: '임베디드 텍스처 생성', lx: 450, ly: 440 },
    { from: 'SM', to: 'PSM', fs: 'b', ts: 't', label: 'GetMesh()', lx: 550, ly: 400 },
    { from: 'TX', to: 'API', fs: 'b', ts: 't', fo: -80, to_off: -80, label: 'Create · 저장 시 리드백', lx: 880, ly: 480, lpos: 'l' }
  ]
};
