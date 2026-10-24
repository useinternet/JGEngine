// 1. 전체 흐름
// 위 줄 = 프로젝트 파일 생성(2번), 아래 줄 = 빌드(5번) · 실행(6번). premake5 → JGEngine.sln 은 두 줄 사이로 내려온다.
// 색: 노랑 = 손으로 고치는 원본, 보라 = 엔진 도구, 회색 = 외부 도구, 파랑 = 생성물, 초록 = 빌드 결과
module.exports = {
  file: 'JGEngine빌드와실행_00_전체흐름',
  alt: '전체 흐름 — 모듈별 module.json과 헤더의 JGCLASS 등을 PreBuild.bat이 받아 JGHeaderTool(Temp/CodeGen 생성)과 JGBuildTool(jgengine.lua · jgengine.bat 생성)을 실행하고, premake5 vs2022가 JGEngine.sln과 vcxproj를 생성하면 MSBuild · Visual Studio가 Bin/<구성>/에 dll · exe를 빌드하고 JGLauncher.exe가 JGEditor를 실행한다',
  type: 'flow',
  w: 1000,
  h: 410,
  groups: [
    { x: 20, y: 20, w: 960, h: 210, label: '프로젝트 파일 생성 (2번)' },
    { x: 20, y: 280, w: 960, h: 110, label: '빌드 (5번) · 실행 (6번)' }
  ],
  nodes: [
    { id: 'MJ', x: 40, y: 62, w: 200, h: 56, color: 'yellow', title: '*.module.json', lines: ['모듈마다 하나 — 목록 · 의존 원본'] },
    { id: 'H', x: 40, y: 152, w: 200, h: 56, color: 'yellow', title: 'JGCLASS 등', lines: ['헤더의 리플렉션 매크로'] },
    { id: 'PB', x: 290, y: 107, w: 170, h: 56, color: 'purple', title: 'PreBuild.bat', lines: ['두 도구를 차례로 실행'] },
    { id: 'HT', x: 510, y: 62, w: 230, h: 56, color: 'purple', title: 'JGHeaderTool', lines: ['리플렉션 코드 → Temp/CodeGen'] },
    { id: 'BT', x: 510, y: 152, w: 230, h: 56, color: 'purple', title: 'JGBuildTool', lines: ['jgengine.lua · jgengine.bat 생성'] },
    { id: 'PM', x: 790, y: 152, w: 170, h: 56, color: 'gray', title: 'premake5', lines: ['vs2022 프로젝트 생성'] },
    { id: 'SLN', x: 40, y: 318, w: 230, h: 56, color: 'blue', title: 'JGEngine.sln', lines: ['+ Temp/ProjectFiles/*.vcxproj'] },
    { id: 'MS', x: 320, y: 318, w: 180, h: 56, color: 'gray', title: 'MSBuild', lines: ['또는 Visual Studio'] },
    { id: 'BIN', x: 550, y: 318, w: 180, h: 56, color: 'green', title: 'Bin/<구성>/', lines: ['*.dll · *.exe'] },
    { id: 'RUN', x: 780, y: 318, w: 180, h: 56, color: 'blue', title: 'JGLauncher.exe', lines: ['JGEditor 실행'] }
  ],
  edges: [
    { from: 'MJ', to: 'PB', fs: 'r', ts: 'l', to_off: -10 },
    { from: 'H', to: 'PB', fs: 'r', ts: 'l', to_off: 10 },
    { from: 'PB', to: 'HT', fs: 'r', ts: 'l', fo: -10 },
    { from: 'PB', to: 'BT', fs: 'r', ts: 'l', fo: 10 },
    { from: 'BT', to: 'PM', fs: 'r', ts: 'l' },
    { from: 'PM', to: 'SLN', fs: 'b', ts: 't', to_off: 60, my: 255 },
    { from: 'SLN', to: 'MS', fs: 'r', ts: 'l' },
    { from: 'MS', to: 'BIN', fs: 'r', ts: 'l' },
    { from: 'BIN', to: 'RUN', fs: 'r', ts: 'l' }
  ]
};
