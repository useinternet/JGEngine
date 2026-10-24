// 0-1. GAssetDatabase와 HAssetPath — 위 = 등록 조건(Graphics가 연결된 프로세스에서만), 아래 = 옛 Asset_00 의 로드 흐름(Start → 두 Content → 비동기 로드 → 메인 스레드 등록)
// 코드: Core/GameFrameWorkModule.cpp(StartupModule — FindModule("Graphics") 일 때 RegisterSystemInstance<GAssetDatabase>, ShutdownModule — 게임 인스턴스 뒤 해제),
//       Asset/AssetDatabase.h · .cpp(Start · loadAssets · LoadAssetAsync · Update · releaseAssets), Asset/AssetPath.h · AssetDefines.h(/JGEngine/ · /JGGame/ · .jgasset)
module.exports = {
  file: 'GameFrameWork_Asset_00_에셋데이터베이스',
  alt: 'GAssetDatabase와 HAssetPath — GameFrameWork 모듈은 Graphics 모듈이 연결된 프로세스(에디터 · GameHost)에서만 GAssetDatabase를 등록하고, Start()가 엔진 Content와 게임 Content의 모든 .jgasset을 LoadAssetAsync로 로드 스레드에서 읽은 뒤 메인 스레드 Update()에서 GUID 풀에 등록 · 콜백하며, 같은 GUID가 이미 있으면 나중 에셋을 건너뛰고, 에셋 경로는 /JGEngine/ · /JGGame/ 토큰으로 두 Content를 가리킨다',
  type: 'flow',
  w: 980,
  h: 860,
  nodes: [
    { id: 'M', x: 20, y: 20, w: 300, h: 56, color: 'blue', title: 'HGameFrameWorkModule', lines: ['StartupModule'] },
    { id: 'Q', x: 40, y: 110, w: 260, h: 90, color: 'white', shape: 'diamond', title: 'Graphics 모듈이', lines: ['연결됐는가'] },
    { id: 'NO', x: 370, y: 127, w: 300, h: 56, color: 'gray', title: '에셋 DB 없음', lines: ['JGConsole — 쿠킹 검사는 파일을 직접 읽음'] },
    { id: 'REG', x: 20, y: 240, w: 300, h: 56, color: 'purple', title: 'GAssetDatabase 등록', lines: ['에디터 · GameHost'] },
    { id: 'S', x: 20, y: 408, w: 170, h: 56, color: 'blue', title: 'Start()', lines: ['loadAssets 호출'] },
    { id: 'E', x: 230, y: 360, w: 210, h: 56, color: 'yellow', title: '엔진 Content', lines: ['*.jgasset 전체'] },
    { id: 'G', x: 230, y: 450, w: 210, h: 70, color: 'yellow', title: '게임 Content', lines: ['*.jgasset 전체', '프로젝트일 때'] },
    { id: 'L', x: 490, y: 408, w: 220, h: 56, color: 'purple', title: 'LoadAssetAsync', lines: ['로드 스레드에서 LoadObject'] },
    { id: 'U', x: 490, y: 510, w: 220, h: 66, color: 'blue', title: 'Update()', lines: ['메인 스레드', 'GUID 풀 등록 · 콜백 호출'] },
    { id: 'D', x: 520, y: 620, w: 160, h: 90, color: 'white', shape: 'diamond', title: '같은 GUID가 이미 있는가' },
    { id: 'R', x: 230, y: 637, w: 230, h: 56, color: 'red', title: '중복 GUID 건너뜀', lines: ['나중 에셋 · Error 로그'] },
    { id: 'P', x: 490, y: 760, w: 220, h: 66, color: 'green', title: 'GUID 풀', lines: ['GUID 키로 소유', 'GetLoadedAsset(경로) → 약참조'] },
    { id: 'PATH', x: 750, y: 360, w: 210, h: 124, color: 'white', shape: 'note', title: 'HAssetPath', lines: ['/JGEngine/… → 엔진 Content', '/JGGame/… → 게임 Content', '확장자 .jgasset'], small: true }
  ],
  edges: [
    { from: 'M', to: 'Q' },
    { from: 'Q', to: 'NO', fs: 'r', ts: 'l', label: '아니오' },
    { from: 'Q', to: 'REG', fs: 'b', ts: 't', label: '예' },
    { from: 'REG', to: 'S', fs: 'b', ts: 't', fo: -65, to_off: 0, label: '시스템 Start' },
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
