// 세션 짜임 — 사용자와 프로젝트 총괄(통합 대시보드 보고함), 엔진 총괄, 분야 세션 6개, Document Writer, 게임 총괄
module.exports = {
  file: 'JGEngine팀구성및운영_00_세션짜임',
  alt: '세션 짜임 — 사용자는 프로젝트 총괄의 통합 대시보드 보고함으로 묻고 답하며, 엔진 총괄은 hq_request.js로 보고 · 질문을 보내고 분야 세션 6개에 일을 맡기고, Document Writer가 보고함 글을 다듬으며, 게임 팀과는 엔진 요청서(ER)로 이어진다',
  type: 'flow',
  w: 980,
  h: 530,
  groups: [
    { x: 20, y: 390, w: 745, h: 120, label: '분야 세션' }
  ],
  nodes: [
    { id: 'USER', x: 400, y: 40, w: 240, h: 52, color: 'yellow', title: '사용자', lines: ['방향 · 결정 · 확인'] },
    { id: 'HQ', x: 400, y: 150, w: 240, h: 52, color: 'purple', title: '프로젝트 총괄', lines: ['통합 대시보드 보고함(H-n)'] },
    { id: 'ENG', x: 160, y: 270, w: 260, h: 52, color: 'blue', title: '엔진 총괄', lines: ['일 나누기 · 묶음 적용 · 위키'] },
    { id: 'GAME', x: 600, y: 270, w: 240, h: 52, color: 'green', title: '게임 총괄', lines: ['ProjectAH 팀'] },
    { id: 'S1', x: 35, y: 430, w: 112, h: 50, color: 'blue', title: 'Core', lines: ['Core 모듈'] },
    { id: 'S2', x: 155, y: 430, w: 112, h: 50, color: 'blue', title: 'Graphics', lines: ['렌더러'] },
    { id: 'S3', x: 275, y: 430, w: 112, h: 50, color: 'blue', title: 'GameFrameWork', lines: ['게임 실행 기반'] },
    { id: 'S4', x: 395, y: 430, w: 112, h: 50, color: 'blue', title: 'GUI', lines: ['ImGui'] },
    { id: 'S5', x: 515, y: 430, w: 112, h: 50, color: 'blue', title: 'DevOps', lines: ['빌드 · 도구'] },
    { id: 'S6', x: 635, y: 430, w: 112, h: 50, color: 'blue', title: 'Editor', lines: ['에디터 창'] },
    { id: 'DW', x: 780, y: 430, w: 180, h: 50, color: 'gray', title: 'Document Writer', lines: ['보고함 글 · 위키 문장'] }
  ],
  edges: [
    { from: 'USER', to: 'HQ', points: [[520, 92], [520, 150]], start: 'arrow', label: '묻기 · 답 · 확인' },
    { from: 'HQ', to: 'ENG', points: [[410, 202], [410, 270]], start: 'arrow', label: 'hq_request.js · 전달' },
    { from: 'HQ', to: 'GAME', points: [[620, 202], [620, 270]], start: 'arrow', label: '보고함 · 전달' },
    { from: 'ENG', to: 'GAME', points: [[420, 296], [600, 296]], dash: true, start: 'arrow', label: '엔진 요청서(ER)' },
    { from: 'ENG', to: 'S3', points: [[330, 322], [330, 430]], start: 'arrow', label: '일 맡기기 · 결과 보고' },
    { from: 'DW', to: 'ENG', points: [[870, 430], [870, 356], [380, 356], [380, 322]], label: '다듬은 글' }
  ]
};
