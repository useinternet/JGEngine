// 일하는 흐름 — 설계도 → 사용자 확인 → 떼어 둔 작업 폴더에서 구현 · 검증 → 엔진 총괄이 본 폴더에 묶어 적용 → 커밋 · 푸시 → 게임 팀 맞춤 → 보고
module.exports = {
  file: 'JGEngine팀구성및운영_01_일하는흐름',
  alt: '일하는 흐름 — 분야 세션이 설계도를 올려 사용자 확인을 받고, 떼어 둔 작업 폴더에서 구현 · 검증하면, 엔진 총괄이 본 폴더에 묶어 적용하고 커밋 · 푸시한 뒤 게임 팀이 같은 때 프로젝트 파일을 다시 만들고, 결과를 보고함에 보고한다',
  type: 'flow',
  w: 1000,
  h: 300,
  nodes: [
    { id: 'S1', x: 30, y: 50, w: 200, h: 52, color: 'blue', title: '설계도', lines: ['그림 · 고칠 곳 · 짧은 코드'] },
    { id: 'S2', x: 270, y: 50, w: 180, h: 52, color: 'yellow', title: '사용자 확인', lines: ['보고함'] },
    { id: 'S3', x: 490, y: 50, w: 220, h: 52, color: 'blue', title: '떼어 둔 작업 폴더', lines: ['Temp/wt에서 구현 · 검증'] },
    { id: 'S4', x: 750, y: 50, w: 220, h: 52, color: 'green', title: '본 폴더에 묶어 적용', lines: ['엔진 총괄 · MainTree.cmd'] },
    { id: 'S5', x: 750, y: 210, w: 220, h: 52, color: 'green', title: '커밋 · 푸시', lines: ['엔진 총괄 — 고친 파일만'] },
    { id: 'S6', x: 490, y: 210, w: 220, h: 52, color: 'green', title: '게임 팀 맞춤', lines: ['같은 때 프로젝트 파일 다시 만들기'] },
    { id: 'S7', x: 270, y: 210, w: 180, h: 52, color: 'purple', title: '보고', lines: ['무엇을 · 커밋 번호'] }
  ],
  edges: [
    { from: 'S1', to: 'S2', points: [[230, 76], [270, 76]] },
    { from: 'S2', to: 'S3', points: [[450, 76], [490, 76]] },
    { from: 'S3', to: 'S4', points: [[710, 76], [750, 76]] },
    { from: 'S2', to: 'S1', points: [[360, 102], [360, 150], [130, 150], [130, 102]], dash: true, label: '고칠 곳 있음' },
    { from: 'S4', to: 'S5', points: [[860, 102], [860, 210]] },
    { from: 'S5', to: 'S6', points: [[750, 236], [710, 236]] },
    { from: 'S6', to: 'S7', points: [[490, 236], [450, 236]] }
  ]
};
