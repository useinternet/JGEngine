// 3-1. 위젯 수명 주기 — 생성 · Initialize → Open → (열린 동안 매 프레임: GenerateGUI · Update) → Close ⇄ Open, GUI 모듈 종료 시 Shutdown.
// 아래 묶음: JGWidgetComponent 는 수명 주기를 창에서 전달받고, 그리기는 소유 창이 직접 호출한다.
// 코드: Runtime/GUI/Widget.h · Widget.cpp · WidgetComponent.h · WidgetComponent.cpp, GUIModule.h(OpenWidget<T>) · GUIModule.cpp(ShutdownModule)
module.exports = {
  file: 'GUI_04_위젯수명주기',
  alt: '위젯 수명 주기 — OpenWidget이나 열린 창 복원이 위젯을 처음 생성하면 Initialize를 호출하고, Open 뒤 열려 있는 동안 GraphicsBegin 단계에 GenerateGUI(ImGui 창 · OnLayout · OnGenerateGUI)를, Update 단계에 OnUpdate를 호출하며, X 단추나 CloseWidget으로 닫혀도 위젯은 남아 다시 열 수 있고, GUI 모듈 종료 시 Shutdown을 호출한다. JGWidgetComponent는 수명 주기를 창에서 전달받고 그리기는 소유 창이 SetupLayout · GenerateGUI로 호출한다',
  type: 'flow',
  w: 980,
  h: 470,
  groups: [
    { x: 480, y: 20, w: 300, h: 214, label: '열려 있는 동안 매 프레임' },
    { x: 260, y: 300, w: 700, h: 150, label: 'JGWidgetComponent — 창이 담은 하위 영역' }
  ],
  nodes: [
    { id: 'N1', x: 20, y: 55, w: 200, h: 66, color: 'purple', title: '생성 · Initialize', lines: ['OpenWidget<T>() 첫 호출', '또는 열린 창 복원'] },
    { id: 'N2', x: 260, y: 55, w: 180, h: 66, color: 'blue', title: 'Open', lines: ['bOpen = true → OnOpen'] },
    { id: 'G', x: 500, y: 56, w: 260, h: 86, color: 'blue', title: 'GenerateGUI (GraphicsBegin)', lines: ['ImGui 창 — 제목 GetTitleName()', 'SetupLayout → OnLayout', 'OnGenerateGUI'] },
    { id: 'U', x: 500, y: 162, w: 260, h: 56, color: 'blue', title: 'Update (Update 단계)', lines: ['OnUpdate — 렌더 등 무거운 일'] },
    { id: 'N4', x: 810, y: 55, w: 150, h: 80, color: 'blue', title: 'Close', lines: ['X 단추 · CloseWidget<T>()', '→ OnClose'] },
    { id: 'N5', x: 20, y: 340, w: 220, h: 86, color: 'gray', title: 'Shutdown', lines: ['GUI 모듈 종료 시', '모든 위젯 → OnShutdown'] },
    { id: 'C1', x: 280, y: 340, w: 320, h: 86, color: 'green', title: '수명 주기 전달', lines: ['창의 Initialize · Open · Update ·', 'Close · Shutdown 때 담은 부품에도 호출'] },
    { id: 'C2', x: 620, y: 340, w: 320, h: 86, color: 'green', title: '그리기', lines: ['소유 창이 OnLayout에서 SetupLayout,', 'OnGenerateGUI에서 GenerateGUI 호출', '(BeginChild 영역)'] }
  ],
  edges: [
    { from: 'N1', to: 'N2', fs: 'r', ts: 'l' },
    { points: [[440, 88], [500, 88]] },
    { from: 'G', to: 'U', fs: 'b', ts: 't' },
    { points: [[760, 88], [810, 88]] },
    { points: [[885, 135], [885, 268], [350, 268], [350, 121]], label: '다시 열기 (위젯은 닫아도 남음)', lx: 617, ly: 268 }
  ]
};
