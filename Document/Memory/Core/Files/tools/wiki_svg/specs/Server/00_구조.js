// 구조 — 위 묶음 = 호스트, 아래 묶음 = 클라. 두 세션 사이 세로선 3개: 왼쪽 위로 가는 요청(글은 선 왼쪽), 가운데 · 오른쪽 아래로 오는 승인 · 문서(글은 선 오른쪽).
// Mermaid 의 호스트 상자 "컨트롤러 · AI 에이전트" 는 클래스 이름(위) / 한글 설명(아래) 규칙에 맞춰 두 상자(컨트롤러 · AI 에이전트)로 나눴다.
module.exports = {
  file: 'Server_00_구조',
  alt: '구조 — 클라 세션 PGameplayClientSession은 CommandRequest를 호스트 세션 PGameplayHostSession에 보내고, 호스트 세션은 권한 쪽 PGameMaster에서 실행한 뒤 CommandAccepted(순번 + 체크섬)와 입장 · 재동기용 Document를 보내 클라의 복제 PGameMaster가 같은 순서로 실행한다',
  type: 'flow',
  w: 960,
  h: 470,
  groups: [
    { x: 20, y: 20, w: 920, h: 200, label: '호스트 (ListenServer)', color: 'blue' },
    { x: 20, y: 330, w: 920, h: 120, label: '클라 (Client)' }
  ],
  nodes: [
    { id: 'HC', x: 40, y: 60, w: 220, h: 56, color: 'green', title: 'JGGameplayControllerActor', lines: ['컨트롤러'] },
    { id: 'AI', x: 40, y: 136, w: 220, h: 56, color: 'gray', title: 'IGameplayAgent', lines: ['AI 에이전트 · 호스트가 구동'] },
    { id: 'HS', x: 320, y: 98, w: 320, h: 56, color: 'orange', title: 'PGameplayHostSession', lines: ['호스트 세션 · 명령 승인 · 방송'] },
    { id: 'HG', x: 700, y: 98, w: 220, h: 56, color: 'yellow', title: 'PGameMaster', lines: ['권한 쪽 규칙 커널'] },
    { id: 'CC', x: 40, y: 370, w: 220, h: 56, color: 'green', title: 'JGGameplayControllerActor', lines: ['컨트롤러'] },
    { id: 'CS', x: 320, y: 370, w: 320, h: 56, color: 'orange', title: 'PGameplayClientSession', lines: ['클라 세션 · 복제 실행'] },
    { id: 'CG', x: 700, y: 370, w: 220, h: 56, color: 'yellow', title: 'PGameMaster', lines: ['복제 규칙 커널'] }
  ],
  edges: [
    { from: 'HC', to: 'HS', fs: 'r', ts: 'l', to_off: -10, mx: 290 },
    { from: 'AI', to: 'HS', fs: 'r', ts: 'l', to_off: 10, mx: 290 },
    { from: 'HS', to: 'HG', fs: 'r', ts: 'l' },
    { from: 'CC', to: 'CS', fs: 'r', ts: 'l' },
    { from: 'CS', to: 'CG', fs: 'r', ts: 'l' },
    { from: 'CS', to: 'HS', fs: 't', ts: 'b', fo: -130, to_off: -130, label: 'CommandRequest', lx: 350, ly: 262, lpos: 'l' },
    { from: 'HS', to: 'CS', fs: 'b', ts: 't', fo: -10, to_off: -10, label: 'CommandAccepted — 순번 + 체크섬', lw: 120, lx: 470, ly: 262 },
    { from: 'HS', to: 'CS', fs: 'b', ts: 't', fo: 145, to_off: 145, label: 'Document — 입장 · 재동기', lx: 625, ly: 262 }
  ]
};
