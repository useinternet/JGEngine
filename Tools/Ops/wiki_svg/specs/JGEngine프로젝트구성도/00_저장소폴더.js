// 저장소 폴더 — git 에 두는 폴더와 git 밖(빌드 결과 · 작업 폴더) 폴더
module.exports = {
  file: 'JGEngine프로젝트구성도_00_저장소폴더',
  alt: '저장소 폴더 — git에는 Source · Content · Build · Tools · ThirdParty · 생성 프로젝트 파일 · CLAUDE.md를 두고, 빌드 결과 Bin과 작업 폴더 Temp는 git 밖에 둔다',
  type: 'flow',
  w: 1000,
  h: 500,
  groups: [
    { x: 20, y: 40, w: 660, h: 440, label: 'git 안' },
    { x: 700, y: 40, w: 280, h: 440, label: 'git 밖' }
  ],
  nodes: [
    { id: 'SRC', x: 40, y: 80, w: 300, h: 70, color: 'blue', title: 'Source', lines: ['Runtime · Editor · Programs', 'ThirdParty · PCH'] },
    { id: 'CONTENT', x: 360, y: 80, w: 300, h: 70, color: 'green', title: 'Content', lines: ['엔진 Content(/JGEngine/)', 'Shaders · TempAsset · RawResources'] },
    { id: 'BUILD', x: 40, y: 190, w: 300, h: 70, color: 'gray', title: 'Build', lines: ['BatchFiles · Templates', 'GitHooks · premake5'] },
    { id: 'TOOLS', x: 360, y: 190, w: 300, h: 70, color: 'gray', title: 'Tools', lines: ['검증 · 운영 도구', 'Common · Ops · 분야별'] },
    { id: 'TP', x: 40, y: 300, w: 300, h: 52, color: 'gray', title: 'ThirdParty', lines: ['미리 빌드한 assimp · zlib'] },
    { id: 'GEN', x: 360, y: 300, w: 300, h: 70, color: 'yellow', title: 'JGEngine.sln · jgengine.lua', lines: ['PreBuild가 만드는 파일', 'git에 둔다'] },
    { id: 'CLAUDE', x: 40, y: 400, w: 300, h: 52, color: 'white', title: 'CLAUDE.md', lines: ['팀 지침'] },
    { id: 'BIN', x: 720, y: 80, w: 240, h: 70, color: 'orange', title: 'Bin', lines: ['DevelopEngine · DevelopGame', 'ConfirmGame · ReleaseGame'] },
    { id: 'TEMP', x: 720, y: 190, w: 240, h: 88, color: 'orange', title: 'Temp', lines: ['CodeGen · ProjectFiles', 'wt — 떼어 둔 작업 폴더', 'Games — 검증 게임'] }
  ],
  edges: []
};
