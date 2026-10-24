// 생성 흐름 — 손으로 쓰는 것(소스 · module.json)에서 PreBuild 가 리플렉션 코드와 프로젝트 파일을 만들고, MSBuild 가 Bin 에 빌드한다
module.exports = {
  file: 'JGEngine프로젝트구성도_01_생성흐름',
  alt: '생성 흐름 — PreBuild는 빌드 도구가 없으면 BootstrapTools로 먼저 빌드하고, JGHeaderTool이 리플렉션 코드를 Temp/CodeGen에, JGBuildTool이 module.json으로 jgengine.lua와 JGEngine.sln을 만들며, MSBuild가 Bin/DevelopEngine에 빌드한다',
  type: 'flow',
  w: 1000,
  h: 500,
  nodes: [
    { id: 'HAND', x: 40, y: 60, w: 260, h: 52, color: 'blue', title: '손으로 쓰는 것', lines: ['소스 · *.module.json · Content'] },
    { id: 'PRE', x: 380, y: 60, w: 240, h: 52, color: 'gray', title: 'PreBuild.bat', lines: ['GenerateProjectFiles.bat이 호출'] },
    { id: 'BOOT', x: 700, y: 60, w: 260, h: 52, color: 'gray', title: 'BootstrapTools.bat', lines: ['빌드 도구가 없을 때 소스에서 빌드'] },
    { id: 'HT', x: 380, y: 180, w: 240, h: 52, color: 'gray', title: 'JGHeaderTool', lines: ['JGCLASS · JGPROPERTY 해석'] },
    { id: 'CG', x: 700, y: 180, w: 260, h: 52, color: 'orange', title: 'Temp/CodeGen', lines: ['*.generation.h · 모듈 정보'] },
    { id: 'BT', x: 380, y: 300, w: 240, h: 52, color: 'gray', title: 'JGBuildTool', lines: ['module.json → jgengine.lua'] },
    { id: 'SLN', x: 700, y: 300, w: 260, h: 52, color: 'yellow', title: 'JGEngine.sln', lines: ['premake가 만든 솔루션 · 프로젝트'] },
    { id: 'MSB', x: 380, y: 420, w: 240, h: 52, color: 'gray', title: 'MSBuild', lines: ['Visual Studio 2022'] },
    { id: 'BIN', x: 700, y: 420, w: 260, h: 52, color: 'orange', title: 'Bin/DevelopEngine', lines: ['모듈 DLL · 실행 파일'] }
  ],
  edges: [
    { from: 'HAND', to: 'PRE', points: [[300, 86], [380, 86]] },
    { from: 'PRE', to: 'BOOT', points: [[620, 86], [700, 86]], dash: true, label: '도구 없음' },
    { from: 'PRE', to: 'HT', points: [[500, 112], [500, 180]] },
    { from: 'HT', to: 'CG', points: [[620, 206], [700, 206]] },
    { from: 'HT', to: 'BT', points: [[500, 232], [500, 300]] },
    { from: 'BT', to: 'SLN', points: [[620, 326], [700, 326]] },
    { from: 'SLN', to: 'MSB', points: [[830, 352], [830, 386], [500, 386], [500, 420]] },
    { from: 'MSB', to: 'BIN', points: [[620, 446], [700, 446]] }
  ]
};
