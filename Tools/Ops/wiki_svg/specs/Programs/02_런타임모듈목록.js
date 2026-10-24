// 1-2. 런타임 모듈 목록 — GameHost와 게임 모듈을 뿌리로 의존을 따라 내려가 게임 실행에 필요한 SharedLib 목록을 만든다
// 코드: JGBuildTool/Class/BuildTool.cpp(writeRuntimeModuleInfo · GAME_HOST_MODULE · DEVELOPMENT_RUNTIME_MODULES), Core/Misc/Module.cpp(HRuntimeModuleInfo)
module.exports = {
  file: 'Programs_02_런타임모듈목록',
  alt: '런타임 모듈 목록 — JGBuildTool은 GameHost와 .jgproject의 GameModules를 뿌리로 ModuleDependencies를 따라 내려가 SharedLib만 의존받는 쪽부터 모은 RuntimeModules를 runtime_module_info.json에 쓰고, 에디터 모듈 의존은 EditorDependencies(쿠킹 실패), GUI 의존은 DevelopmentDependencies(경고)로 기록하며, 이 목록을 JGLauncher -game과 cook.run이 사용한다',
  type: 'flow',
  w: 1060,
  h: 340,
  nodes: [
    { id: 'R1', x: 30, y: 50, w: 210, h: 56, color: 'green', title: 'GameHost', lines: ['엔진 쪽 뿌리'] },
    { id: 'R2', x: 30, y: 150, w: 210, h: 56, color: 'yellow', title: '.jgproject GameModules', lines: ['게임 쪽 뿌리'] },
    { id: 'V', x: 290, y: 84, w: 270, h: 90, color: 'purple', title: 'writeRuntimeModuleInfo', lines: ['ModuleDependencies를 따라 내려감', 'SharedLib만 · 의존받는 쪽 먼저'] },
    { id: 'OUT', x: 610, y: 40, w: 230, h: 72, color: 'blue', title: 'runtime_module_info.json', lines: ['RuntimeModules', 'GameModules'] },
    { id: 'E1', x: 610, y: 150, w: 230, h: 56, color: 'red', title: 'EditorDependencies', lines: ['에디터 모듈 의존 — 쿠킹 실패'] },
    { id: 'E2', x: 610, y: 240, w: 230, h: 56, color: 'yellow', title: 'DevelopmentDependencies', lines: ['GUI 의존 — 경고'] },
    { id: 'C1', x: 880, y: 32, w: 160, h: 56, color: 'purple', title: 'JGLauncher -game', lines: ['런타임 모듈만 로드'] },
    { id: 'C2', x: 880, y: 110, w: 160, h: 56, color: 'orange', title: 'cook.run', lines: ['Bin · JGPackage.json'] }
  ],
  edges: [
    { from: 'R1', to: 'V', fs: 'r', ts: 'l', to_off: -14 },
    { from: 'R2', to: 'V', fs: 'r', ts: 'l', to_off: 14 },
    { from: 'V', to: 'OUT', fs: 'r', ts: 'l', fo: -20 },
    { from: 'V', to: 'E1', fs: 'r', ts: 'l', fo: 10 },
    { from: 'V', to: 'E2', fs: 'b', ts: 'l' },
    { from: 'OUT', to: 'C1', fs: 'r', ts: 'l', fo: -16 },
    { from: 'OUT', to: 'C2', fs: 'r', ts: 'l', fo: 16 }
  ]
};
