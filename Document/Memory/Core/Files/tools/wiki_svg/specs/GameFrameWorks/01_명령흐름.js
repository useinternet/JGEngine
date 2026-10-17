// 명령 한 개가 흐르는 길
// Mermaid 의 V→C "HandleClick(ray) → Pick → 게임의 OnClick" 은 호출(V→C)과 컨트롤러 안 처리(C 자기 자신)로 나눴다.
// PGameMaster 안 처리(검증 → … → 흐름 단계)는 맨 오른쪽 칸이라 자기 화살표 글이 캔버스를 넘어서 메모로 그렸다.
// 칸 폭 214 = JGGameplayControllerActor 이름이 들어가는 최소. w 는 오른쪽 메모 끝(1086)에 맞춰 줄였다.
module.exports = {
  file: 'GameFrameWorks_01_명령흐름',
  alt: '명령 한 개가 흐르는 길 — PGameViewport 입력이 컨트롤러 액터의 OnClick에서 명령으로 작성되어 JGGameMasterActor · 세션을 거쳐 PGameMaster에서 검증 · 실행되고, 관찰자로 받은 이벤트 목록은 연출 큐로 재생된다',
  type: 'seq',
  colW: 214,
  w: 1100,
  participants: [
    { id: 'V', label: 'PGameViewport', sub: '게임 화면 · 입력 라우팅', color: 'blue' },
    { id: 'C', label: 'JGGameplayControllerActor', sub: '컨트롤러 액터', color: 'green' },
    { id: 'A', label: 'JGGameMasterActor', sub: '게임 마스터 액터', color: 'green' },
    { id: 'S', label: 'PGameplaySession', sub: '세션 · 기본 Standalone', color: 'orange' },
    { id: 'G', label: 'PGameMaster', sub: '규칙 커널', color: 'yellow' }
  ],
  steps: [
    { gap: 14 },
    { from: 'V', to: 'V', text: '게임 UI 우선, 미처리 시 카메라 광선' },
    { from: 'V', to: 'C', text: 'HandleClick(ray)' },
    { from: 'C', to: 'C', text: 'Pick → 게임의 OnClick 호출' },
    { from: 'C', to: 'A', text: 'BeginCommand · AddTarget · Commit → Submit' },
    { from: 'A', to: 'A', text: '입력 정책 — Block · Buffer · Skip' },
    { from: 'A', to: 'S', text: 'SubmitLocal(명령)' },
    { from: 'S', to: 'G', text: 'Submit (호스트일 때)' },
    { note: '클라 세션은 호스트로 CommandRequest 전송', over: ['S', 'G'], color: 'gray' },
    { note: '검증 → 핸들러 → 효과 큐 → 트리거 → 흐름 단계', over: ['G'] },
    { gap: 6 },
    { from: 'G', to: 'A', text: '관찰자로 이벤트 목록 전달', dash: true },
    { from: 'A', to: 'A', text: 'JGGameplayCue 연출 큐 차례로 재생' },
    { from: 'A', to: 'A', text: '엔티티 액터 스폰 · 삭제' }
  ]
};
