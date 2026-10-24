// 엔진과 게임 프로젝트 — 게임 프로젝트는 엔진 밖 폴더(.jgproject 의 EngineRoot)이고, 게임 모듈은 엔진 모듈에 의존한다.
// Content 는 토큰 /JGEngine/ · /JGGame/ 로 구분되고, 쿠킹이 두 Content 를 패키지로 모은다.
module.exports = {
  file: 'JGEngine전체구조_02_엔진과게임프로젝트',
  alt: '엔진과 게임 프로젝트 — 엔진 도구가 만든 게임 프로젝트는 .jgproject로 엔진을 가리키고, 게임 모듈은 GameFrameWork에, 게임 에디터 모듈은 GUI와 게임 모듈에 의존하며, 엔진 Content(/JGEngine/)와 게임 Content(/JGGame/)는 쿠킹으로 패키지에 모인다',
  type: 'flow',
  w: 980,
  h: 560,
  groups: [
    { x: 20, y: 40, w: 400, h: 340, label: '엔진(JGEngine)' },
    { x: 520, y: 40, w: 440, h: 340, label: '게임 프로젝트(<Name>)' }
  ],
  nodes: [
    { id: 'ETOOL', x: 40, y: 80, w: 360, h: 52, color: 'gray', title: 'CreateGameProject · JGBuildTool', lines: ['게임 프로젝트 · 프로젝트 파일 생성'] },
    { id: 'EMOD', x: 40, y: 180, w: 360, h: 52, color: 'blue', title: '엔진 모듈', lines: ['Core · Graphics · GUI · GameFrameWork'] },
    { id: 'ECONT', x: 40, y: 290, w: 360, h: 52, color: 'green', title: 'Content', lines: ['/JGEngine/ — 엔진 셰이더 · 에셋'] },
    { id: 'GPROJ', x: 540, y: 80, w: 400, h: 52, color: 'yellow', title: '<Name>.jgproject', lines: ['EngineRoot · GameModules · EditorModules · Package'] },
    { id: 'GMOD', x: 540, y: 180, w: 190, h: 70, color: 'blue', title: 'Source/<Name>', lines: ['게임 모듈', 'GameFrameWork에 의존'] },
    { id: 'GED', x: 750, y: 180, w: 190, h: 70, color: 'purple', title: 'Source/<Name>Editor', lines: ['게임 에디터 모듈', 'GUI · 게임 모듈에 의존'] },
    { id: 'GCONT', x: 540, y: 290, w: 400, h: 52, color: 'green', title: 'Content', lines: ['/JGGame/ — 게임 에셋 · 데이터 테이블'] },
    { id: 'PKG', x: 220, y: 460, w: 540, h: 52, color: 'orange', title: 'Package/ReleaseGame', lines: ['<Name>.exe · Bin · Content/JGEngine · Content/JGGame'] }
  ],
  edges: [
    { from: 'ETOOL', to: 'GPROJ', points: [[400, 106], [540, 106]], label: '생성' },
    { from: 'GMOD', to: 'EMOD', points: [[540, 206], [400, 206]], label: '의존' },
    { from: 'ECONT', to: 'PKG', points: [[300, 342], [300, 460]], label: '쿠킹' },
    { from: 'GCONT', to: 'PKG', points: [[680, 342], [680, 460]], label: '쿠킹' }
  ]
};
