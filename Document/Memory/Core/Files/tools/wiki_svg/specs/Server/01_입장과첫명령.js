// 입장부터 첫 명령까지
// Mermaid 의 클라 자기 처리 "같은 월드 로드 → GameMasterActor 가 붙으면 보관 메시지 적용" 은 두 단계로,
// 마지막 "(다르면 ResyncRequest → Document)" 는 메모로 나눠 그렸다.
module.exports = {
  file: 'Server_01_입장과첫명령',
  alt: '입장부터 첫 명령까지 — 클라 세션이 Hello로 입장해 Welcome · SlotChanged를 받고, 호스트의 StartGame으로 같은 시드 · 초기 상태에서 시작한 뒤, CommandRequest · CommandAccepted로 명령을 복제 실행하며 순번 · 체크섬이 다르면 Document로 재동기한다',
  type: 'seq',
  colW: 300,
  participants: [
    { id: 'C', label: 'PGameplayClientSession', sub: '클라 세션', color: 'orange' },
    { id: 'H', label: 'PGameplayHostSession', sub: '호스트 세션', color: 'orange' },
    { id: 'G', label: 'PGameMaster', sub: '호스트 규칙 커널', color: 'yellow' }
  ],
  steps: [
    { from: 'C', to: 'H', text: 'Hello(프로토콜 2, 스키마, 이름, 토큰)' },
    { from: 'H', to: 'H', text: '프로토콜 · 스키마 검사, 토큰으로 자리 예약' },
    { from: 'H', to: 'C', text: 'Welcome(슬롯, 지문, 월드, 슬롯 표, 방 정보)', dash: true },
    { from: 'H', to: 'C', text: 'SlotChanged — 전원에게', dash: true },
    { from: 'C', to: 'C', text: '같은 월드 로드' },
    { from: 'C', to: 'C', text: 'GameMasterActor 연결 시 보관 메시지 적용' },
    { from: 'H', to: 'G', text: '엔트리 액터가 StartGame(seed) 호출' },
    { from: 'H', to: 'C', text: 'StartGame(시드, 지문, 초기 상태)', dash: true },
    { from: 'C', to: 'C', text: '지문 확인 → 초기 상태 Import → Start(seed)' },
    { from: 'C', to: 'H', text: 'CommandRequest(명령)' },
    { from: 'H', to: 'G', text: 'Submit(명령)' },
    { from: 'H', to: 'C', text: 'CommandAccepted(Seq, Checksum)', dash: true },
    { from: 'C', to: 'C', text: '복제본 Submit → Seq · Checksum 비교' },
    { note: '다르면 ResyncRequest → Document로 재동기', over: ['C', 'H'] }
  ]
};
