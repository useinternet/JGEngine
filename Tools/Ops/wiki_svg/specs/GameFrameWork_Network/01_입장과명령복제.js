// 1-1. 입장부터 명령 복제까지 — 클라 세션 · 호스트 세션 · 호스트 규칙 커널 사이의 메시지 차례
// 옛 Server_01_입장과첫명령을 새 문서로 옮겨 다시 그림(월드 이동 Travel 을 앞에 더함)
// 코드: Network/Session/GameplayHostSession.cpp · GameplayClientSession.cpp, Network/Messages/GameplayNetMessage.h(EGameplayNetMessage · Version 2)
module.exports = {
  file: 'GameFrameWork_Network_01_입장과명령복제',
  alt: '입장부터 명령 복제까지 — 클라 세션이 Hello로 입장해 Welcome · SlotChanged를 받고, 호스트의 Travel로 같은 월드를 로드한 뒤, StartGame으로 같은 시드 · 초기 상태에서 시작하고, CommandRequest · CommandAccepted로 명령을 같은 순서로 복제 실행하며, 순번 · 체크섬이 다르면 ResyncRequest · Document로 재동기한다',
  type: 'seq',
  colW: 300,
  participants: [
    { id: 'C', label: 'PGameplayClientSession', sub: '클라 세션', color: 'orange' },
    { id: 'H', label: 'PGameplayHostSession', sub: '호스트 세션', color: 'orange' },
    { id: 'G', label: 'PGameMaster', sub: '호스트 규칙 커널', color: 'yellow' }
  ],
  steps: [
    { from: 'C', to: 'H', text: 'Hello(프로토콜, 스키마, 이름, 재접속 토큰)' },
    { from: 'H', to: 'H', text: '프로토콜 · 스키마 검사, 자리 배정' },
    { from: 'H', to: 'C', text: 'Welcome(슬롯, 규칙 지문, 월드, 슬롯 표)', dash: true },
    { from: 'H', to: 'C', text: 'SlotChanged · Travel(월드)', dash: true },
    { from: 'C', to: 'C', text: '같은 월드 로드 · 보관 메시지 적용' },
    { from: 'H', to: 'G', text: '엔트리 액터의 StartGame(seed)' },
    { from: 'H', to: 'C', text: 'StartGame(시드, 지문, 초기 상태)', dash: true },
    { from: 'C', to: 'C', text: '지문 확인 → 같은 초기 상태로 Start' },
    { from: 'C', to: 'H', text: 'CommandRequest(명령)' },
    { from: 'H', to: 'G', text: '소유 검사 → Submit(명령)' },
    { from: 'H', to: 'C', text: 'CommandAccepted(순번, 체크섬) — 전원', dash: true },
    { from: 'C', to: 'C', text: '복제본 Submit → 순번 · 체크섬 비교' },
    { note: '다르면 ResyncRequest → Document(초기 + 현재 + 명령 기록)로 재동기', over: ['C', 'H'] }
  ]
};
