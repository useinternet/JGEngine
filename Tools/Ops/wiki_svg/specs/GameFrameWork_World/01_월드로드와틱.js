// 0-2. 월드 로드와 틱 — 왼쪽 세로줄 = LoadWorld 순서, 오른쪽 세로줄 = 프레임마다 Tick 순서
// 코드: Core/GameInstance.cpp(LoadWorld · UnloadWorld · Tick), Core/World.cpp(BeginPlay · Tick — 스폰 · 파괴는 틱 끝에 모아서)
module.exports = {
  file: 'GameFrameWork_World_01_월드로드와틱',
  alt: '월드 로드와 틱 — LoadWorld는 활성 월드를 언로드하고 새 PWorld를 생성한 뒤, 호스트 세션이면 Travel을 먼저 알리고, 엔트리 액터를 스폰해 OnEnterWorld를 호출한 다음 OnWorldLoaded를 알리고 BeginPlay를 호출하며, Tick은 세션 → 클라의 월드 이동 → 월드 → 게임 UI 순서로 진행한다',
  type: 'flow',
  w: 960,
  h: 640,
  groups: [
    { x: 20, y: 20, w: 450, h: 600, label: 'LoadWorld(name)' },
    { x: 490, y: 20, w: 450, h: 400, label: 'Tick(deltaSeconds) — 프레임마다' }
  ],
  nodes: [
    { id: 'L1', x: 60, y: 60, w: 370, h: 56, color: 'gray', title: 'UnloadWorld', lines: ['OnWorldUnloading → EndPlay'] },
    { id: 'L2', x: 60, y: 150, w: 370, h: 56, color: 'green', title: 'Allocate<PWorld>(name)', lines: ['새 월드 생성'] },
    { id: 'L3', x: 60, y: 240, w: 370, h: 56, color: 'orange', title: 'NotifyTravel', lines: ['호스트 세션이면 참가자에게 월드 이동 알림'] },
    { id: 'L4', x: 60, y: 330, w: 370, h: 56, color: 'green', title: '엔트리 액터 스폰 · OnEnterWorld', lines: ['SetEntryClass로 정한 클래스'] },
    { id: 'L5', x: 60, y: 420, w: 370, h: 56, color: 'blue', title: 'OnWorldReady · OnWorldLoaded', lines: ['게임 인스턴스 훅 · 대리자'] },
    { id: 'L6', x: 60, y: 510, w: 370, h: 56, color: 'green', title: 'PWorld::BeginPlay', lines: ['액터 OnBeginPlay'] },
    { id: 'T1', x: 530, y: 60, w: 370, h: 56, color: 'orange', title: 'PGameplaySession::Tick', lines: ['수신 · 명령 승인 · 복제 실행'] },
    { id: 'T2', x: 530, y: 150, w: 370, h: 56, color: 'orange', title: '월드 이동', lines: ['클라가 호스트의 Travel을 받았을 때만'] },
    { id: 'T3', x: 530, y: 240, w: 370, h: 56, color: 'green', title: 'PWorld::Tick', lines: ['스폰 반영 → 액터 OnTick → 파괴 반영'] },
    { id: 'T4', x: 530, y: 330, w: 370, h: 56, color: 'purple', title: 'PGameUIManager::Update', lines: ['활성 위젯 OnUpdate · 트윈'] }
  ],
  edges: [
    { from: 'L1', to: 'L2' },
    { from: 'L2', to: 'L3' },
    { from: 'L3', to: 'L4' },
    { from: 'L4', to: 'L5' },
    { from: 'L5', to: 'L6' },
    { from: 'T1', to: 'T2' },
    { from: 'T2', to: 'T3' },
    { from: 'T3', to: 'T4' }
  ]
};
