// 7-2. 콘솔 명령 실행 흐름
module.exports = {
  file: 'Core_16_명령실행길',
  alt: '명령 실행 과정 — DevConsole 창은 Submit으로 명령을 대기열에 넣어 다음 프레임 5단계에서 실행하고 결과 줄을 로그 · 최근 로그로 표시하며, JGConsole은 Execute로 즉시 호출하여 종료 코드 0 · 1을 받는다',
  type: 'seq',
  colW: 230,
  participants: [
    { id: 'DC', label: 'DevConsole 창', color: 'purple' },
    { id: 'JC', label: 'JGConsole', color: 'gray' },
    { id: 'R', label: 'GConsoleCommandGlobalSystem', sub: '콘솔 명령 레지스트리', color: 'blue' },
    { id: 'H', label: '명령 함수', color: 'orange' },
    { id: 'LG', label: 'GLogGlobalSystem', sub: '로그 시스템', color: 'green' }
  ],
  steps: [
    { from: 'DC', to: 'R', text: 'Submit — 대기열에 추가' },
    { note: '다음 프레임 콘솔 명령 시스템 Update에서 실행', over: ['R'] },
    { gap: 10 },
    { from: 'R', to: 'R', text: '명령 줄 토큰화' },
    { from: 'R', to: 'H', text: '호출' },
    { from: 'H', to: 'R', text: '성공 · 실패', dash: true },
    { from: 'R', to: 'LG', text: '결과 줄 기록' },
    { from: 'LG', to: 'DC', text: '최근 로그로 표시', dash: true },
    { gap: 14 },
    { from: 'JC', to: 'R', text: 'Execute — 즉시 실행' },
    { from: 'R', to: 'H', text: '호출' },
    { from: 'H', to: 'JC', text: '종료 코드 0 · 1', dash: true }
  ]
};
