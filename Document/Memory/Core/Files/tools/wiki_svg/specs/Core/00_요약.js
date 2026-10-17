// 0. Core 모듈 — Core.lib 안의 관리자 · 베이스 시스템 8개(클래스) · 공용 도구, 그 위에 올라가는 모듈 DLL
// 2026-10-17 20:35 사용자 피드백: "Core (.lib) — GCoreSystem, 베이스 시스템 클래스 다이어그램, 모듈 DLL 을 자연스럽게"
// 베이스 시스템은 등록 차례(1~8)대로 두 줄씩. 가운데 세로줄 = GGlobalSystemInstance<T> 상속.
const TRUNK = 680;
const sys = [
  { id: 'S1', stereo: '1 · 로그', name: 'GLogGlobalSystem', members: ['+AddLog()  ← JG_LOG', '+GetRecentLogs()'] },
  { id: 'S2', stereo: '2 · 메모리', name: 'GMemoryGlobalSystem', members: ['+Allocate<T>()', '#Update()  // GC'] },
  { id: 'S3', stereo: '3 · 타이머', name: 'GTimerGlobalSystem', members: ['+CreateTimer()'] },
  { id: 'S4', stereo: '4 · 스케줄', name: 'GScheduleGlobalSystem', members: ['+ScheduleByFrame()', '+ScheduleAsync()'] },
  { id: 'S5', stereo: '5 · 문자열 테이블', name: 'GStringTable', members: ['+RegisterString()', '+FindString()'] },
  { id: 'S6', stereo: '6 · 콘솔 명령', name: 'GConsoleCommandGlobalSystem', members: ['+Register()', '+Execute() · Submit()'] },
  { id: 'S7', stereo: '7 · 모듈', name: 'GModuleGlobalSystem', members: ['+ConnectModule()', '+FindModule<T>()'] },
  { id: 'S8', stereo: '8 · 오브젝트(리플렉션)', name: 'GObjectGlobalSystem', members: ['+NewObject()', '+GetStaticClass()'] }
].map((s, i) => Object.assign({
  x: i % 2 === 0 ? 400 : 720,
  y: 400 + Math.floor(i / 2) * 110,
  w: 240,
  color: 'green'
}, s));

// 베이스 시스템 → 가운데 세로줄 → GGlobalSystemInstance<T> (상속)
const sysEdges = sys.map(s => {
  const left = s.x < TRUNK;
  const cy = s.y + (s.members.length === 1 ? 36 : 45);
  return { points: [[left ? s.x + s.w : s.x, cy], [TRUNK, cy], [TRUNK, 350]], end: 'triangle' };
});

module.exports = {
  file: 'Core_00_Core모듈',
  alt: 'Core 모듈 — Core.lib 안의 관리자 GCoreSystem, 전역 시스템 기반 클래스를 상속한 베이스 시스템 8개, 공용 도구, Core에 의존하는 모듈 DLL의 기반 클래스 IModuleInterface와 모듈이 등록한 전역 시스템',
  type: 'class',
  w: 1010,
  h: 1110,
  groups: [
    { x: 20, y: 20, w: 960, h: 830, label: 'Core 모듈 (Core.lib) — 엔진의 가장 하위 계층' },
    { x: 380, y: 890, w: 600, h: 200, label: '모듈 DLL — Core에 의존하는 상위 시스템', color: 'purple' }
  ],
  classes: [
    { id: 'GC', x: 40, y: 70, w: 300, stereo: '관리자 · 단일 객체', name: 'GCoreSystem', color: 'blue', members: ['+Create(args)    // 초기화', '+Update()        // 프레임 갱신', '+Destroy()       // 종료', '+RegisterSystemInstance<T>()', '+GetSystemInstance<T>()', '+RequestExit(code)', '-SystemInstanceList'] },
    { id: 'BASE', x: 535, y: 106, w: 290, stereo: '전역 시스템 기반 클래스', name: 'GGlobalSystemInstanceBase', color: 'blue', members: ['#Start()', '#Update()', '#Destroy()'] },
    { id: 'INST', x: 535, y: 260, w: 290, stereo: 'GetInstance()로 전역 접근', name: 'GGlobalSystemInstance<T>', color: 'blue', members: ['+GetInstance() : T&', '+IsValid()'] },
    { id: 'TOOLS', x: 40, y: 400, w: 300, stereo: '시스템에 속하지 않는 유틸리티', name: '공용 도구', color: 'yellow', members: ['대리자: HMulticastDelegate', '창: PJWindow', '플랫폼: HPlatform', '파일 · 경로: HFileHelper', 'JSON: PJsonData', '문자열: PString · PName', '수학: HVector · HMatrix', 'GUID · 시간: HGuid · PDateTime'] },
    { id: 'IMOD', x: 400, y: 940, w: 240, stereo: '모듈 기반 클래스 · DLL마다 하나', name: 'IModuleInterface', color: 'purple', members: ['#StartupModule()', '#ShutdownModule()'] },
    { id: 'AUD', x: 720, y: 940, w: 240, stereo: '각 모듈이 StartupModule에서 등록', name: '모듈의 전역 시스템', color: 'green', members: ['GAssetDatabase (Asset)', 'GAudioGlobalSystem (Audio)', 'GShaderLibrary (Graphics)'] }
  ].concat(sys),
  nodes: [
    { id: 'KEY', x: 40, y: 660, w: 300, h: 150, color: 'white', shape: 'note', title: '범례', lines: ['◇— 포함(시스템 목록)', '—▷ 상속', '→ 호출 · 생성', '- - → 등록', '+ public · # protected · - private'] },
  ],
  edges: [
    { from: 'GC', to: 'BASE', fs: 'r', ts: 'l', start: 'diamond', end: 'none', label: '시스템 목록 (등록 순서대로 갱신)', lw: 220 },
    { from: 'INST', to: 'BASE', fs: 't', ts: 'b', end: 'triangle' },
    { from: 'S7', to: 'IMOD', points: [[520, 820], [520, 860], [365, 860], [365, 962], [400, 962]], label: 'DLL 로드 → 모듈 객체 생성 → StartupModule 호출', lw: 250, lx: 365, ly: 912, lpos: 'l' },
    { points: [[520, 1030], [520, 1065], [840, 1065], [840, 1048]], dash: true, label: 'StartupModule에서 등록 (RegisterSystemInstance)', lw: 360, lx: 680, ly: 1065, lpos: 'b' },
    { points: [[840, 940], [840, 876], [995, 876], [995, 305], [825, 305]], end: 'triangle', label: '같은 기반 클래스 — 목록에 추가되어 매 프레임 갱신', lw: 320, lx: 848, ly: 876 }
  ].concat(sysEdges)
};
