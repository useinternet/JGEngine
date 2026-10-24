// 0. GameFrameWork 모듈 — 사용하는 모듈 / GameFrameWork.dll 분야 지도(분야 문서마다 묶음 하나) / 의존 모듈
// 2026-10-24 2단계 모듈 정리(Asset · Audio 합침, 자체 시험 모음, 이름 GameFrameWork) 뒤 새 문서
// 코드: Runtime/GameFrameWork/Core/GameFrameWorkModule.h · GameInstance.h · World.h, Actors/Actor.h, Components/ActorComponent.h, Viewport/GameViewport.h,
//       GameMaster/GameMaster.h, Network/Session/GameplaySession.h, Network/Transport/NetTransport.h, UI/GameUIManager.h, Data/DataTable.h,
//       Asset/AssetDatabase.h · Asset.h, Audio/Audio.h, SelfTest/GameFrameWorkSelfTest.cpp, GameFrameWork.module.json
module.exports = {
  file: 'GameFrameWork_00_GameFrameWork모듈',
  alt: 'GameFrameWork 모듈 — GameFrameWork.dll은 월드와 액터(HGameFrameWorkModule, JGGameInstance, PWorld, JGActor · JGActorComponent, PGameViewport), 규칙 커널(PGameMaster) · 네트워크(PGameplaySession, INetTransport), 게임 UI(PGameUIManager) · 데이터 테이블(JGDataTable) · 에셋(GAssetDatabase · JGAsset) · 오디오(HAudio), 자체 시험(gfw.selftest) 분야로 나뉘며, JGEditor · GameHost · JGConsole · 게임 모듈이 사용하고 Core와 Graphics에 의존한다',
  type: 'class',
  w: 1100,
  h: 980,
  groups: [
    { x: 20, y: 20, w: 1060, h: 110, label: 'GameFrameWork를 사용하는 모듈', color: 'purple' },
    { x: 20, y: 170, w: 1060, h: 640, label: 'GameFrameWork 모듈 (GameFrameWork.dll)' },
    { x: 40, y: 210, w: 1020, h: 170, label: '월드와 액터', dash: true },
    { x: 40, y: 410, w: 1020, h: 170, label: '규칙 커널(GameMaster) · 네트워크', dash: true },
    { x: 40, y: 610, w: 820, h: 180, label: '게임 UI · 데이터 테이블 · 에셋 · 오디오', dash: true },
    { x: 880, y: 610, w: 180, h: 180, label: '자체 시험', dash: true },
    { x: 20, y: 850, w: 700, h: 110, label: 'GameFrameWork가 의존하는 모듈' }
  ],
  classes: [
    // 런타임 기반
    { id: 'MOD', x: 60, y: 250, w: 180, name: 'HGameFrameWorkModule', stereo: '모듈 클래스', color: 'blue', members: ['#StartupModule()', '-tick()'] },
    { id: 'GI', x: 260, y: 250, w: 180, name: 'JGGameInstance', stereo: '게임 인스턴스', color: 'blue', members: ['+LoadWorld()', '+GetSession() · GetUI()'] },
    { id: 'W', x: 460, y: 250, w: 180, name: 'PWorld', stereo: '월드', color: 'green', members: ['+SpawnActor()', '+GetScene()'] },
    { id: 'ACT', x: 660, y: 250, w: 180, name: 'JGActor', stereo: '액터 · 컴포넌트', color: 'green', members: ['+AddComponent()', '#OnBeginPlay() 등'] },
    { id: 'VP', x: 860, y: 250, w: 180, name: 'PGameViewport', stereo: '게임 뷰포트', color: 'green', members: ['+Render()', '+HandleInput()'] },
    // 규칙 · 네트워크
    { id: 'GM', x: 60, y: 450, w: 280, name: 'PGameMaster', stereo: '규칙 커널', color: 'yellow', members: ['+Start(seed)', '+Submit(command)'] },
    { id: 'SES', x: 400, y: 450, w: 280, name: 'PGameplaySession', stereo: '세션 · 호스트 · 클라', color: 'orange', members: ['+SubmitLocal(command)', '+Tick()'] },
    { id: 'NET', x: 740, y: 450, w: 300, name: 'INetTransport', stereo: '전송 · TCP · 루프백', color: 'orange', members: ['+Listen() · Connect()', '+Send() · Poll()'] },
    // 화면 · 데이터 · 에셋 · 소리
    { id: 'UI', x: 60, y: 650, w: 180, name: 'PGameUIManager', stereo: '게임 UI', color: 'purple', members: ['+PushWidget<T>()', '+Render()'] },
    { id: 'DT', x: 260, y: 650, w: 180, name: 'JGDataTable', stereo: '데이터 테이블', color: 'purple', members: ['HDataTableView<T>'] },
    { id: 'AST', x: 460, y: 650, w: 180, name: 'GAssetDatabase', stereo: '에셋 DB · JGAsset', color: 'purple', members: ['+LoadAssetAsync()'] },
    { id: 'AUD', x: 660, y: 650, w: 180, name: 'HAudio', stereo: '오디오', color: 'purple', members: ['+Play() · PlayMusic()'] },
    // 자체 시험
    { id: 'ST', x: 895, y: 650, w: 150, name: 'gfw.selftest', stereo: '시험 그룹 8개', color: 'gray', members: ['별칭 6개'] }
  ],
  nodes: [
    { id: 'U1', x: 40, y: 56, w: 240, h: 56, color: 'purple', title: 'JGEditor', lines: ['씬 뷰포트 · 도구 창'] },
    { id: 'U2', x: 300, y: 56, w: 240, h: 56, color: 'purple', title: 'GameHost', lines: ['게임만 실행 · 창 전체'] },
    { id: 'U3', x: 560, y: 56, w: 240, h: 56, color: 'purple', title: 'JGConsole', lines: ['헤드리스 시험 · 쿠킹'] },
    { id: 'U4', x: 820, y: 56, w: 240, h: 56, color: 'purple', title: '게임 모듈', lines: ['엔트리 액터 · 규칙 · 화면'] },
    { id: 'CORE', x: 40, y: 886, w: 320, h: 56, color: 'gray', title: 'Core', lines: ['모듈 · 메모리 · 리플렉션 · 스케줄'] },
    { id: 'GFX', x: 380, y: 886, w: 320, h: 56, color: 'gray', title: 'Graphics', lines: ['장면 · 렌더러 · GPU 리소스'] },
    { id: 'KEY', x: 760, y: 850, w: 300, h: 110, color: 'white', shape: 'note', title: '범례', lines: ['◇— 소유 · → 사용 · 호출', '+ public · # protected · - private'] }
  ],
  edges: [
    { points: [[550, 130], [550, 170]], label: '사용' },
    { points: [[370, 810], [370, 850]], label: '의존' },
    { from: 'MOD', to: 'GI', fs: 'r', ts: 'l', start: 'diamond', end: 'none' },
    { from: 'GI', to: 'W', fs: 'r', ts: 'l', start: 'diamond', end: 'none' },
    { from: 'W', to: 'ACT', fs: 'r', ts: 'l', start: 'diamond', end: 'none' },
    { from: 'SES', to: 'GM', fs: 'l', ts: 'r', label: 'Submit' },
    { from: 'SES', to: 'NET', fs: 'r', ts: 'l', start: 'diamond', end: 'none' }
  ]
};
