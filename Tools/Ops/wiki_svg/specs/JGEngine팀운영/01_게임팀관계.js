// 6. 게임 팀(ProjectAH)과의 관계
// 프로젝트 총괄은 맨 위, 사용자는 두 저장소 사이 가운데. 왼쪽 엔진 저장소 · 오른쪽 게임 저장소.
// 저장소 사이 두 길: 분야 세션 → 게임 총괄(위 줄), 엔진요청 폴더 → 엔진 총괄(아래 줄, 엔진 묶음 왼쪽으로 돌아 들어감) — 선이 엇갈리지 않게.
module.exports = {
  file: 'JGEngine팀운영_01_게임팀관계',
  alt: '게임 팀(ProjectAH)과의 관계 — 사용자와 프로젝트 총괄이 엔진 총괄 · 게임 총괄에 지시하고, 게임 쪽 세션이 엔진요청 폴더에 ER-n을 쓰면 게임 총괄이 엔진 총괄에 전달하며, 엔진 총괄은 분야 세션에 검토 · 구현을 배정하고, 분야 세션은 결정이 필요하면 사용자에게 D-n을 올리고 쓰는 법 · 반영 알림을 게임 총괄에 보낸다',
  type: 'flow',
  w: 1000,
  h: 495,
  groups: [
    { x: 50, y: 112, w: 250, h: 208, label: '엔진 저장소 C:/Develop/JGEngine' },
    { x: 700, y: 112, w: 280, h: 365, label: '게임 저장소 C:/Develop/ProjectAH' }
  ],
  nodes: [
    { id: 'PJ', x: 380, y: 20, w: 240, h: 60, color: 'purple', title: '프로젝트 총괄', lines: ['통합 대시보드 · 전체 지침'] },
    { id: 'U', x: 430, y: 150, w: 140, h: 50, color: 'orange', title: '사용자' },
    { id: 'EC', x: 75, y: 150, w: 200, h: 50, color: 'blue', title: '엔진 총괄' },
    { id: 'ES', x: 75, y: 250, w: 200, h: 50, color: 'blue', title: '분야 세션 7개' },
    { id: 'GC', x: 740, y: 150, w: 200, h: 50, color: 'green', title: '게임 총괄' },
    { id: 'ER', x: 740, y: 330, w: 200, h: 56, color: 'yellow', title: '엔진요청 폴더', lines: ['ER-n 요청서'] },
    { id: 'GL', x: 715, y: 420, w: 120, h: 40, color: 'green', title: '게임 로직 구현' },
    { id: 'GV', x: 845, y: 420, w: 120, h: 40, color: 'green', title: '게임 연출 구현' }
  ],
  edges: [
    { from: 'U', to: 'PJ', fs: 't', ts: 'b' },
    { from: 'PJ', to: 'EC', points: [[380, 50], [330, 50], [330, 163], [275, 163]], label: '전달 사항' },
    { from: 'PJ', to: 'GC', points: [[620, 50], [670, 50], [670, 163], [740, 163]], label: '전달 사항', lpos: 'l' },
    { from: 'U', to: 'EC', fs: 'l', ts: 'r', fo: 12, to_off: 12 },
    { from: 'U', to: 'GC', fs: 'r', ts: 'l', fo: 12, to_off: 12 },
    { from: 'EC', to: 'ES', fs: 'b', ts: 't', label: '검토 · 구현 배정' },
    { from: 'ES', to: 'U', points: [[275, 265], [500, 265], [500, 200]], label: '결정이 필요하면 D-n' },
    { from: 'ES', to: 'GC', points: [[275, 285], [800, 285], [800, 200]], label: '쓰는 법 · 반영 알림', lpos: 'b', lx: 560, ly: 285 },
    { from: 'GC', to: 'ER', fs: 'b', ts: 't' },
    { from: 'GL', to: 'ER', fs: 't', ts: 'b', to_off: -65 },
    { from: 'GV', to: 'ER', fs: 't', ts: 'b', to_off: 65 },
    { from: 'ER', to: 'EC', points: [[740, 358], [30, 358], [30, 175], [75, 175]], label: '게임 총괄이 전달', lx: 500, ly: 358 }
  ]
};
