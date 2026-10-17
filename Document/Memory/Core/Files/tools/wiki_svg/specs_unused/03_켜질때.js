// 2-1. 엔진 시작 순서
module.exports = {
  file: 'Core_03_켜질때',
  alt: '초기화 — 런처, Core 관리자, 베이스 시스템, 모듈 시스템, 실행 모듈 사이의 초기화 호출 순서',
  type: 'seq',
  participants: [
    { id: 'L', label: 'JGLauncher', sub: '실행 파일', color: 'gray' },
    { id: 'C', label: 'GCoreSystem', sub: '관리자', color: 'blue' },
    { id: 'S', label: '베이스 시스템', color: 'green' },
    { id: 'M', label: 'GModuleGlobalSystem', sub: '모듈 시스템', color: 'green' },
    { id: 'X', label: '실행 모듈', color: 'purple' }
  ],
  steps: [
    { from: 'L', to: 'C', text: 'Create 호출' },
    { from: 'C', to: 'S', text: '시스템 8개 등록' },
    { from: 'C', to: 'C', text: '루트 디렉터리 결정 (10번)' },
    { from: 'C', to: 'C', text: '리플렉션 등록 (5번)' },
    { from: 'C', to: 'M', text: '실행 모듈 연결' },
    { from: 'M', to: 'X', text: 'StartupModule 호출' },
    { from: 'X', to: 'X', text: '창 생성 (9번)' },
    { from: 'X', to: 'M', text: '의존 모듈 연결' },
    { from: 'C', to: 'S', text: '전체 시스템 Start 호출' },
    { from: 'C', to: 'L', text: '프레임 루프 시작', dash: true }
  ]
};
