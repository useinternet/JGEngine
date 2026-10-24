// 에셋 데이터베이스 GAssetDatabase — 윗줄은 Start → 두 Content → 비동기 로드(왼쪽 → 오른쪽),
// 로드 뒤는 오른쪽 세로줄로 내려가 갈림길에서 "예"(중복)는 왼쪽, "아니오"(정상)는 아래로.
module.exports = {
  file: 'Asset_00_에셋데이터베이스',
  alt: '에셋 데이터베이스 GAssetDatabase — Start()가 엔진 Content와 게임 Content(프로젝트일 때)의 모든 .jgasset을 LoadAssetAsync로 AssetLoadThread에서 로드하고, 메인 스레드 Update()에서 GUID 풀에 등록 · 콜백하며, 같은 GUID가 이미 있으면 나중 에셋을 Error 로그와 함께 건너뛴다',
  type: 'flow',
  w: 730,
  h: 520,
  nodes: [
    { id: 'S', x: 20, y: 78, w: 170, h: 56, color: 'blue', title: 'Start()', lines: ['loadAssets 호출'] },
    { id: 'E', x: 230, y: 30, w: 210, h: 56, color: 'yellow', title: '엔진 Content', lines: ['*.jgasset 전체'] },
    { id: 'G', x: 230, y: 120, w: 210, h: 70, color: 'yellow', title: '게임 Content', lines: ['*.jgasset 전체', '프로젝트일 때'] },
    { id: 'L', x: 490, y: 78, w: 220, h: 56, color: 'purple', title: 'LoadAssetAsync', lines: ['AssetLoadThread에서 LoadObject'] },
    { id: 'U', x: 490, y: 180, w: 220, h: 66, color: 'blue', title: 'Update()', lines: ['메인 스레드', 'GUID 풀 등록 · 콜백 호출'] },
    { id: 'D', x: 520, y: 290, w: 160, h: 90, color: 'white', shape: 'diamond', title: '같은 GUID가 이미 있는가' },
    { id: 'R', x: 230, y: 307, w: 230, h: 56, color: 'red', title: '중복 GUID 건너뜀', lines: ['나중 에셋 Error 로그 (GM-A1)'] },
    { id: 'P', x: 490, y: 430, w: 220, h: 66, color: 'green', title: '_assetPool[guid]', lines: ['GUID 키로 소유', '경로 → 약참조'] }
  ],
  edges: [
    { from: 'S', to: 'E', fs: 'r', ts: 'l', mx: 210 },
    { from: 'S', to: 'G', fs: 'r', ts: 'l', mx: 210 },
    { from: 'E', to: 'L', fs: 'r', ts: 'l', mx: 465 },
    { from: 'G', to: 'L', fs: 'r', ts: 'l', mx: 465 },
    { from: 'L', to: 'U', fs: 'b', ts: 't' },
    { from: 'U', to: 'D', fs: 'b', ts: 't' },
    { from: 'D', to: 'R', fs: 'l', ts: 'r', label: '예' },
    { from: 'D', to: 'P', fs: 'b', ts: 't', label: '아니오' }
  ]
};
