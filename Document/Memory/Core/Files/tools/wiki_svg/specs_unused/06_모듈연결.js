// 1-1. 모듈 연결 과정
// 가운데 줄 = 처음 연결하는 경로(아니오). 이미 연결된 경우(예)는 오른쪽으로 분기한다. 분기는 마름모.
module.exports = {
  file: 'Core_06_모듈연결',
  alt: '모듈 연결 과정 — ConnectModule 호출 시 이미 연결된 모듈이면 즉시 성공, 아니면 DLL 로드 → Link_Module → 모듈 객체 생성 → StartupModule → 연결 순서 목록 끝에 추가',
  type: 'flow',
  w: 630,
  h: 670,
  nodes: [
    { id: 'A', x: 50, y: 30, w: 300, h: 40, color: 'blue', title: 'ConnectModule(이름)' },
    { id: 'B', x: 90, y: 100, w: 220, h: 80, color: 'white', shape: 'diamond', title: '이미 연결되었는가' },
    { id: 'OK', x: 380, y: 115, w: 220, h: 50, color: 'green', title: '즉시 성공 반환', lines: ['참조 카운트 증가 없음'] },
    { id: 'C', x: 50, y: 240, w: 300, h: 50, color: 'purple', title: '이름.dll 로드', lines: ['LoadLibrary'] },
    { id: 'D', x: 50, y: 330, w: 300, h: 50, color: 'purple', title: 'Link_Module', lines: ['DLL의 Core 사본을 관리자에 연결'] },
    { id: 'E', x: 50, y: 420, w: 300, h: 50, color: 'purple', title: '_Create_Module_Interface_', lines: ['모듈 객체 생성'] },
    { id: 'F', x: 50, y: 510, w: 300, h: 50, color: 'purple', title: 'StartupModule', lines: ['모듈 자체 시스템 등록 · 의존 모듈 연결'] },
    { id: 'H', x: 50, y: 600, w: 300, h: 40, color: 'green', title: '연결 순서 목록 끝에 추가' }
  ],
  edges: [
    { from: 'A', to: 'B' },
    { from: 'B', to: 'OK', fs: 'r', ts: 'l', label: '예' },
    { from: 'B', to: 'C', fs: 'b', ts: 't', label: '아니오' },
    { from: 'C', to: 'D' },
    { from: 'D', to: 'E' },
    { from: 'E', to: 'F' },
    { from: 'F', to: 'H' }
  ]
};
