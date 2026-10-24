// Devkit — 어떻게 여나: imgui.ini 의 JGDevFeature=1 줄로 GUI 모듈 레이아웃 복원이 첫 프레임에 창을 연다
// 근거: Source/Runtime/GUI/GUIModule.cpp(RegisterLayoutSettings · RestoreLayoutWidgets · UpdateWidgets),
//       Source/Runtime/GUI/Widget.cpp(Initialize · Open), Document/Memory/tools/capture_devscene.ps1(-WorkDir)
module.exports = {
  file: 'Devkit_01_창여는과정',
  alt: '어떻게 여나 — 작업 폴더 imgui.ini의 JGDevFeature=1 줄을 HGUIModule이 시작 시 읽고, 첫 프레임 RestoreLayoutWidgets가 등록된 클래스이면 JGDevFeature를 생성 · 초기화하여 열며, 이후 매 프레임 JGDevScene의 OnUpdate · OnGenerateGUI가 호출된다',
  type: 'flow',
  w: 780,
  h: 790,
  nodes: [
    { id: 'INI', x: 40, y: 20, w: 300, h: 76, color: 'yellow', shape: 'note', title: 'imgui.ini (런처 작업 폴더)', lines: ['[JGWidget][Open]', 'JGDevFeature=1'] },
    { id: 'WD', x: 440, y: 20, w: 310, h: 76, color: 'gray', shape: 'note', title: 'capture_devscene.ps1 -WorkDir', lines: ['이 imgui.ini가 있는 폴더를', '작업 폴더로 런처 실행'] },
    { id: 'GS', x: 40, y: 150, w: 300, h: 70, color: 'purple', title: 'HGUIModule::StartupModule', lines: ['RegisterLayoutSettings → ini 읽기', '창 이름별 열림 상태 저장'] },
    { id: 'FF', x: 40, y: 270, w: 300, h: 64, color: 'purple', title: '첫 프레임 GenerateWidgetGUI', lines: ['RestoreLayoutWidgets'] },
    { id: 'DIA', x: 70, y: 380, w: 240, h: 90, color: 'white', shape: 'diamond', title: '클래스가 등록되었는가' },
    { id: 'SKIP', x: 440, y: 395, w: 310, h: 60, color: 'gray', title: '건너뜀 · 항목 유지', lines: ['그 위젯의 모듈이 이번 실행에 없음'] },
    { id: 'NEW', x: 40, y: 520, w: 300, h: 64, color: 'blue', title: 'NewObject(JGDevFeature)', lines: ['Initialize → 하위 영역 생성 · 초기화'] },
    { id: 'MENU', x: 440, y: 520, w: 310, h: 64, color: 'red', shape: 'note', title: '메뉴 · 코드 호출 없음', lines: ['DevScene 여는 메뉴(Graphics 5-32) 미착수'] },
    { id: 'OPEN', x: 40, y: 620, w: 300, h: 50, color: 'green', title: 'Open', lines: ['창 "DevFeature" 열림'] },
    { id: 'FRAME', x: 40, y: 706, w: 300, h: 70, color: 'green', title: '매 프레임', lines: ['Update 단계: JGDevScene::OnUpdate', 'GUI 그리기: OnGenerateGUI'] }
  ],
  edges: [
    { from: 'WD', to: 'INI', fs: 'l', ts: 'r', dash: true },
    { from: 'INI', to: 'GS', label: 'LoadIniSettingsFromDisk', lw: 200 },
    { from: 'GS', to: 'FF' },
    { from: 'FF', to: 'DIA' },
    { from: 'DIA', to: 'SKIP', fs: 'r', ts: 'l', label: '아니오' },
    { from: 'DIA', to: 'NEW', fs: 'b', ts: 't', label: '예' },
    { from: 'NEW', to: 'OPEN' },
    { from: 'OPEN', to: 'FRAME' }
  ]
};
