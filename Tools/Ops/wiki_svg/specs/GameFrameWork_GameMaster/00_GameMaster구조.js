// 0-1. PGameMaster와 HGameplayState — 위 = 파사드 · 값 타입 상태 · 서비스, 가운데 = 규칙 엔진과 게임이 파생하는 규칙 클래스 · 확장 인터페이스, 아래 = 관찰자 · 에이전트
// 코드: GameMaster/GameMaster.h(_initialState · _state · _engine · _snapshots · _log · _observers), State/GameplayState.h,
//       Rules/GameplayRuleEngine.h(Handlers · Effects · Triggers · Modifiers · ValuePipeline · OrderPolicy · Board · Flow), Rules/GameplayFlow.h · GameplayRoundTurnFlow.h,
//       Boards/GameplayBoard.h, Services/GameplaySnapshotStack.h · GameplayCommandLog.h · GameplaySerializer.h · GameplayObserver.h, Agents/GameplayAgent.h
module.exports = {
  file: 'GameFrameWork_GameMaster_00_GameMaster구조',
  alt: 'PGameMaster와 HGameplayState — 게임이 사용하는 단일 입구 PGameMaster는 초기 · 현재 HGameplayState, PGameplayRuleEngine, 스냅샷 스택 · 명령 기록 · 직렬화 서비스를 소유하고, 규칙 엔진은 게임이 파생한 명령 핸들러 · 효과 · 트리거 · 수정자를 등록부에 보관하며 흐름 · 보드 · 순서 정책을 사용하고, 실행 결과 이벤트는 관찰자에게 전달하며 AI 에이전트는 상태 사본에서 명령을 고른다',
  type: 'class',
  w: 1100,
  h: 690,
  classes: [
    { id: 'GM', x: 40, y: 40, w: 320, name: 'PGameMaster', stereo: '규칙 커널 · 단일 입구', color: 'yellow', members: ['+Register*() · SetFlow() · SetBoard()', '+EditInitialState() · Start(seed)', '+Submit(command) · Validate()', '+Undo() · Save() · Load() · Replay()', '+Checksum() · RulesFingerprint()'] },
    { id: 'ST', x: 420, y: 40, w: 300, name: 'HGameplayState', stereo: '값 타입 상태 · 복사 = 스냅샷', color: 'yellow', members: ['Entities · Zones · Board', 'Turn · Choice', 'Random  // 용도별 난수 스트림', 'Tables  // 컴포넌트 표', 'RewindPoints  // 되감기 지점'] },
    { id: 'SV', x: 780, y: 40, w: 280, name: '서비스', stereo: 'PGameMaster가 소유', color: 'gray', stack: 1, members: ['PGameplaySnapshotStack  // Undo', 'PGameplayCommandLog  // 명령 기록', 'PGameplaySerializer  // 저장 문서'] },
    { id: 'ENG', x: 40, y: 310, w: 320, name: 'PGameplayRuleEngine', stereo: '규칙 엔진 · 상태를 소유하지 않음', color: 'orange', members: ['Handlers · Effects  // 등록부', 'Triggers · Modifiers  // 등록부', 'ValuePipeline  // 수치 단계', '-_queue  // 효과 큐'] },
    { id: 'RL', x: 420, y: 310, w: 300, name: '규칙 클래스', stereo: '게임이 파생 · 리플렉션 등록', color: 'green', stack: 1, members: ['JGGameplayCommandHandler  // 명령', 'JGGameplayEffect  // 효과', 'JGGameplayTrigger  // 이벤트 반응', 'JGGameplayModifier  // 수치 수정'] },
    { id: 'EX', x: 780, y: 310, w: 280, name: '확장 인터페이스', stereo: '게임이 교체 · 엔진 기본 구현', color: 'purple', stack: 1, members: ['IGameplayFlow  // 턴 · 페이즈', 'IGameplayBoard  // Square · Hex · Free', 'IGameplayOrderPolicy  // 행동 순서', 'IGameplayMigrator  // 저장 버전 변환'] },
    { id: 'OB', x: 40, y: 580, w: 320, name: 'IGameplayObserver', stereo: '이벤트 관찰자', color: 'blue', members: ['+OnEvents(events)'] },
    { id: 'AG', x: 420, y: 580, w: 300, name: 'IGameplayAgent', stereo: 'AI 에이전트 · Random · Greedy', color: 'blue', members: ['+Choose(state, actor)'] }
  ],
  nodes: [
    { id: 'KEY', x: 780, y: 580, w: 280, h: 96, color: 'white', shape: 'note', title: '범례', lines: ['◇— 소유 · → 사용 · 호출', '겹친 카드 = 여러 개', '+ public · - private'] }
  ],
  edges: [
    { from: 'GM', to: 'ST', fs: 'r', ts: 'l', fo: -40, to_off: -40, start: 'diamond', end: 'none', label: '초기 · 현재' },
    { points: [[300, 40], [300, 16], [920, 16], [920, 40]], start: 'diamond', end: 'none', label: '소유', lx: 610, ly: 16, lpos: 'b' },
    { from: 'GM', to: 'ENG', fs: 'b', ts: 't', start: 'diamond', end: 'none', label: 'Submit 시 실행' },
    { from: 'ENG', to: 'RL', fs: 'r', ts: 'l', fo: -30, to_off: -30, start: 'diamond', end: 'none', label: '1 : N' },
    { points: [[200, 474], [200, 530], [920, 530], [920, 474]], label: '사용', lx: 560, ly: 530 },
    { points: [[40, 130], [24, 130], [24, 620], [40, 620]], dash: true, label: '이벤트 전달', lx: 24, ly: 560, lpos: 'r' },
    { points: [[720, 620], [750, 620], [750, 260], [300, 260], [300, 184]], dash: true, label: '상태 사본으로 Simulate', lx: 525, ly: 260 }
  ]
};
