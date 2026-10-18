// 5-2. 저장 · 불러오기 구조
module.exports = {
  file: 'Core_12_저장불러오기',
  alt: '직렬화 구조 — SaveObject는 객체를 JSON 필드 2개(타입 이름 · 내용)로 저장하고 내용은 상속 계층마다 항목 하나를 두며, LoadObject는 타입 이름으로 등록된 타입을 찾아 생성 함수로 새 객체를 생성한 뒤 계층마다 ReadJson을 호출한다',
  type: 'flow',
  w: 630,
  h: 560,
  nodes: [
    { id: 'OBJ', x: 30, y: 20, w: 280, h: 60, color: 'orange', title: '객체', lines: ['예: 게임 액터'] },
    { id: 'J', x: 30, y: 150, w: 280, h: 70, color: 'yellow', title: 'JSON 필드 2개', lines: ['JGObjectType: 타입 이름', 'JGObject: 내용'] },
    { id: 'L1', x: 380, y: 140, w: 220, h: 90, color: 'yellow', title: '내용: 상속 계층별 항목', lines: ['JGObject 계층: Name', 'JGActor 계층: 속성', '게임 클래스 계층: 속성'] },
    { id: 'T', x: 30, y: 290, w: 280, h: 40, color: 'green', title: '타입 이름으로 등록된 타입 검색' },
    { id: 'N', x: 30, y: 380, w: 280, h: 40, color: 'green', title: '해당 타입의 생성 함수로 새 객체 생성' },
    { id: 'R', x: 30, y: 470, w: 280, h: 60, color: 'green', title: '계층별 ReadJson 호출', lines: ['속성 값 설정'] }
  ],
  edges: [
    { from: 'OBJ', to: 'J', label: 'SaveObject' },
    { from: 'J', to: 'L1', fs: 'r', ts: 'l' },
    { from: 'J', to: 'T', label: 'LoadObject' },
    { from: 'T', to: 'N' },
    { from: 'N', to: 'R' }
  ]
};
