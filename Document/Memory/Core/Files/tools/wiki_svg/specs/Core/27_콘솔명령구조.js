// 4-1. GConsoleCommandGlobalSystem과 HAutoConsoleCommand — 콘솔 명령 레지스트리와 정적 선언 객체, 인자 파서
// 코드: Core/ConsoleCommand/ConsoleCommandGlobalSystem.h/.cpp, ConsoleCommandArgs.h/.cpp, Core/Misc/Module.h/.cpp(RegisterAutoConsoleCommands)
module.exports = {
  file: 'Core_27_콘솔명령구조',
  alt: 'GConsoleCommandGlobalSystem과 HAutoConsoleCommand — GConsoleCommandGlobalSystem은 이름별 HConsoleCommandDesc와 Submit 대기열을 보관하고, 모듈 DLL의 HAutoConsoleCommand는 모듈 연결 시 RegisterAll로 등록되며, DevConsole은 Submit, JGConsole은 Execute로 명령을 실행하고 인자는 HConsoleCommandArgs가 분석한다',
  type: 'class',
  w: 960,
  h: 620,
  classes: [
    { id: 'GCC', x: 40, y: 130, w: 380, name: 'GConsoleCommandGlobalSystem', stereo: '콘솔 명령 시스템 · 베이스 시스템 6번', color: 'green', members: ['-_commands : 이름 → HConsoleCommandDesc', '-_pendingLines : Submit 대기열', '+Register(desc) · Unregister(name)', '+Execute(line)  // 즉시 실행', '+Submit(line)  // 다음 프레임 실행', '+FindCommands(text)', '#Update()  // 대기열 실행'] },
    { id: 'DESC', x: 580, y: 130, w: 340, name: 'HConsoleCommandDesc', stereo: '명령 정의', color: 'purple', members: ['Name : 영역.동작', 'Usage · Description', 'Handler : bool (const HConsoleCommandArgs&)'] },
    { id: 'AUTO', x: 580, y: 290, w: 340, name: 'HAutoConsoleCommand', stereo: '정적 선언 객체 · 바이너리마다 목록', color: 'yellow', members: ['-Head (static) · _next : 바이너리별 목록', '+RegisterAll(registry)', '+UnregisterAll(registry)'] },
    { id: 'IMI', x: 580, y: 460, w: 340, name: 'IModuleInterface', stereo: '모듈 기반 클래스', color: 'purple', members: ['#RegisterAutoConsoleCommands()', '#UnregisterAutoConsoleCommands()'] },
    { id: 'ARG', x: 40, y: 390, w: 380, name: 'HConsoleCommandArgs', stereo: '인자 파서', color: 'blue', members: ['+Tokenize(line)', '+GetPositional(i) · Has(name)', '+TryGetString() · TryGetInt()'] }
  ],
  nodes: [
    { id: 'DC', x: 40, y: 30, w: 180, h: 56, color: 'gray', title: 'DevConsole 창', lines: ['에디터 GUI'] },
    { id: 'JC', x: 240, y: 30, w: 180, h: 56, color: 'gray', title: 'JGConsole', lines: ['명령줄 도구'] },
    { id: 'KEY', x: 40, y: 530, w: 380, h: 70, color: 'white', shape: 'note', title: '범례', lines: ['◇— 보관 · → 호출 · - - → 등록', '+ public · # protected · - private'] }
  ],
  edges: [
    { from: 'DC', to: 'GCC', fs: 'b', ts: 't', fo: 0, to_off: -100, label: 'Submit' },
    { from: 'JC', to: 'GCC', fs: 'b', ts: 't', fo: 0, to_off: 100, label: 'Execute' },
    { from: 'GCC', to: 'DESC', fs: 'r', ts: 'l', fo: -36, to_off: 0, start: 'diamond', end: 'none', label: '보관' },
    { points: [[580, 344], [500, 344], [500, 290], [420, 290]], dash: true, label: '등록 · 해제', lx: 500, ly: 317 },
    { from: 'IMI', to: 'AUTO', fs: 't', ts: 'b', label: 'RegisterAll · UnregisterAll 호출', lw: 240 },
    { from: 'GCC', to: 'ARG', fs: 'b', ts: 't', label: '줄 토큰화 · 인자 분석' }
  ]
};
