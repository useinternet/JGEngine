// 8. 로그 흐름
module.exports = {
  file: 'Core_17_로그흐름',
  alt: '로그 흐름 — 모든 모듈의 JG_LOG는 로그 시스템(spdlog 로거 1개)을 거쳐 파일 · 콘솔 창 · 최근 로그로 출력되고, 최근 로그는 DevConsole 창과 JGConsole 모듈 명령이 읽는다',
  type: 'flow',
  w: 760,
  h: 450,
  nodes: [
    { id: 'A', x: 210, y: 20, w: 340, h: 44, color: 'purple', title: '모든 모듈의 JG_LOG(분류, 레벨, printf 형식, 값…)' },
    { id: 'B', x: 250, y: 104, w: 260, h: 60, color: 'blue', title: 'GLogGlobalSystem', lines: ['로그 시스템 · spdlog 로거 1개'] },
    { id: 'F', x: 30, y: 220, w: 220, h: 80, color: 'yellow', title: '파일', lines: ['툴 · 에디터: jg_log.txt', '패키지: Saved/Logs/이름.log'] },
    { id: 'C', x: 300, y: 220, w: 160, h: 80, color: 'gray', title: '콘솔 창', lines: ['레벨별 색상'] },
    { id: 'RS', x: 510, y: 220, w: 220, h: 80, color: 'green', shape: 'cyl', title: '최근 로그', lines: ['Info 이상 1024줄 + 일련번호'] },
    { id: 'DC', x: 270, y: 360, w: 220, h: 60, color: 'purple', title: 'DevConsole 창', lines: ['일련번호 변경 시에만 복사'] },
    { id: 'JC', x: 520, y: 360, w: 200, h: 60, color: 'gray', title: 'JGConsole 모듈 명령' }
  ],
  edges: [
    { from: 'A', to: 'B' },
    { from: 'B', to: 'F', fs: 'b', ts: 't' },
    { from: 'B', to: 'C', fs: 'b', ts: 't' },
    { from: 'B', to: 'RS', fs: 'b', ts: 't' },
    { from: 'RS', to: 'DC', fs: 'b', ts: 't' },
    { from: 'RS', to: 'JC', fs: 'b', ts: 't' }
  ]
};
