// 3-2. 모듈 연결 순서와 종료 순서
// 왼쪽 묶음 = 시작 시 목록에 추가되는 순서, 오른쪽 묶음 = 종료 시 ShutdownModule 호출 순서(역순). 게임 모듈만 주황(게임 코드).
const col = (x, items) => items.map((it, i) => ({ id: it[0], x, y: 80 + i * 70, w: 180, h: 40, color: it[2] || 'purple', title: it[1] }));
const on = col(60, [['o1', '1 Graphics'], ['o2', '2 GUI'], ['o3', '3 Asset'], ['o4', '4 … GameFrameWorks'], ['o5', '5 게임 모듈', 'orange'], ['o6', '6 JGEditor']]);
const off = col(420, [['f6', '1 JGEditor'], ['f5', '2 게임 모듈', 'orange'], ['f4', '3 GameFrameWorks …'], ['f3', '4 Asset'], ['f2', '5 GUI'], ['f1', '6 Graphics']]);
const chain = (list) => list.slice(1).map((n, i) => ({ from: list[i].id, to: n.id }));

module.exports = {
  file: 'Core_07_연결순서',
  alt: '모듈 연결 순서와 종료 순서 — 시작 시 Graphics · GUI · Asset · GameFrameWorks · 게임 모듈 · JGEditor 순으로 목록에 추가되고, 종료 시 역순으로 JGEditor부터 ShutdownModule 호출',
  type: 'flow',
  w: 660,
  h: 520,
  groups: [
    { x: 20, y: 30, w: 260, h: 460, label: '시작 시 — 목록 추가 순서' },
    { x: 380, y: 30, w: 260, h: 460, label: '종료 시 — ShutdownModule 호출 순서' }
  ],
  nodes: on.concat(off),
  edges: chain(on).concat(chain(off), [
    { from: 'ON', to: 'OFF', points: [[270, 260], [390, 260]], label: '역순' }
  ])
};
