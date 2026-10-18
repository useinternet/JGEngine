// 4-3. 단일 루트 규칙 — IMemoryObject는 하나만 허용
module.exports = {
  file: 'Core_10_뿌리규칙',
  alt: 'IMemoryObject 단일 루트 규칙 — IMemoryObject 아래에 문자열 · 스마트 포인터, JGObject(하위에 액터 · 위젯 · 게임 클래스), 창 · 스케줄 작업 · 콜백이 위치하며, 두 부모가 모두 IMemoryObject를 상속한 클래스는 금지',
  type: 'flow',
  w: 930,
  h: 430,
  nodes: [
    { id: 'BAD', x: 320, y: 20, w: 320, h: 40, color: 'red', title: '두 부모가 모두 IMemoryObject를 상속한 클래스' },
    { id: 'R', x: 350, y: 130, w: 260, h: 60, color: 'blue', title: 'IMemoryObject', lines: ['메모리 시스템이 관리하는 모든 객체의 루트'] },
    { id: 'A1', x: 30, y: 250, w: 300, h: 60, color: 'green', title: 'PString · PName', lines: ['PSharedPtr · PWeakPtr'] },
    { id: 'A2', x: 370, y: 250, w: 220, h: 60, color: 'green', title: 'JGObject', lines: ['리플렉션 객체'] },
    { id: 'A4', x: 630, y: 250, w: 270, h: 60, color: 'green', title: 'PJWindow · 스케줄 작업', lines: ['대리자 내부 콜백 등'] },
    { id: 'A3', x: 355, y: 350, w: 250, h: 50, color: 'purple', title: 'JGActor · JGWidget · 게임 클래스 등' }
  ],
  edges: [
    { from: 'R', to: 'A1', fs: 'b', ts: 't' },
    { from: 'R', to: 'A2' },
    { from: 'R', to: 'A4', fs: 'b', ts: 't' },
    { from: 'A2', to: 'A3' },
    { from: 'BAD', to: 'R', dash: true, color: 'red', label: '금지 — Allocate가 사용할 루트를 결정할 수 없음', lw: 260 }
  ]
};
