// 0. GUI 모듈 — 사용하는 모듈 / GUI.dll 세 묶음(모듈 · 에디터 창, 메뉴 · 그리기 도구, 백엔드) / 의존 모듈
// 코드(구조 개편 뒤 최종 트리): Runtime/GUI/GUI.module.json · GUIModule.h · Widget.h · WidgetComponent.h · Menu/MenuTree.h · GUI.h · Grid/GUIGrid.h,
//       Backends/GUIBackend.h · DX12GUIBackend.h, Programs/JGBuildTool/Class/BuildTool.cpp(DEVELOPMENT_RUNTIME_MODULES)
module.exports = {
  file: 'GUI_00_GUI모듈',
  alt: 'GUI 모듈 — GUI.dll은 모듈 · 에디터 창(HGUIModule, JGWidget, JGWidgetComponent), 메뉴 · 그리기 도구(HMainMenuTree, HGUI, PGUIGrid), 백엔드(PGUIBackend, PDX12GUIBackend, 내장 Dear ImGui · ImPlot)로 나뉘며, JGEditor와 게임 에디터 모듈이 사용하고 Core와 Graphics에 의존하며, GameHost와 패키지는 GUI를 연결하지 않는다',
  type: 'class',
  w: 1080,
  h: 1000,
  groups: [
    { x: 20, y: 20, w: 1040, h: 110, label: 'GUI를 사용하는 모듈', color: 'purple' },
    { x: 20, y: 170, w: 1040, h: 660, label: 'GUI 모듈 (GUI.dll)' },
    { x: 40, y: 210, w: 1000, h: 210, label: '모듈 · 에디터 창', dash: true },
    { x: 40, y: 450, w: 1000, h: 160, label: '메뉴 · 그리기 도구', dash: true },
    { x: 40, y: 640, w: 1000, h: 170, label: '백엔드', dash: true },
    { x: 20, y: 870, w: 700, h: 110, label: 'GUI가 의존하는 모듈' }
  ],
  classes: [
    // 모듈 · 에디터 창
    { id: 'MOD', x: 60, y: 250, w: 290, name: 'HGUIModule', stereo: '모듈 클래스', color: 'blue', members: ['-GUIBackend · MainMenuTree', '-Widgets : 클래스 → 위젯', '+OpenWidget<T>() · CloseWidget<T>()', '+AddMainMenuItem(item)'] },
    { id: 'WID', x: 410, y: 250, w: 250, name: 'JGWidget', stereo: '에디터 창 기반 · JGCLASS', color: 'blue', members: ['#OnInitialize() · OnShutdown()', '#OnOpen() · OnClose() · OnUpdate()', '#OnLayout() · OnGenerateGUI()', '#GetTitleName()'] },
    { id: 'WC', x: 740, y: 250, w: 280, name: 'JGWidgetComponent', stereo: '창 안 하위 영역 · JGCLASS', color: 'blue', members: ['#창과 같은 수명 주기 훅', '+SetupLayout() · GenerateGUI()'] },
    // 메뉴 · 그리기 도구
    { id: 'MENU', x: 60, y: 490, w: 260, name: 'HMainMenuTree', stereo: '메인 메뉴 막대', color: 'green', members: ['+AddMainMenuItem(item)', '+GenerateMainMenuGUI()'] },
    { id: 'HGUI', x: 380, y: 490, w: 290, name: 'HGUI', stereo: '즉시 모드 정적 함수 묶음', color: 'green', members: ['+Text() · Button() · InputText…()', '+InteractiveImage() · PlotLines()'] },
    { id: 'GRID', x: 720, y: 490, w: 300, name: 'PGUIGrid', stereo: '스프레드시트 그리드 컨트롤', color: 'green', members: ['+Draw(id, source, size, events)', 'IGUIGridSource · HGUIGridEvent'] },
    // 백엔드
    { id: 'BASE', x: 60, y: 680, w: 270, name: 'PGUIBackend', stereo: 'GUI 백엔드 · 추상 클래스', color: 'yellow', members: ['+OnGUI · OnMainMenuGUI 이벤트', '+GPUAllocate() · GetDpiScale()'] },
    { id: 'DX', x: 400, y: 680, w: 280, name: 'PDX12GUIBackend', stereo: 'DX12 · Win32 구현', color: 'yellow', members: ['+NewFrame()', '-OnUpdate() · OnPresent() · OnWndProc()'] }
  ],
  nodes: [
    { id: 'U1', x: 40, y: 56, w: 300, h: 56, color: 'purple', title: 'JGEditor', lines: ['에디터 창 · 메인 메뉴 등록'] },
    { id: 'U2', x: 370, y: 56, w: 300, h: 56, color: 'purple', title: '게임 에디터 모듈', lines: ['<게임>Editor — 메뉴 · 창 등록'] },
    { id: 'NO', x: 700, y: 50, w: 340, h: 70, color: 'white', shape: 'note', title: 'GameHost · 패키지', lines: ['GUI를 연결하지 않는다', '(JGBuildTool의 개발용 런타임 모듈)'] },
    { id: 'IMG', x: 740, y: 680, w: 280, h: 90, color: 'gray', title: 'Dear ImGui · ImPlot', lines: ['GUI.dll 안에 내장 · 밖으로 노출 안 함', '도킹 · 멀티 뷰포트', 'imgui_impl_win32 · imgui_impl_dx12'] },
    { id: 'CORE', x: 40, y: 906, w: 320, h: 56, color: 'gray', title: 'Core', lines: ['모듈 · 리플렉션 · 스케줄 · 창 WndProc'] },
    { id: 'GFX', x: 380, y: 906, w: 320, h: 56, color: 'gray', title: 'Graphics', lines: ['DX12 장치 · 프레임 버퍼 이벤트'] },
    { id: 'KEY', x: 760, y: 870, w: 300, h: 110, color: 'white', shape: 'note', title: '범례', lines: ['◇— 소유 · —▷ 상속 · → 사용 · 호출', '+ public · # protected · - private'] }
  ],
  edges: [
    // 모듈 사이
    { points: [[540, 130], [540, 170]], label: '사용' },
    { points: [[370, 830], [370, 870]], label: '의존' },
    // 모듈 · 에디터 창
    { points: [[350, 290], [410, 290]], start: 'diamond', end: 'none', label: '1 : N' },
    { points: [[660, 290], [740, 290]], start: 'diamond', end: 'none', label: '1 : N' },
    { points: [[205, 394], [205, 490]], start: 'diamond', end: 'none', label: '소유' },
    { points: [[340, 394], [340, 630], [540, 630], [540, 680]], start: 'diamond', end: 'none', label: '소유', lx: 440, ly: 630 },
    // 백엔드
    { points: [[400, 725], [330, 725]], end: 'triangle' },
    { points: [[680, 725], [740, 725]], label: '호출' }
  ]
};
