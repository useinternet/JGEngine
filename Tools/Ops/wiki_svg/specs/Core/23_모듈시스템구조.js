// 1-1. GModuleGlobalSystem과 IModuleInterface — 모듈 시스템이 모듈 객체를 보관하고, 모듈 DLL은 IModuleInterface를 구현해 진입 함수로 넘긴다
// 코드: Core/Misc/Module.h · Module.cpp, Editor/JGEditor/JGEditor.cpp, Runtime/GameHost/GameHostModule.cpp
const TRUNK = 650;
const derived = [
  { id: 'ED', x: 430, y: 230, name: 'HJGEditorModule', stereo: '실행 모듈 · 에디터' },
  { id: 'GH', x: 670, y: 230, name: 'HGameHostModule', stereo: '실행 모듈 · 게임' },
  { id: 'EM', x: 430, y: 320, name: '엔진 모듈 클래스', stereo: 'Graphics, GUI, Asset 등' },
  { id: 'GM', x: 670, y: 320, name: '게임 모듈 클래스', stereo: '.jgproject GameModules' }
].map(c => Object.assign({ w: 200, color: 'purple' }, c));

module.exports = {
  file: 'Core_23_모듈시스템구조',
  alt: 'GModuleGlobalSystem과 IModuleInterface — 모듈 시스템은 모듈 객체를 이름 · 타입 · 연결 순서로 보관하고, 각 모듈 DLL은 IModuleInterface를 상속한 모듈 클래스를 JG_MODULE_IMPL 진입 함수로 넘긴다',
  type: 'class',
  w: 900,
  h: 560,
  classes: [
    { id: 'GMS', x: 40, y: 30, w: 330, name: 'GModuleGlobalSystem', stereo: '모듈 시스템 · 베이스 시스템', color: 'green', members: ['-_modulesByName : 이름 → 모듈', '-_modulesByType : 타입 → 모듈', '-_moduleOrder : 연결 순서', '+ConnectModule(name)', '+DisconnectModule(name)', '+FindModule<T>() · FindModule(name)'] },
    { id: 'IMI', x: 500, y: 30, w: 300, name: 'IModuleInterface', stereo: '모듈 기반 클래스 · DLL마다 하나', color: 'purple', members: ['#GetModuleType()', '#StartupModule()', '#ShutdownModule()', '#RegisterAutoConsoleCommands()'] }
  ].concat(derived),
  nodes: [
    { id: 'DLL', x: 40, y: 290, w: 330, h: 104, color: 'yellow', title: 'JG_MODULE_IMPL(모듈 클래스, API)', lines: ['DLL 진입 함수 2개를 생성한다', 'Link_Module(GCoreSystem*) — Core 사본 연결', '_Create_Module_Interface_() — 모듈 객체 생성'] },
    { id: 'KEY', x: 40, y: 430, w: 330, h: 100, color: 'white', shape: 'note', title: '범례', lines: ['◇— 포함 · —▷ 상속 · → 호출 · 생성', '+ public · # protected(모듈 시스템만 호출) · - private'] }
  ],
  edges: [
    { from: 'GMS', to: 'IMI', fs: 'r', ts: 'l', start: 'diamond', end: 'none', label: '연결된 모듈 보관', fo: -30, to_off: -12 },
    { from: 'GMS', to: 'DLL', fs: 'b', ts: 't', label: 'ConnectModule: DLL 로드 후 호출', lw: 260 },
    { points: [[370, 342], [420, 342], [420, 120], [500, 120]], label: '모듈 객체 생성', lx: 460, ly: 120 }
  ].concat(derived.map(c => {
    const left = c.x < TRUNK;
    const cy = c.y + 22;
    return { points: [[left ? c.x + c.w : c.x, cy], [TRUNK, cy], [TRUNK, 156]], end: 'triangle' };
  }))
};
