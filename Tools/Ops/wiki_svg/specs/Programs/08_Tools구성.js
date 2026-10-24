// 9-1. Tools 폴더 구성 — Common(공용 검증) · Ops(팀 운영) · 분야별 검증 도구, 검증 도구는 엔진 · 게임 실행 파일을 감싸 실행한다
// 코드: Tools/README.md, Tools/Common/crashwalk/crashwalk.cpp, Tools/Common/capture_devscene.ps1, Tools/Ops/
module.exports = {
  file: 'Programs_08_Tools구성',
  alt: 'Tools 폴더 구성 — Tools/는 여러 분야가 함께 쓰는 Common(crashwalk · capture_devscene · uitest_hook), 팀 운영용 Ops(compaction_watch · wiki_svg), 분야별 검증 도구(Core · DevOps · Editor · GameFrameWorks · Graphics)로 나뉘며, 검증 도구는 JGLauncher · JGConsole · 패키지 exe를 crashwalk로 감싸 실행하고 종료 코드와 로그로 판정한다',
  type: 'flow',
  w: 980,
  h: 470,
  groups: [
    { x: 20, y: 20, w: 940, h: 300, label: 'Tools/' },
    { x: 40, y: 60, w: 430, h: 240, label: 'Common/ — 공용 검증' },
    { x: 500, y: 60, w: 440, h: 110, label: 'Ops/ — 팀 운영' },
    { x: 500, y: 190, w: 440, h: 110, label: '분야별 — Core · DevOps · Editor · GameFrameWorks · Graphics' }
  ],
  nodes: [
    { id: 'CW', x: 60, y: 100, w: 390, h: 56, color: 'purple', title: 'crashwalk', lines: ['디버거로 실행 · 예외 콜스택 · 정한 초 뒤 닫기 · 종료 코드'] },
    { id: 'CAP', x: 60, y: 168, w: 390, h: 56, color: 'purple', title: 'capture_devscene.ps1', lines: ['런처를 띄워 에디터 창 캡처'] },
    { id: 'HOOK', x: 60, y: 236, w: 390, h: 56, color: 'yellow', title: 'uitest_hook', lines: ['워크트리 전용 ImGui 입력 훅 — 창 드라이버용'] },
    { id: 'CWW', x: 520, y: 100, w: 200, h: 56, color: 'blue', title: 'compaction_watch.js', lines: ['세션 대화 압축 감시'] },
    { id: 'WS', x: 730, y: 100, w: 195, h: 56, color: 'blue', title: 'wiki_svg', lines: ['위키 그림 그리개'] },
    { id: 'FLD', x: 520, y: 230, w: 405, h: 56, color: 'purple', title: '검증 게임 · 창 드라이버 · 측정 스크립트', lines: ['분야 코드를 바꿀 때 다시 돌리는 시험'] },
    { id: 'TGT', x: 60, y: 370, w: 390, h: 72, color: 'green', title: '엔진 · 게임 실행 파일', lines: ['JGLauncher · JGConsole · <Name>.exe', '종료 코드 · 로그로 판정'] },
    { id: 'MT', x: 520, y: 370, w: 405, h: 72, color: 'white', title: 'MainTree.cmd exec', lines: ['메인 트리 Bin을 쓰는 도구는', '도우미로 감싸 실행'] }
  ],
  edges: [
    { from: 'CW', to: 'TGT', points: [[60, 128], [30, 128], [30, 406], [60, 406]], label: '감싸 실행', lx: 30, ly: 340, lpos: 'r' },
    { from: 'FLD', to: 'CW', fs: 'l', ts: 'r', label: 'crashwalk 사용', lx: 485, ly: 180, lpos: 'r' },
    { from: 'TGT', to: 'MT', fs: 'r', ts: 'l', dash: true }
  ]
};
