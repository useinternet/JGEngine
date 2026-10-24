// 1-1. JGActor와 JGActorComponent — 왼쪽 = 액터 기반과 엔진이 제공하는 파생, 오른쪽 = 컴포넌트 기반과 파생, 아래 = 컴포넌트가 다루는 Graphics 장면
// 코드: Actors/Actor.h · GameEntryActor.h · GameMasterActor.h · GameplayControllerActor.h · GameplayEntityActor.h · CameraActor.h,
//       Components/ActorComponent.h · CameraComponent.h · StaticMeshComponent.h · PickShapeComponent.h · GameplayEntityComponent.h
module.exports = {
  file: 'GameFrameWork_World_02_액터컴포넌트구조',
  alt: 'JGActor와 JGActorComponent — JGActor는 트랜스폼 · 자식 액터 · 컴포넌트를 소유하고 OnSpawned · OnBeginPlay · OnTick · OnEndPlay 훅을 가지며, 엔트리 · 게임 마스터 · 컨트롤러 · 엔티티 · 카메라 액터가 이를 상속하고, 카메라 · 정적 메시 · 피킹 판정 · 엔티티 컴포넌트는 JGActorComponent를 상속해 PScene의 카메라 · 메시 배치를 번호로 다룬다',
  type: 'class',
  w: 1100,
  h: 570,
  classes: [
    { id: 'ACT', x: 40, y: 30, w: 440, name: 'JGActor', stereo: '액터 · 월드에 놓이는 것', color: 'green', members: ['LocalTransform  // 부모 기준', '-_children · _components', '+AddComponent<T>() · AttachTo()', '#OnSpawned() · OnBeginPlay()', '#OnTick() · OnEndPlay()'] },
    { id: 'CMP', x: 620, y: 30, w: 440, name: 'JGActorComponent', stereo: '컴포넌트 · 액터에 붙는 기능', color: 'blue', members: ['+GetOwner()', '#OnBeginPlay() · OnTick()', '#OnEndPlay()'] },
    { id: 'DA', x: 40, y: 240, w: 440, name: '엔진 액터', stereo: 'JGActor 파생', color: 'green', stack: 1, members: ['JGGameEntryActor  // 월드 진입점', 'JGGameMasterActor  // 규칙 연결 · 연출', 'JGGameplayControllerActor  // 입력 → 명령', 'JGGameplayEntityActor  // 엔티티의 몸', 'JGCameraActor  // 카메라 · Fixed · Orbit'] },
    { id: 'DC', x: 620, y: 240, w: 440, name: '엔진 컴포넌트', stereo: 'JGActorComponent 파생', color: 'blue', stack: 1, members: ['JGCameraComponent  // 원근 카메라', 'JGStaticMeshComponent  // 정적 메시 배치', 'JGPickShapeComponent  // 피킹 판정 상자', 'JGGameplayEntityComponent  // 엔티티 바인딩'] }
  ],
  nodes: [
    { id: 'SCN', x: 620, y: 480, w: 440, h: 56, color: 'gray', title: 'PScene', lines: ['Graphics 장면 · 카메라 · 메시 배치를 번호로 보관'] },
    { id: 'KEY', x: 40, y: 460, w: 440, h: 96, color: 'white', shape: 'note', title: '범례', lines: ['◇— 소유 · —▷ 상속 · → 사용', '겹친 카드 = 여러 개 · + public · # protected · - private'] }
  ],
  edges: [
    { points: [[480, 66], [620, 66]], start: 'diamond', end: 'none', label: '1 : N' },
    { from: 'DA', to: 'ACT', fs: 't', ts: 'b', end: 'triangle' },
    { from: 'DC', to: 'CMP', fs: 't', ts: 'b', end: 'triangle' },
    { from: 'DC', to: 'SCN', fs: 'b', ts: 't', label: '카메라 · 메시 배치를 번호로 생성 · 갱신', lw: 300 }
  ]
};
