// 0-1. PGameplaySession과 INetTransport — 위 = 게임 인스턴스 · 세션 · 규칙 커널, 가운데 = 호스트 · 클라 세션, 아래 = 전송 · 메시지 코덱
// 코드: Core/GameInstance.h(_session · HostSession · JoinSession · LeaveSession), Network/Session/GameplaySession.h · GameplayHostSession.h · GameplayClientSession.h,
//       Network/Transport/NetTransport.h · NetTcpTransport.h · NetLoopbackTransport.h, Network/Messages/GameplayNetMessage.h · GameplayPlayerSlot.h
module.exports = {
  file: 'GameFrameWork_Network_00_네트워크구조',
  alt: 'PGameplaySession과 INetTransport — JGGameInstance가 세션(기본 Standalone)을 소유하고, 호스트 세션 PGameplayHostSession(Standalone · ListenServer)과 클라 세션 PGameplayClientSession이 PGameplaySession을 상속해 PGameMaster에 명령을 실행하며, 세션은 바이트만 아는 INetTransport(TCP · 루프백)와 메시지 코덱 HGameplayNetCodec으로 통신한다',
  type: 'class',
  w: 1100,
  h: 650,
  classes: [
    { id: 'GI', x: 40, y: 30, w: 300, name: 'JGGameInstance', stereo: '게임 인스턴스', color: 'blue', members: ['+HostSession() · JoinSession()', '+LeaveSession()', '+IsAuthority()'] },
    { id: 'SES', x: 400, y: 30, w: 300, name: 'PGameplaySession', stereo: '세션 기반 · 권한 경로의 입구', color: 'orange', members: ['+SubmitLocal(command)', '+Tick()', '-_slots : HGameplayPlayerSlot[]', 'NetMode · State'] },
    { id: 'GM', x: 800, y: 30, w: 260, name: 'PGameMaster', stereo: '규칙 커널', color: 'yellow', members: ['+Submit() · Checksum()', '+ExportDocument()', '+ImportDocument()'] },
    { id: 'HOST', x: 400, y: 260, w: 300, name: 'PGameplayHostSession', stereo: '호스트 · Standalone · ListenServer', color: 'orange', members: ['-executeAuthoritative()', '+NotifyTravel(world)', '+SetSessionData()', 'AI 에이전트 구동'] },
    { id: 'CL', x: 760, y: 260, w: 300, name: 'PGameplayClientSession', stereo: '클라 · 복제 실행', color: 'orange', members: ['순번 · 체크섬 비교', '재동기 · 재접속', '+SetLocalReady()'] },
    { id: 'TR', x: 40, y: 480, w: 300, name: 'INetTransport', stereo: '전송 · 신뢰 · 순서 보장', color: 'purple', members: ['+Listen() · Connect()', '+Send() · Poll()'] },
    { id: 'IMP', x: 400, y: 480, w: 300, name: '전송 구현', stereo: 'INetTransport 상속', color: 'purple', stack: 1, members: ['PNetTcpTransport  // Winsock TCP', 'PNetLoopbackTransport  // 한 프로세스'] },
    { id: 'COD', x: 760, y: 480, w: 300, name: 'HGameplayNetCodec', stereo: '메시지 코덱', color: 'gray', members: ['EGameplayNetMessage  // 14종', '[종류][플래그][JSON 본문]'] }
  ],
  nodes: [
    { id: 'KEY', x: 40, y: 260, w: 300, h: 96, color: 'white', shape: 'note', title: '범례', lines: ['◇— 소유 · —▷ 상속 · → 사용', '겹친 카드 = 여러 개', '+ public · - private'] }
  ],
  edges: [
    { from: 'GI', to: 'SES', fs: 'r', ts: 'l', fo: -30, to_off: -30, start: 'diamond', end: 'none', label: '소유' },
    { from: 'SES', to: 'GM', fs: 'r', ts: 'l', fo: -30, to_off: -30, label: '명령 실행' },
    { from: 'HOST', to: 'SES', fs: 't', ts: 'b', end: 'triangle' },
    { points: [[910, 260], [910, 220], [650, 220], [650, 178]], end: 'triangle' },
    { points: [[400, 120], [372, 120], [372, 440], [190, 440], [190, 480]], start: 'diamond', end: 'none', label: '소유', lx: 280, ly: 440 },
    { from: 'IMP', to: 'TR', fs: 'l', ts: 'r', end: 'triangle' },
    { points: [[700, 150], [730, 150], [730, 450], [880, 450], [880, 480]], label: '인코딩 · 디코딩', lx: 810, ly: 450 }
  ]
};
