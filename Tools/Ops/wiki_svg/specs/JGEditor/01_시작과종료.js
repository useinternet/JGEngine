// 1-1. 시작과 종료 순서 — StartupModule: 창 → 엔진 모듈(Graphics · GUI · GameFrameWork) → 메뉴 → 프로젝트 모듈 → 씬 뷰포트 / ShutdownModule: 역순
// 코드(구조 개편 뒤): Editor/JGEditor/JGEditor.cpp(StartupModule · ShutdownModule · openDefaultWidgets · connectProjectModules · openSceneViewport · disconnectProjectModules),
//       Runtime/GUI/GUIModule.cpp(RestoreLayoutWidgets — 첫 프레임)
module.exports = {
  file: 'JGEditor_01_시작과종료',
  alt: '시작과 종료 순서 — HJGEditorModule::StartupModule은 창을 생성하고 엔진 모듈을 Graphics · GUI · GameFrameWork 순으로 연결한 뒤 에디터 창 메뉴를 등록하고, 프로젝트 모드에서는 게임 모듈 · 게임 에디터 모듈을 연결해 씬 뷰포트를 기본 창으로 열며, GUI 모듈은 첫 프레임에 저장된 창을 다시 열고, ShutdownModule은 프로젝트 모듈 · GameFrameWork · 창 · 나머지 엔진 모듈 순으로 해제한다',
  type: 'seq',
  colW: 310,
  w: 1330,
  participants: [
    { id: 'C', label: 'GCoreSystem', sub: 'Core 시스템', color: 'gray' },
    { id: 'E', label: 'HJGEditorModule', sub: '에디터 호스트', color: 'blue' },
    { id: 'M', label: 'GModuleGlobalSystem', sub: '모듈 시스템', color: 'gray' },
    { id: 'G', label: 'HGUIModule', sub: 'GUI 모듈', color: 'purple' }
  ],
  steps: [
    { gap: 10 },
    { from: 'C', to: 'E', text: 'StartupModule (실행 모듈 연결)' },
    { from: 'E', to: 'E', text: '창 생성 — 기본 크기 × DPI 배율' },
    { from: 'E', to: 'M', text: 'ConnectModule — Graphics → GUI → GameFrameWork' },
    { from: 'E', to: 'G', text: 'AddMainMenuItem — 에디터 창 메뉴' },
    { gap: 6 },
    { note: '프로젝트 모드일 때', over: ['E', 'G'], color: 'green' },
    { gap: 10 },
    { from: 'E', to: 'M', text: 'ConnectModule — GameModules → EditorModules' },
    { from: 'E', to: 'G', text: 'OpenWidgetByDefault<JGSceneViewport>' },
    { gap: 6 },
    { from: 'G', to: 'G', text: '첫 프레임 — 저장된 창 다시 열기' },
    { gap: 14 },
    { from: 'C', to: 'E', text: 'ShutdownModule (종료)' },
    { from: 'E', to: 'M', text: 'DisconnectModule — 프로젝트 모듈 → GameFrameWork' },
    { from: 'E', to: 'E', text: '창 해제' },
    { from: 'E', to: 'M', text: 'DisconnectModule — GUI → Graphics' }
  ]
};
