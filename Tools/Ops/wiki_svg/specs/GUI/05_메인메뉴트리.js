// 4-1. HMainMenuItem과 HMainMenuTree — 메뉴 경로 "A/B/C" 를 노드 트리로 저장하고, 가지는 BeginMenu · 잎은 MenuItem 으로 그린다
// 코드: Runtime/GUI/Menu/MenuTree.h · MenuTree.cpp, Editor/JGEditor/JGEditor.cpp(메뉴 등록), Build/Templates/GameProject/…EditorModule.cpp
module.exports = {
  file: 'GUI_05_메인메뉴트리',
  alt: 'HMainMenuItem과 HMainMenuTree — HMainMenuItem은 메뉴 경로와 Action · CanAction · Visibility 델리게이트를 가지고, AddMainMenuItem이 경로를 / 로 나눠 Root 아래 노드 트리에 추가하며, GenerateMainMenuGUI는 가지 노드를 BeginMenu로, 잎 노드를 MenuItem으로 그린다',
  type: 'flow',
  w: 1000,
  h: 360,
  groups: [
    { x: 320, y: 20, w: 420, h: 320, label: 'HMainMenuTree (노드 목록)' }
  ],
  nodes: [
    { id: 'ITEM', x: 20, y: 40, w: 260, h: 96, color: 'yellow', shape: 'note', title: 'HMainMenuItem', lines: ['MenuPath "Windows/Statistics/Memory"', 'Action · CanAction · Visibility'] },
    { id: 'ROOT', x: 470, y: 56, w: 120, h: 36, color: 'gray', shape: 'pill', title: 'Root' },
    { id: 'WIN', x: 370, y: 126, w: 130, h: 40, color: 'blue', title: 'Windows' },
    { id: 'GAME', x: 560, y: 126, w: 160, h: 40, color: 'blue', title: '<게임>' },
    { id: 'STAT', x: 340, y: 206, w: 120, h: 40, color: 'blue', title: 'Statistics' },
    { id: 'CON', x: 470, y: 206, w: 120, h: 40, color: 'green', title: 'DevConsole' },
    { id: 'REL', x: 600, y: 206, w: 130, h: 40, color: 'green', title: 'Reload World' },
    { id: 'MEM', x: 340, y: 286, w: 120, h: 40, color: 'green', title: 'Memory' },
    { id: 'GEN', x: 770, y: 40, w: 210, h: 150, color: 'purple', title: 'GenerateMainMenuGUI', lines: ['가지 노드 → BeginMenu', '잎 노드 → MenuItem', 'Visibility → 보임', 'CanAction → 활성', '누르면 Action 실행'] },
    { id: 'KEY', x: 770, y: 220, w: 210, h: 106, color: 'white', shape: 'note', title: '범례', lines: ['파랑: 가지 노드', '초록: 잎 노드(메뉴 항목)'] }
  ],
  edges: [
    { points: [[150, 136], [150, 180], [320, 180]], label: 'AddMainMenuItem', lx: 235, ly: 180 },
    { from: 'ROOT', to: 'WIN', fs: 'b', ts: 't' },
    { from: 'ROOT', to: 'GAME', fs: 'b', ts: 't' },
    { from: 'WIN', to: 'STAT', fs: 'b', ts: 't' },
    { from: 'WIN', to: 'CON', fs: 'b', ts: 't' },
    { from: 'GAME', to: 'REL', fs: 'b', ts: 't' },
    { from: 'STAT', to: 'MEM', fs: 'b', ts: 't' },
    { points: [[740, 110], [770, 110]] }
  ]
};
