// 4. 게임 프레임워크 런타임 구조
// PWorld는 묶음 틀로 그려 틀 안 상자 = PWorld 소유(Mermaid 의 WD → 여섯 상자)로 나타냈다.
// 틀 안 왼쪽 열 = 씬 · 진입 · 카메라, 오른쪽 열 = 명령 흐름(Controller → GameMaster → Entity).
// 규칙 커널(PGameMaster)과 세션은 오른쪽 열, 게임 UI는 왼쪽 열, PGameViewport는 아래에서 UI · 월드 · 컨트롤러로 올라간다.
module.exports = {
  file: 'JGEngine구조도_04_게임프레임워크구조',
  alt: '게임 프레임워크 런타임 구조 — JGGameInstance가 PWorld · PGameplaySession · PGameUIManager를 소유하고, PWorld 안의 컨트롤러가 명령을 JGGameMasterActor에 제출하면 세션을 거쳐 승인된 명령이 PGameMaster(규칙 커널)에서 실행되며, 이벤트는 엔티티 액터의 JGGameplayCue로 재생되고, PGameViewport가 월드 · 게임 UI의 그리기와 입력을 맡는다',
  type: 'flow',
  w: 1000,
  h: 560,
  groups: [
    { x: 255, y: 110, w: 450, h: 330, label: 'PWorld — 씬 · 액터 소유' }
  ],
  nodes: [
    { id: 'GI', x: 380, y: 20, w: 240, h: 50, color: 'yellow', title: 'JGGameInstance', lines: ['게임 인스턴스'] },
    // 게임 UI
    { id: 'UM', x: 20, y: 120, w: 195, h: 50, color: 'purple', title: 'PGameUIManager', lines: ['게임 UI 관리자'] },
    { id: 'GW', x: 20, y: 210, w: 195, h: 82, color: 'purple', title: 'JGGameWidget', lines: ['레이어 스택', 'Game · GameMenu', 'Menu · Modal'] },
    { id: 'KEY', x: 20, y: 330, w: 195, h: 92, color: 'white', shape: 'note', title: '범례', lines: ['→ 소유', '- - → 호출 · 전달', 'PWorld 틀 = PWorld가 소유'], small: true },
    // PWorld 안
    { id: 'SCN', x: 270, y: 150, w: 165, h: 50, color: 'gray', title: 'PScene', lines: ['Graphics 씬 데이터'] },
    { id: 'EN', x: 270, y: 260, w: 165, h: 50, color: 'blue', title: 'JGGameEntryActor', lines: ['게임 파생 — OnEnterWorld'], small: true },
    { id: 'CA', x: 270, y: 370, w: 165, h: 50, color: 'blue', title: 'JGCameraActor', lines: ['카메라'] },
    { id: 'CT', x: 480, y: 150, w: 210, h: 50, color: 'blue', title: 'JGGameplayControllerActor', lines: ['월드 클릭 → 명령'] },
    { id: 'GMA', x: 480, y: 260, w: 210, h: 50, color: 'blue', title: 'JGGameMasterActor', lines: ['규칙 커널 소유 · 이벤트 관찰'] },
    { id: 'ENT', x: 480, y: 370, w: 210, h: 50, color: 'blue', title: 'JGGameplayEntityActor', lines: ['엔티티 표현'] },
    // 세션 · 규칙 커널
    { id: 'SS', x: 750, y: 120, w: 240, h: 50, color: 'orange', title: 'PGameplaySession', lines: ['게임플레이 세션'] },
    { id: 'GM', x: 750, y: 260, w: 240, h: 50, color: 'green', title: 'PGameMaster', lines: ['규칙 커널'] },
    { id: 'ST', x: 750, y: 350, w: 118, h: 50, color: 'green', title: 'HGameplayState', lines: ['값 타입 상태'] },
    { id: 'FL', x: 872, y: 350, w: 118, h: 50, color: 'green', title: 'IGameplayFlow', lines: ['흐름 · 규칙 · 보드'] },
    // 입력 · 그리기
    { id: 'VP', x: 300, y: 490, w: 340, h: 50, color: 'gray', title: 'PGameViewport', lines: ['에디터 씬 뷰포트 · GameHost 공용'] }
  ],
  edges: [
    // 소유
    { from: 'GI', to: 'UM', points: [[380, 45], [117, 45], [117, 120]] },
    { from: 'GI', to: 'SS', points: [[620, 45], [870, 45], [870, 120]] },
    { from: 'GI', to: 'PWorld', points: [[500, 70], [500, 110]] },
    { from: 'UM', to: 'GW', points: [[117, 170], [117, 210]] },
    { from: 'GMA', to: 'GM', points: [[690, 293], [750, 293]] },
    { from: 'GM', to: 'ST', points: [[809, 310], [809, 350]] },
    { from: 'GM', to: 'FL', points: [[931, 310], [931, 350]] },
    // 호출 · 전달
    { from: 'CT', to: 'GMA', points: [[540, 200], [540, 260]], dash: true, label: '명령 제출' },
    { from: 'GMA', to: 'ENT', points: [[540, 310], [540, 370]], dash: true, label: '이벤트 → JGGameplayCue 재생', lw: 120 },
    { from: 'GMA', to: 'SS', points: [[690, 272], [727, 272], [727, 145], [750, 145]], dash: true, label: 'SubmitLocal' },
    { from: 'SS', to: 'GM', points: [[870, 170], [870, 260]], dash: true, label: '승인된 명령' },
    { from: 'VP', to: 'PWorld', points: [[600, 490], [600, 440]], dash: true, label: 'Render · HandleInput' },
    { from: 'VP', to: 'CT', points: [[457, 490], [457, 175], [480, 175]], dash: true, label: '월드 클릭', lx: 457, ly: 465 },
    { from: 'VP', to: 'UM', points: [[300, 515], [235, 515], [235, 145], [215, 145]], dash: true }
  ]
};
