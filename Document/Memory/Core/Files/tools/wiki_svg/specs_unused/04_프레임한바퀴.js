// 2-2. 한 프레임 처리 순서
// 가운데 줄 = 관리자 Update가 시스템을 등록 순서(1~8)대로 호출하는 순서. 3 스케줄 안에서 프레임 단계 6개가 왼쪽에서 오른쪽으로 실행된다.
// "창이 닫혔는가" 분기는 마름모.
const stages = [
  { id: 'G1', title: 'Graphics Begin' },
  { id: 'B1', title: 'Begin' },
  { id: 'UP', title: 'Update' },
  { id: 'E1', title: 'End' },
  { id: 'G2', title: 'Graphics End' },
  { id: 'W1', title: 'Update Window' }
].map((n, i) => Object.assign({ x: 50 + i * 110, y: 390, w: 80, h: 50, color: 'white' }, n));

const stageEdges = [];
for (let i = 1; i < stages.length; i++) {
  stageEdges.push({ from: stages[i - 1].id, to: stages[i].id });
}

module.exports = {
  file: 'Core_04_프레임한바퀴',
  alt: '프레임 갱신 — GCoreSystem::Update가 1 메모리 GC, 2 타이머, 3 스케줄(프레임 단계 Graphics Begin → Begin → Update → End → Graphics End → Update Window), 4 문자열 테이블, 5 콘솔 명령, 6~8을 순서대로 호출하고, 창이 닫히면 종료 단계로 진행',
  type: 'flow',
  w: 790,
  h: 930,
  groups: [{ x: 30, y: 340, w: 670, h: 120, label: '프레임 단계', color: 'green' }],
  nodes: [
    { id: 'U', x: 215, y: 30, w: 300, h: 60, color: 'blue', title: 'GCoreSystem::Update', lines: ['프레임마다 1회 호출'] },
    { id: 'M1', x: 215, y: 130, w: 300, h: 70, color: 'green', title: '1 메모리 GC → 2 타이머', lines: ['GC: 이전 프레임에 참조가 해제된', '객체 파괴 · 빈 페이지 회수'] },
    { id: 'SC', x: 215, y: 240, w: 300, h: 60, color: 'green', title: '3 스케줄', lines: ['프레임 단계별 작업 순차 실행'] }
  ].concat(stages, [
    { id: 'CC', x: 215, y: 500, w: 300, h: 70, color: 'green', title: '4 문자열 테이블 → 5 콘솔 명령', lines: ['5: Submit으로 대기 중인 명령 실행', '(프레임 단계 외부)'] },
    { id: 'EX', x: 215, y: 610, w: 300, h: 70, color: 'green', title: '6 모듈 → 7 오브젝트 → 8 모듈이 등록한 시스템', lines: ['6 · 7: 갱신 작업 없음', '8: 에셋 DB · 오디오 등'] },
    { id: 'Q', x: 255, y: 715, w: 220, h: 80, color: 'white', shape: 'diamond', title: '창이 닫혔는가' },
    { id: 'D', x: 295, y: 860, w: 140, h: 40, color: 'gray', title: '종료 단계로' }
  ]),
  edges: [
    { from: 'U', to: 'M1' },
    { from: 'M1', to: 'SC' },
    { from: 'SC', to: 'STAGE', points: [[365, 300], [365, 340]] },
    { from: 'STAGE', to: 'CC', points: [[365, 460], [365, 500]] },
    { from: 'CC', to: 'EX' },
    { from: 'EX', to: 'Q' },
    { from: 'Q', to: 'U', fs: 'r', ts: 'r', via: [[730, 755], [730, 60]], label: '아니오', lx: 730, ly: 400 },
    { from: 'Q', to: 'D', fs: 'b', ts: 't', label: '예 — WM_QUIT 수신 또는 RequestExit 호출', lw: 260 }
  ].concat(stageEdges)
};
