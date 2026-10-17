// 1-1. Core 의존 관계
// 화살표 = 링크하여 사용. 왼쪽 상자 안 글 = 해당 모듈이 Core에서 주로 사용하는 기능. Core는 세로로 길게 두어 화살표가 모두 직선으로 들어가게 한다.
const left = [
  { id: 'ASSET', title: 'Asset', lines: ['파일 · 리플렉션 직렬화로 에셋 로드'] },
  { id: 'AUDIO', title: 'Audio', lines: ['전역 시스템 · 창 이벤트 · 파일'] },
  { id: 'GFX', title: 'Graphics', lines: ['프레임 단계 · 창 크기 변경 이벤트'] },
  { id: 'GUI', title: 'GUI', lines: ['창 메시지 · 프레임 단계'] },
  { id: 'GFW', title: 'GameFrameWorks', lines: ['메모리 · 리플렉션 · 프레임 Update'] },
  { id: 'HOST', title: 'GameHost', lines: ['창 생성 · 엔진 입력 · 렌더링 단계'] },
  { id: 'ED', title: 'JGEditor', lines: ['창 생성 · 모듈 연결'] }
];
const CORE = { id: 'CORE', x: 380, y: 30, w: 200, h: 630, color: 'blue', title: 'Core', lines: ['다른 모듈에 의존하지 않음'] };
const nodes = left.map((n, i) => Object.assign({ x: 30, y: 30 + i * 80, w: 270, h: 60, color: 'purple' }, n));
nodes.push({ id: 'PROG', x: 30, y: 590, w: 270, h: 70, color: 'gray', title: '실행 프로그램', lines: ['JGLauncher · JGConsole · JGBuildTool', 'JGHeaderTool · JGGameLaunch'] });
nodes.push(CORE);
const coreMid = CORE.y + CORE.h / 2;

module.exports = {
  file: 'Core_01_누가기대나',
  alt: 'Core 의존 관계 — Asset · Audio · Graphics · GUI · GameFrameWorks · GameHost · JGEditor 모듈과 실행 프로그램은 모두 Core를 링크하여 사용하며, Core는 다른 모듈에 의존하지 않음',
  type: 'flow',
  w: 610,
  h: 690,
  nodes,
  edges: nodes.filter(n => n.id !== 'CORE').map(n => ({ from: n.id, to: 'CORE', fs: 'r', ts: 'l', to_off: n.y + n.h / 2 - coreMid }))
};
