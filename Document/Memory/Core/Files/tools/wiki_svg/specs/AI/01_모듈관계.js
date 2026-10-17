// AI — 다른 모듈과의 관계: AI 모듈의 의존과, 게임 AI 에이전트가 실제로 있는 곳(GameFrameWorks GameMaster/Agents/)
// 근거: AI.module.json · Main.cpp · jgengine.lua, GameFrameWorks GameMaster/Agents/GameplayAgent.h,
//       Network/Session/GameplayHostSession.cpp driveAgents()(PGameplayAgentRunner::Choose)
module.exports = {
  file: 'AI_01_모듈관계',
  alt: '다른 모듈과의 관계 — AI 모듈은 Core.h만 include하고 Graphics는 module.json 의존뿐이며 쓰이는 곳이 없고, 게임 AI 에이전트는 GameFrameWorks의 IGameplayAgent와 구현 클래스 · PGameplayAgentRunner에 있으며 네트워크 호스트 세션이 Choose로 명령을 선택한다',
  type: 'class',
  w: 1000,
  h: 470,
  groups: [
    { x: 20, y: 20, w: 300, h: 130, label: 'AI 모듈 (AI.dll) — 쓰이는 곳 없음', color: 'purple' },
    { x: 20, y: 190, w: 960, h: 260, label: 'GameFrameWorks — 게임 AI 에이전트 (GameMaster/Agents/)' }
  ],
  nodes: [
    { id: 'AIM', x: 50, y: 62, w: 240, h: 64, color: 'purple', title: 'Main.cpp', lines: ['int dummy()만 있음', '모듈 클래스 없음'] },
    { id: 'CORE', x: 530, y: 30, w: 180, h: 50, color: 'blue', title: 'Core', lines: ['기반 모듈'] },
    { id: 'GFX', x: 530, y: 100, w: 180, h: 50, color: 'green', title: 'Graphics', lines: ['렌더링 모듈'] },
    { id: 'KEY', x: 740, y: 20, w: 240, h: 130, color: 'white', shape: 'note', title: '범례', lines: ['—▷ 상속', '→ 호출 · include', '- - → module.json 의존만', '+ public · - private'] }
  ],
  classes: [
    { id: 'HOST', x: 40, y: 240, w: 230, name: 'PGameplayHostSession', stereo: '네트워크 호스트 세션', members: ['-driveAgents()'], color: 'orange' },
    { id: 'RUN', x: 360, y: 240, w: 240, name: 'PGameplayAgentRunner', stereo: '에이전트로 명령 선택 · 실행', members: ['+Choose()', '+Step() · Run()'], color: 'blue' },
    { id: 'AG', x: 700, y: 240, w: 250, name: 'IGameplayAgent', stereo: '행동자 하나의 명령 선택', members: ['+ChooseCommand()', '+ChooseOption()'], color: 'green' },
    { id: 'RND', x: 480, y: 380, w: 220, name: 'PGameplayRandomAgent', stereo: '합법 수 중 무작위', color: 'green' },
    { id: 'GRD', x: 730, y: 380, w: 240, name: 'PGameplayGreedyAgent', stereo: '1수 탐욕 · 평가 함수로 선택', color: 'green' }
  ],
  edges: [
    { from: 'AIM', to: 'CORE', fs: 'r', ts: 'l', fo: -14, label: 'Core.h include', lx: 468, ly: 55 },
    { from: 'AIM', to: 'GFX', fs: 'r', ts: 'l', fo: 14, dash: true, label: 'module.json 의존만', lx: 468, ly: 125, lpos: 'b' },
    { from: 'HOST', to: 'RUN', fs: 'r', ts: 'l', to_off: -9, label: 'Choose 호출' },
    { from: 'RUN', to: 'AG', fs: 'r', ts: 'l', fo: -18, to_off: -18, label: 'ChooseCommand' },
    { from: 'RND', to: 'AG', fs: 't', ts: 'b', to_off: -60, end: 'triangle' },
    { from: 'GRD', to: 'AG', fs: 't', ts: 'b', to_off: 25, end: 'triangle' }
  ]
};
