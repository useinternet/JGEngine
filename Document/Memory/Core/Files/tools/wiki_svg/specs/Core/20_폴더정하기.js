// 10-1. 시작 시 폴더 결정
module.exports = {
  file: 'Core_20_폴더정하기',
  alt: '루트 디렉터리 결정 — 에디터 빌드는 -project= 인자(툴)와 ../../의 .jgproject 유무로 프로젝트 모드와 엔진 단독 모드를 구분하고, 패키지는 Bin/JGPackage.json을 읽어 엔진 · 프로젝트 디렉터리를 모두 패키지 디렉터리로 지정하며, 읽기에 실패하면 종료 코드 2로 시작을 중단한다',
  type: 'flow',
  w: 980,
  h: 530,
  nodes: [
    { id: 'B', x: 320, y: 20, w: 260, h: 80, color: 'blue', shape: 'diamond', title: '빌드 종류' },
    { id: 'P', x: 120, y: 150, w: 260, h: 80, color: 'blue', shape: 'diamond', title: '-project= 인자 여부' },
    { id: 'K', x: 560, y: 165, w: 200, h: 50, color: 'blue', title: 'Bin/JGPackage.json 읽기' },
    { id: 'P1', x: 30, y: 300, w: 180, h: 60, color: 'yellow', lines: ['프로젝트 = 인자 디렉터리', '엔진 = 툴의 엔진 디렉터리'] },
    { id: 'J', x: 250, y: 290, w: 260, h: 80, color: 'blue', shape: 'diamond', title: '../../의 .jgproject 여부' },
    { id: 'K1', x: 570, y: 300, w: 180, h: 60, color: 'yellow', lines: ['엔진 = 프로젝트 = 패키지 루트'] },
    { id: 'K2', x: 800, y: 300, w: 150, h: 60, color: 'red', title: '시작 중단', lines: ['종료 코드 2'] },
    { id: 'J1', x: 150, y: 430, w: 200, h: 70, color: 'yellow', title: '프로젝트 모드', lines: ['엔진 = EngineRoot', '또는 JGENGINE_ROOT'] },
    { id: 'J2', x: 420, y: 430, w: 180, h: 70, color: 'yellow', title: '엔진 단독 모드', lines: ['엔진 · 프로젝트 모두 ../../'] }
  ],
  edges: [
    { from: 'B', to: 'P', fs: 'l', ts: 't', label: '에디터 빌드' },
    { from: 'B', to: 'K', fs: 'r', ts: 't', label: '패키지' },
    { from: 'P', to: 'P1', fs: 'l', ts: 't', label: '있음 — 툴' },
    { from: 'P', to: 'J', fs: 'r', ts: 't', label: '없음' },
    { from: 'J', to: 'J1', fs: 'l', ts: 't', label: '있음' },
    { from: 'J', to: 'J2', fs: 'r', ts: 't', label: '없음' },
    { from: 'K', to: 'K1', fs: 'b', ts: 't', label: '읽기 성공' },
    { from: 'K', to: 'K2', fs: 'r', ts: 't', label: '읽기 실패', lx: 875, ly: 245 }
  ]
};
