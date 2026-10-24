// 0-1. JGGameInstance와 PWorld — 모듈 → 게임 인스턴스 → 월드 · 세션 · 게임 UI, 월드가 소유하는 것은 왼쪽 세로줄에서 갈라지는 목록으로.
// 옛 GameFrameWorks_00_런타임구조를 새 이름(GameFrameWork)으로 다시 그림. 실선 = 소유 · 생성, 점선 = 호출 · 이벤트.
// 코드: Core/GameFrameWorkModule.cpp(StartupModule · tick), Core/GameInstance.h(_world · _session · _ui · Tick), Core/World.h(_actors · _scene · _activeCamera)
const TRUNK = 375;   // PWorld 아래 세로줄
const kids = [
  { id: 'SC', y: 305, color: 'gray', title: 'PScene', lines: ['Graphics 장면 데이터'] },
  { id: 'ENT', y: 385, color: 'green', title: 'JGGameEntryActor', lines: ['엔트리 액터 · 게임이 파생'] },
  { id: 'CTL', y: 465, color: 'green', title: 'JGGameplayControllerActor', lines: ['컨트롤러 · 입력 → 명령'] },
  { id: 'GMA', y: 545, color: 'green', title: 'JGGameMasterActor', lines: ['게임 마스터 액터'] },
  { id: 'EA', y: 625, color: 'green', title: 'JGGameplayEntityActor', lines: ['엔티티 액터 (여러 개)'] },
  { id: 'CAM', y: 705, color: 'green', title: 'JGCameraActor', lines: ['카메라 액터 · Fixed · Orbit'] }
].map(k => Object.assign({ x: 400, w: 250, h: 50 }, k));

module.exports = {
  file: 'GameFrameWork_World_00_런타임구조',
  alt: 'JGGameInstance와 PWorld — HGameFrameWorkModule이 매 프레임 Tick하는 JGGameInstance가 PWorld · PGameplaySession · PGameUIManager를 소유하고, PWorld는 PScene과 엔트리 · 컨트롤러 · 게임 마스터 · 엔티티 · 카메라 액터를 소유하며, 컨트롤러가 제출한 명령은 JGGameMasterActor와 세션을 거쳐 PGameMaster로, 결과 이벤트는 엔티티 액터의 연출로 이어진다',
  type: 'flow',
  w: 960,
  h: 785,
  nodes: [
    { id: 'MOD', x: 330, y: 20, w: 300, h: 56, color: 'blue', title: 'HGameFrameWorkModule', lines: ['Update 단계에서 매 프레임 Tick'] },
    { id: 'GI', x: 330, y: 116, w: 300, h: 56, color: 'blue', title: 'JGGameInstance', lines: ['게임 인스턴스 · 하나만 존재'] },
    { id: 'UI', x: 30, y: 225, w: 240, h: 56, color: 'purple', title: 'PGameUIManager', lines: ['게임 UI · 월드보다 오래 유지'] },
    { id: 'W', x: 355, y: 225, w: 250, h: 56, color: 'green', title: 'PWorld', lines: ['활성 월드 하나'] },
    { id: 'SES', x: 700, y: 225, w: 240, h: 56, color: 'orange', title: 'PGameplaySession', lines: ['세션 · 기본 Standalone'] },
    { id: 'WG', x: 30, y: 330, w: 240, h: 56, color: 'purple', title: 'JGGameWidget', lines: ['레이어별 위젯 스택'] },
    { id: 'GM', x: 700, y: 545, w: 240, h: 50, color: 'yellow', title: 'PGameMaster', lines: ['규칙 커널'] },
    { id: 'KEY', x: 30, y: 665, w: 240, h: 90, color: 'white', shape: 'note', title: '범례', lines: ['→ 소유 · 생성', '- - → 호출 · 이벤트'] }
  ].concat(kids),
  edges: [
    { from: 'MOD', to: 'GI', fs: 'b', ts: 't' },
    { from: 'GI', to: 'UI', fs: 'b', ts: 't', my: 199 },
    { from: 'GI', to: 'W', fs: 'b', ts: 't' },
    { from: 'GI', to: 'SES', fs: 'b', ts: 't', my: 199 },
    { from: 'UI', to: 'WG', fs: 'b', ts: 't' },
    { from: 'GMA', to: 'GM', fs: 'r', ts: 'l' },
    { from: 'CTL', to: 'GMA', fs: 'b', ts: 't', dash: true, label: '명령 제출' },
    { from: 'GMA', to: 'EA', fs: 'b', ts: 't', dash: true, label: '이벤트 → 연출 큐 재생' },
    { from: 'SES', to: 'GM', fs: 'b', ts: 't', dash: true, label: '연결 · 명령 실행' }
  ].concat(kids.map(k => ({ from: 'W', to: k.id, points: [[TRUNK, 281], [TRUNK, k.y + k.h / 2], [k.x, k.y + k.h / 2]] })))
};
