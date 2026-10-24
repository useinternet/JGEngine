// Devkit — 핵심 클래스 · 개념: Devkit 모듈의 클래스와 GUI 기반 클래스, JGEditor 연결
// 근거: Source/Runtime/Devkit/DevkitModule.h/.cpp · DevFeature.h/.cpp · DevScene.h · DevSettings.h/.cpp,
//       Source/Editor/JGEditor/JGEditor.cpp(EDITOR_ENGINE_MODULES 에 "Devkit")
module.exports = {
  file: 'Devkit_00_클래스구조',
  alt: '핵심 클래스 · 개념 — JGEditor가 연결하는 Devkit 모듈은 시작 · 종료가 비어 있는 HDevKitModule, JGWidget을 상속한 창 JGDevFeature, 그 창이 소유하는 JGWidgetComponent 하위 영역 JGDevScene(장면 · 렌더러 · 테스트 메시)과 JGDevSettings(비어 있음)로 구성된다',
  type: 'class',
  w: 1130,
  h: 590,
  groups: [
    { x: 290, y: 20, w: 820, h: 110, label: 'GUI 모듈', color: 'purple' },
    { x: 20, y: 190, w: 1090, h: 280, label: 'Devkit 모듈 (Devkit.dll)' }
  ],
  nodes: [
    { id: 'KEY', x: 20, y: 20, w: 240, h: 140, color: 'white', shape: 'note', title: '범례', lines: ['◇— 포함(소유)', '—▷ 상속', '→ 모듈 연결', '+ public · # protected · - private'] },
    { id: 'ED', x: 40, y: 506, w: 220, h: 64, color: 'gray', title: 'JGEditor', lines: ['엔진 모듈 목록에서 연결'] }
  ],
  classes: [
    { id: 'W', x: 310, y: 64, w: 240, name: 'JGWidget', stereo: '창 기반 클래스', color: 'purple' },
    { id: 'WC', x: 740, y: 64, w: 260, name: 'JGWidgetComponent', stereo: '창 안 하위 영역 기반 클래스', color: 'purple' },
    { id: 'MOD', x: 40, y: 240, w: 220, name: 'HDevKitModule', stereo: 'IModuleInterface 상속 · 모듈 객체', color: 'blue', members: ['#StartupModule()  // 비어 있음', '#ShutdownModule()  // 비어 있음'] },
    { id: 'FEAT', x: 310, y: 240, w: 240, name: 'JGDevFeature', stereo: '창 "DevFeature"', color: 'green', members: ['-DevScene  // 800×600', '-DevSettings  // 너비 250', '#GetDevSceneClass()', '#GetDevSettingsClass()', '-OnInitialize()  // 하위 영역 생성', '-OnGenerateGUI()  // 나란히 표시'] },
    { id: 'SET', x: 600, y: 240, w: 210, name: 'JGDevSettings', stereo: '설정 영역 · 비어 있음', color: 'gray', members: ['+OnInitialize()  // 테두리', '+OnGenerateGUI()  // 비어 있음'] },
    { id: 'SCN', x: 840, y: 240, w: 250, name: 'JGDevScene', stereo: '그래픽스 테스트 표면', color: 'orange', members: ['-Scene : PScene', '-Renderer : PSceneRenderer', '-Camera · MeshInstance', '-Mesh : JGStaticMesh', '+OnInitialize()  // 장면 · 렌더러 생성', '+OnUpdate()  // Render · 리드백', '+OnGenerateGUI()  // HGUI::Image', '-DumpReadbackOnce()  // 30프레임 뒤'] }
  ],
  edges: [
    { from: 'FEAT', to: 'W', fs: 't', ts: 'b', end: 'triangle' },
    { from: 'SET', to: 'WC', fs: 't', ts: 'b', to_off: -60, end: 'triangle' },
    { from: 'SCN', to: 'WC', fs: 't', ts: 'b', to_off: 60, end: 'triangle' },
    { from: 'FEAT', to: 'SET', fs: 'r', ts: 'l', fo: -36, start: 'diamond', end: 'none' },
    { from: 'FEAT', to: 'SCN', fs: 'r', ts: 'l', fo: 50, to_off: 32, start: 'diamond', end: 'none' },
    { from: 'ED', to: 'MOD', fs: 't', ts: 'b', label: 'ConnectModule("Devkit")', lw: 200 }
  ]
};
