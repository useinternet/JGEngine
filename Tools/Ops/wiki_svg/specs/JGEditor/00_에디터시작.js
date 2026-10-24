// 에디터 호스트 HJGEditorModule — 시작 과정
// 엔진 모듈 7개는 한 상자에 늘어놓지 않고 연결 순서대로 작은 상자 줄로 그린다(창 생성 → 첫 모듈, 마지막 모듈 → 메뉴 등록).
const MODS = [['Graphics', 84], ['GUI', 56], ['Asset', 66], ['DevStatistics', 116], ['DevConsole', 96], ['Devkit', 70], ['GameFrameWorks', 128]];
let mx = 210;
const mods = MODS.map(([name, w], i) => {
  const n = { id: 'M' + i, x: mx, y: 250, w, h: 40, color: 'purple', shape: 'pill', title: name };
  mx += w + 20;
  return n;
});
const last = mods[mods.length - 1];

module.exports = {
  file: 'JGEditor_00_에디터시작',
  alt: '에디터 호스트 HJGEditorModule — JGLauncher.exe가 JGEditor를 실행 모듈로 띄우면 창을 생성하고 엔진 모듈 7개(Graphics → GUI → Asset → DevStatistics → DevConsole → Devkit → GameFrameWorks)를 순서대로 연결한 뒤 Windows 메뉴 4개를 등록하고, 프로젝트 모드면 .jgproject의 게임 · 에디터 모듈을 연결해 씬 뷰포트를 열며, 아니면 엔진만으로 실행한다',
  type: 'flow',
  w: 980,
  h: 710,
  groups: [
    { x: 20, y: 208, w: 940, h: 100, label: '엔진 모듈 연결 (순서대로)' }
  ],
  nodes: [
    { id: 'L', x: 330, y: 20, w: 300, h: 56, color: 'blue', title: 'JGLauncher.exe', lines: ['LaunchModule = JGEditor'] },
    { id: 'W', x: 280, y: 106, w: 400, h: 72, color: 'green', title: '창 생성', lines: ['제목: JGEditor 또는 JGEditor - 프로젝트', '1920x1080 클라이언트 × DPI 배율'] },
    { id: 'D', x: 330, y: 334, w: 300, h: 56, color: 'blue', title: 'openDefaultWidgets', lines: ['Windows 메뉴 4개 등록'] },
    { id: 'P', x: 370, y: 420, w: 220, h: 80, color: 'white', shape: 'diamond', title: '프로젝트 모드인가' },
    { id: 'G', x: 60, y: 530, w: 340, h: 60, color: 'blue', title: 'connectProjectModules', lines: ['.jgproject의 GameModules → EditorModules'] },
    { id: 'S', x: 60, y: 630, w: 340, h: 60, color: 'green', title: 'openSceneViewport', lines: ['게임 모듈 연결 시 씬 뷰포트를 기본으로 엶'] },
    { id: 'E', x: 560, y: 530, w: 340, h: 60, color: 'gray', title: '엔진만', lines: ['창들은 안내 문구만 그림'] }
  ].concat(mods),
  edges: [
    { from: 'L', to: 'W' },
    { from: 'W', to: 'M0', fs: 'b', ts: 't', my: 193 },
    ...mods.slice(1).map((m, i) => ({ from: mods[i].id, to: m.id, fs: 'r', ts: 'l' })),
    { from: last.id, to: 'D', fs: 'b', ts: 't', my: 321 },
    { from: 'D', to: 'P' },
    { from: 'P', to: 'G', fs: 'l', ts: 't', label: '예' },
    { from: 'G', to: 'S' },
    { from: 'P', to: 'E', fs: 'r', ts: 't', label: '아니오' }
  ]
};
