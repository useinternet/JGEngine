// 6. 네트워크 — 리슨 서버: 입장 → 시작 → 명령 중계 → (어긋나면) 재동기
module.exports = {
  file: 'JGEngine구조도_06_리슨서버',
  alt: '네트워크 — 리슨 서버 — 클라 세션이 Hello로 입장해 Welcome을 받고 같은 월드를 로드하며, 호스트 PGameMaster가 Start(seed)한 초기 상태를 StartGame으로 받아 같이 시작하고, 명령은 호스트 도착 순으로 Submit된 뒤 CommandAccepted(Seq, Checksum)로 돌아와 클라 복제본에서 비교되며, 어긋나면 ResyncRequest로 Document를 받아 재동기한다',
  type: 'seq',
  colW: 300,
  participants: [
    { id: 'C', label: 'PGameplayClientSession', sub: '클라이언트 세션', color: 'blue' },
    { id: 'H', label: 'PGameplayHostSession', sub: '호스트 세션', color: 'orange' },
    { id: 'G', label: 'PGameMaster', sub: '호스트 규칙 커널', color: 'green' }
  ],
  steps: [
    { from: 'C', to: 'H', text: 'Hello(프로토콜 2, 이름, 재접속 토큰)' },
    { from: 'H', to: 'C', text: 'Welcome(슬롯, 규칙 지문, 월드, 슬롯 표, 방 정보)', dash: true },
    { from: 'C', to: 'C', text: '호스트와 같은 월드 로드' },
    { gap: 8 },
    { from: 'H', to: 'G', text: 'Start(seed) — 엔트리 액터가 권한 쪽에서만' },
    { from: 'H', to: 'C', text: 'StartGame(시드, 지문, 초기 상태)', dash: true },
    { from: 'C', to: 'C', text: '같은 초기 상태로 Start(seed)' },
    { gap: 8 },
    { from: 'C', to: 'H', text: 'CommandRequest(명령)' },
    { from: 'H', to: 'G', text: 'Submit — 순서는 호스트 도착 순' },
    { from: 'H', to: 'C', text: 'CommandAccepted(Seq, Checksum)', dash: true },
    { from: 'C', to: 'C', text: '복제 GameMaster에 같은 명령 → Seq · Checksum 비교' },
    { gap: 8 },
    { from: 'C', to: 'H', text: '(어긋나면) ResyncRequest' },
    { from: 'H', to: 'C', text: 'Document(초기 + 현재 + 명령 기록)', dash: true }
  ]
};
