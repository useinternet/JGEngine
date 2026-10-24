// 1-1. 명령 처리 과정 — 가운데 세로줄 = Submit 한 번의 차례, 오른쪽 = 효과 큐 루프와 멈추는 곳, 왼쪽 = 거절
// 코드: GameMaster/GameMaster.cpp(Submit — Validate → 스냅샷 → Execute → 명령 기록 → 관찰자), Rules/GameplayRuleEngine.h(명령 → 핸들러 → 효과 큐 루프 → 트리거 → 흐름 단계),
//       Rules/GameplayEffectQueue.h · GameplayTriggerDispatcher.h · GameplayValuePipeline.h · GameplayFlow.h
module.exports = {
  file: 'GameFrameWork_GameMaster_01_명령처리과정',
  alt: '명령 처리 과정 — PGameMaster::Submit은 규칙 엔진의 Validate로 명령을 검사해 틀리면 거절하고, 상태 스냅샷을 쌓은 뒤 Execute에서 핸들러가 효과를 큐에 넣으면 방금 낸 이벤트의 트리거 반응과 효과 하나 해결(수정자 · 수치 단계)을 되풀이하며, 효과가 선택을 요청하면 선택 대기에서 멈추고, 큐가 비면 흐름의 예약된 단계로 다음 대기 지점까지 진행한 뒤 명령을 기록하고 이벤트 목록을 관찰자에게 전달한다',
  type: 'flow',
  w: 980,
  h: 820,
  nodes: [
    { id: 'S', x: 300, y: 20, w: 320, h: 56, color: 'yellow', title: 'PGameMaster::Submit(command)', lines: ['세션만 호출'] },
    { id: 'V', x: 330, y: 110, w: 260, h: 90, color: 'white', shape: 'diamond', title: 'Validate', lines: ['핸들러 · 차례 · 선택 대기'] },
    { id: 'R', x: 30, y: 128, w: 220, h: 56, color: 'red', title: 'Rejected', lines: ['상태 변화 없음 · 이유'] },
    { id: 'P', x: 300, y: 240, w: 320, h: 56, color: 'gray', title: 'PGameplaySnapshotStack::Push', lines: ['Undo용 상태 스냅샷'] },
    { id: 'H', x: 300, y: 336, w: 320, h: 56, color: 'green', title: 'JGGameplayCommandHandler', lines: ['명령 실행 · 효과 요청을 큐에'] },
    { id: 'T', x: 300, y: 432, w: 320, h: 56, color: 'green', title: 'JGGameplayTrigger', lines: ['방금 낸 이벤트에 반응 · 효과 요청'] },
    { id: 'E', x: 300, y: 528, w: 320, h: 70, color: 'green', title: 'JGGameplayEffect 하나 해결', lines: ['JGGameplayModifier · 수치 단계 적용', '이벤트 생성'] },
    { id: 'Q', x: 330, y: 632, w: 260, h: 80, color: 'white', shape: 'diamond', title: '효과 큐가 비었는가' },
    { id: 'F', x: 680, y: 640, w: 270, h: 64, color: 'purple', title: 'IGameplayFlow 예약 단계', lines: ['턴 · 페이즈 진행', '다음 대기 지점까지'] },
    { id: 'C', x: 680, y: 528, w: 270, h: 70, color: 'orange', title: '선택 대기에서 멈춤', lines: ['효과가 RequestChoice', '응답 명령으로 재개'] },
    { id: 'L', x: 300, y: 748, w: 320, h: 56, color: 'yellow', title: '명령 기록 · 관찰자에게 이벤트 전달', lines: ['PGameplayCommandLog · IGameplayObserver'] }
  ],
  edges: [
    { from: 'S', to: 'V' },
    { from: 'V', to: 'R', fs: 'l', ts: 'r', label: '틀림' },
    { from: 'V', to: 'P', fs: 'b', ts: 't', label: '맞음' },
    { from: 'P', to: 'H' },
    { from: 'H', to: 'T' },
    { from: 'T', to: 'E' },
    { from: 'E', to: 'C', fs: 'r', ts: 'l', label: '선택 요청', dash: true },
    { from: 'E', to: 'Q' },
    { points: [[330, 672], [270, 672], [270, 460], [300, 460]], label: '아니오 — 다음 효과', lx: 270, ly: 560, lpos: 'l' },
    { from: 'Q', to: 'F', fs: 'r', ts: 'l', label: '예' },
    { from: 'F', to: 'L', fs: 'b', ts: 'r', via: [[815, 776]] }
  ]
};
