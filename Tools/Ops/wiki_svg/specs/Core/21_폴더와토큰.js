// 10-2. 폴더와 토큰 경로
// 가운데(토큰 경로 · 저장 폴더)에서 왼쪽 에디터 빌드 폴더 · 오른쪽 패키지 폴더로 뻗게 놓았다(같은 줄 = 같은 폴더 함수, 화살표가 엇갈리지 않게).
module.exports = {
  file: 'Core_21_폴더와토큰',
  alt: '디렉터리와 토큰 경로 — /JGEngine/…은 EngineContentDirectory를 통해 에디터 빌드의 엔진 디렉터리/Content와 패키지/Content/JGEngine에, /JGGame/…은 GameContentDirectory를 통해 프로젝트 디렉터리/Content와 패키지/Content/JGGame에 대응하고, 저장 디렉터리(SavedDirectory)는 프로젝트 디렉터리/Saved와 패키지/Saved에 대응한다',
  type: 'flow',
  w: 1030,
  h: 340,
  groups: [
    { x: 30, y: 20, w: 200, h: 290, label: '에디터 빌드' },
    { x: 400, y: 20, w: 200, h: 190, label: '토큰 경로 — 에셋 · 사운드 이름' },
    { x: 770, y: 20, w: 230, h: 290, label: '패키지' }
  ],
  nodes: [
    { id: 'EE', x: 50, y: 70, w: 160, h: 50, color: 'yellow', title: '엔진 루트/Content' },
    { id: 'EG', x: 50, y: 140, w: 160, h: 50, color: 'yellow', title: '프로젝트 루트/Content' },
    { id: 'ES', x: 50, y: 240, w: 160, h: 50, color: 'yellow', title: '프로젝트 루트/Saved' },
    { id: 'TE', x: 420, y: 70, w: 160, h: 50, color: 'blue', title: '/JGEngine/…' },
    { id: 'TG', x: 420, y: 140, w: 160, h: 50, color: 'blue', title: '/JGGame/…' },
    { id: 'SV', x: 385, y: 240, w: 230, h: 50, color: 'blue', title: '저장 디렉터리 (SavedDirectory)', lines: ['패키지 로그: Saved/Logs'] },
    { id: 'PE', x: 790, y: 70, w: 190, h: 50, color: 'yellow', title: '패키지/Content/JGEngine' },
    { id: 'PG', x: 790, y: 140, w: 190, h: 50, color: 'yellow', title: '패키지/Content/JGGame' },
    { id: 'PS', x: 790, y: 240, w: 190, h: 50, color: 'yellow', title: '패키지/Saved' }
  ],
  edges: [
    { from: 'TE', to: 'EE', fs: 'l', ts: 'r', label: 'EngineContentDirectory' },
    { from: 'TE', to: 'PE', fs: 'r', ts: 'l', label: 'EngineContentDirectory' },
    { from: 'TG', to: 'EG', fs: 'l', ts: 'r', label: 'GameContentDirectory' },
    { from: 'TG', to: 'PG', fs: 'r', ts: 'l', label: 'GameContentDirectory' },
    { from: 'SV', to: 'ES', fs: 'l', ts: 'r' },
    { from: 'SV', to: 'PS', fs: 'r', ts: 'l' }
  ]
};
