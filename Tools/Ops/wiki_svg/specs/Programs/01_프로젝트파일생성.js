// 1-1. 프로젝트 파일 생성 과정 — GenerateProjectFiles.bat → PreBuild.bat(툴이 없으면 BootstrapTools.bat 먼저) → JGHeaderTool · JGBuildTool → 생성물
// 코드: GenerateProjectFiles.bat, Build/BatchFiles/PreBuild.bat · BootstrapTools.bat, JGBuildTool/Class/BuildTool.cpp(Run)
// 색: 흰색 = 배치, 보라 = 엔진 프로그램, 회색 = 외부 도구, 파랑 = 생성물, 초록 = 빌드 출력 폴더
module.exports = {
  file: 'Programs_01_프로젝트파일생성',
  alt: '프로젝트 파일 생성 과정 — GenerateProjectFiles.bat이 PreBuild.bat을 실행하고, PreBuild.bat은 두 도구가 없으면 BootstrapTools.bat으로 먼저 빌드한 뒤 JGHeaderTool과 JGBuildTool을 차례로 실행하며, JGHeaderTool은 Temp/CodeGen에 생성 코드를, JGBuildTool은 jgengine.lua · 모듈 목록 · Bin 서드파티 사본을 쓰고 premake5로 JGEngine.sln을 생성한다',
  type: 'flow',
  w: 1000,
  h: 450,
  nodes: [
    { id: 'GEN', x: 20, y: 40, w: 220, h: 56, color: 'white', title: 'GenerateProjectFiles.bat', lines: ['엔진 루트'] },
    { id: 'PB', x: 20, y: 160, w: 220, h: 56, color: 'white', title: 'PreBuild.bat', lines: ['두 도구를 차례로 실행'] },
    { id: 'BS', x: 20, y: 300, w: 220, h: 72, color: 'white', title: 'BootstrapTools.bat', lines: ['premake · MSBuild로', 'Core와 두 도구를 소스에서 빌드'] },
    { id: 'HT', x: 300, y: 70, w: 230, h: 72, color: 'purple', title: 'JGHeaderTool', lines: ['모듈 헤더의', 'JGCLASS 등을 분석'] },
    { id: 'BT', x: 300, y: 250, w: 230, h: 72, color: 'purple', title: 'JGBuildTool', lines: ['<모듈>.module.json 수집', '모듈별 project 블록 생성'] },
    { id: 'CG', x: 590, y: 70, w: 240, h: 72, color: 'blue', title: 'Temp/CodeGen/<모듈>/', lines: ['<헤더>.generation.h · .cpp', '<모듈>.codegen.generate.cpp'] },
    { id: 'LUA', x: 590, y: 180, w: 240, h: 56, color: 'blue', title: 'jgengine.lua', lines: ['BuildTemplate.lua + 모듈'] },
    { id: 'LIST', x: 590, y: 262, w: 240, h: 72, color: 'blue', title: '모듈 목록', lines: ['module_system_info.json', 'runtime_module_info.json'] },
    { id: 'BIN', x: 590, y: 360, w: 240, h: 56, color: 'green', title: 'Bin/<구성>/', lines: ['서드파티 lib · dll 사본'] },
    { id: 'PM', x: 870, y: 180, w: 110, h: 56, color: 'gray', title: 'premake5', lines: ['vs2022'] },
    { id: 'SLN', x: 845, y: 290, w: 140, h: 56, color: 'blue', title: 'JGEngine.sln', lines: ['Temp/ProjectFiles'] }
  ],
  edges: [
    { from: 'GEN', to: 'PB', fs: 'b', ts: 't' },
    { from: 'PB', to: 'BS', fs: 'b', ts: 't', dash: true, label: '도구가 없으면 먼저' },
    { from: 'PB', to: 'HT', fs: 'r', ts: 'l', fo: -10 },
    { from: 'PB', to: 'BT', fs: 'r', ts: 'l', fo: 10 },
    { from: 'HT', to: 'CG', fs: 'r', ts: 'l' },
    { from: 'BT', to: 'LUA', fs: 'r', ts: 'l', fo: -20 },
    { from: 'BT', to: 'LIST', fs: 'r', ts: 'l', fo: 12 },
    { from: 'BT', to: 'BIN', fs: 'b', ts: 'l' },
    { from: 'LUA', to: 'PM', fs: 'r', ts: 'l' },
    { from: 'PM', to: 'SLN', fs: 'b', ts: 't' }
  ]
};
