// 3-2. 코드 생성과 등록 과정 — 선언 → 코드 생성 → 등록 → 사용
module.exports = {
  file: 'Core_11_리플렉션흐름',
  alt: '코드 생성과 등록 과정 — 헤더의 JGCLASS · JGPROPERTY 선언, 빌드 전 JGHeaderTool의 분석, 코드 생성, 모듈 DLL 컴파일, 초기화 시 모듈 목록의 DLL 로드, 오브젝트 시스템 등록, 사용의 7단계',
  type: 'flow',
  w: 680,
  h: 820,
  nodes: [
    { id: 'H', x: 30, y: 20, w: 300, h: 70, color: 'yellow', title: '1 헤더에 리플렉션 매크로 선언', lines: ['JGCLASS · JGPROPERTY', 'JGFUNCTION · JGENUM'] },
    { id: 'HT', x: 30, y: 140, w: 300, h: 70, color: 'gray', title: '2 빌드 전 JGHeaderTool 실행', lines: ['선언을 찾아 클래스 · 속성 · 함수 분석', '분석 결과로 코드 생성'] },
    { id: 'GEN', x: 30, y: 260, w: 300, h: 70, color: 'yellow', title: '3 생성된 코드', lines: ['Temp/CodeGen/모듈 디렉터리의 .generation 파일', '+ 등록 함수 Engine_CodeGenerate'] },
    { id: 'DLL', x: 30, y: 380, w: 300, h: 50, color: 'purple', title: '4 모듈 DLL에 함께 컴파일' },
    { id: 'LOAD', x: 30, y: 480, w: 300, h: 70, color: 'blue', title: '5 초기화 시', lines: ['목록의 DLL 로드 후', 'Engine_CodeGenerate 호출'] },
    { id: 'LIST', x: 400, y: 470, w: 260, h: 90, color: 'yellow', title: '모듈 목록', lines: ['에디터: module_system_info.json', '게임 전용: runtime_module_info.json', '패키지: JGPackage.json'] },
    { id: 'REG', x: 30, y: 600, w: 300, h: 70, color: 'green', title: '6 오브젝트 시스템에 등록', lines: ['이름 → 타입', '타입 → 클래스 정보 · 생성 함수'] },
    { id: 'USE', x: 30, y: 720, w: 300, h: 70, color: 'orange', title: '7 사용', lines: ['StaticClass · AllocateByClass', 'SaveObject · LoadObject · Cast'] }
  ],
  edges: [
    { from: 'H', to: 'HT' },
    { from: 'HT', to: 'GEN' },
    { from: 'GEN', to: 'DLL' },
    { from: 'DLL', to: 'LOAD' },
    { from: 'LIST', to: 'LOAD', fs: 'l', ts: 'r', dash: true },
    { from: 'LOAD', to: 'REG' },
    { from: 'REG', to: 'USE' }
  ]
};
