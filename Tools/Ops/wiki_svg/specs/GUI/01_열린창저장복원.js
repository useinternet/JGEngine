// 1-1. 열린 창 저장과 복원 — imgui.ini 의 [Window](ImGui) · [JGWidget][Open](GUI 모듈) 절, 시작 · 첫 프레임 · 매 프레임 · 종료 흐름
// 코드: Runtime/GUI/GUIModule.h · GUIModule.cpp(RegisterLayoutSettings · RestoreLayoutWidgets · UpdateLayoutWidgetStates · IsWidgetClosedInLayout)
module.exports = {
  file: 'GUI_01_열린창저장복원',
  alt: '열린 창 저장과 복원 — HGUIModule은 StartupModule에서 imgui.ini의 열린 위젯 항목(JGWidget · Open)을 읽고, 다른 모듈의 OpenWidgetByDefault는 저장된 상태가 닫힘이면 창을 열지 않으며, 첫 프레임에 열림으로 저장된 위젯 클래스를 리플렉션으로 생성해 열고, 매 프레임 열림 상태를 갱신해 종료 시 ImGui가 imgui.ini에 저장한다',
  type: 'flow',
  w: 1020,
  h: 410,
  groups: [
    { x: 20, y: 20, w: 980, h: 170, label: 'HGUIModule' }
  ],
  nodes: [
    { id: 'S1', x: 40, y: 64, w: 210, h: 100, color: 'blue', title: 'StartupModule', lines: ['설정 핸들러 등록', 'imgui.ini 읽기', '→ 클래스별 열림 상태'] },
    { id: 'S2', x: 280, y: 64, w: 210, h: 100, color: 'purple', title: 'OpenWidgetByDefault<T>()', lines: ['실행 모듈 StartupModule에서 호출', '저장된 상태가 닫힘이면 열지 않음'] },
    { id: 'S3', x: 520, y: 64, w: 210, h: 100, color: 'blue', title: 'GenerateWidgetGUI', lines: ['첫 프레임에 열림 상태인 위젯', '클래스를 리플렉션으로 생성 · Open', '등록되지 않은 클래스는 건너뜀'] },
    { id: 'S4', x: 760, y: 64, w: 220, h: 100, color: 'blue', title: '매 프레임', lines: ['열린 위젯 상태를 갱신', '바뀌면 저장 표시', '(MarkIniSettingsDirty)'] },
    { id: 'INI', x: 40, y: 250, w: 420, h: 100, color: 'yellow', shape: 'cyl', title: 'imgui.ini (실행 폴더)', lines: ['[Window] — 창 위치 · 크기 · 도킹 (ImGui)', '[JGWidget][Open] — 위젯 클래스 이름 = 0 | 1 (GUI 모듈)'] },
    { id: 'OBJ', x: 520, y: 250, w: 210, h: 100, color: 'gray', title: 'GObjectGlobalSystem', lines: ['클래스 이름 → JGType', 'NewObject로 위젯 생성'] }
  ],
  edges: [
    { from: 'S1', to: 'S2', fs: 'r', ts: 'l' },
    { from: 'S2', to: 'S3', fs: 'r', ts: 'l' },
    { from: 'S3', to: 'S4', fs: 'r', ts: 'l' },
    { points: [[145, 250], [145, 164]], label: '읽기' },
    { points: [[625, 164], [625, 250]], dash: true, label: '생성' },
    { points: [[870, 164], [870, 385], [250, 385], [250, 350]], label: '종료 시 ImGui가 저장', lx: 560, ly: 385 }
  ]
};
