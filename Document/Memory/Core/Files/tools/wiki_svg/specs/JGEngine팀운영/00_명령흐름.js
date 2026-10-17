// 4. 대시보드 — 명령 흐름
module.exports = {
  file: 'JGEngine팀운영_00_명령흐름',
  alt: '명령 흐름 — 사용자가 통합 대시보드에서 보낸 명령은 inbox 폴더에 JSON 파일로 저장되고 수신기 watch_inbox.js가 엔진 총괄에 알리며, 엔진 총괄은 기록 후 담당 세션에 원문 · 맥락을 전달하고 data.js 명령 상태를 갱신하고, 담당 세션의 완료 보고를 받아 항목 이동 · 명령 상태를 완료로 바꾼다',
  type: 'seq',
  colW: 175,
  w: 1000,
  participants: [
    { id: 'U', label: '사용자', color: 'orange' },
    { id: 'D', label: '통합 대시보드', sub: '엔진 팀 화면', color: 'purple' },
    { id: 'I', label: 'inbox/', sub: '명령 파일 폴더', color: 'yellow' },
    { id: 'C', label: '엔진 총괄', sub: '관리자 세션', color: 'blue' },
    { id: 'S', label: '담당 세션', sub: '분야 세션', color: 'green' }
  ],
  steps: [
    { gap: 30 },
    { from: 'U', to: 'D', text: '항목 · 분야 카드 · 명령함에서 명령 전송 (그림 첨부 가능)' },
    { from: 'D', to: 'I', text: 'c-날짜-시각.json 파일 쓰기' },
    { from: 'I', to: 'C', text: '수신기 watch_inbox.js가 알림', dash: true },
    { gap: 14 },
    { from: 'C', to: 'C', text: '채팅에 기록, 결정이면 진행현황.md 결정 대기 표에 D-n 추가' },
    { from: 'C', to: 'S', text: '총괄 대시보드 명령 + 원문 · 맥락 전달' },
    { from: 'C', to: 'D', text: 'data.js commands 상태 → 전달' },
    { gap: 14 },
    { from: 'S', to: 'S', text: '구현 · 검증 · 현황 · TODO · 진행현황 갱신' },
    { from: 'S', to: 'C', text: '완료 보고 (결과 · 증적 위치)', dash: true },
    { from: 'C', to: 'D', text: '항목 이동 · 명령 상태 → 완료' }
  ]
};
