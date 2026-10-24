// 1-2. 게임 모듈 연결과 설정 파일 — 실행 모듈(JGEditor · GameHost)이 엔진 모듈과 게임 모듈을 연결하고, 목록은 .jgproject · JGPackage.json 에서 읽는다
// 코드: Core/CoreSystem.cpp(Create), Core/Object/ObjectGlobalSystem.cpp(codeGen), Core/Misc/Module.cpp(HModuleSystemInfo::Get),
//       Editor/JGEditor/JGEditor.cpp(StartupModule · connectProjectModules), Runtime/GameHost/GameHostModule.cpp(loadSettings · connectModules)
module.exports = {
  file: 'Core_24_게임모듈연결',
  alt: '게임 모듈 연결과 설정 파일 — GCoreSystem::Create가 리플렉션 등록용 DLL을 로드한 뒤 실행 모듈을 연결하고, JGEditor는 .jgproject의 GameModules · EditorModules를, GameHost는 .jgproject 또는 JGPackage.json의 GameModules를 연결한다',
  type: 'flow',
  w: 900,
  h: 760,
  nodes: [
    { id: 'C', x: 250, y: 20, w: 300, h: 56, color: 'blue', title: 'GCoreSystem::Create', lines: ['베이스 시스템 8개 등록 후'] },
    { id: 'R', x: 250, y: 116, w: 300, h: 74, color: 'green', title: 'GObjectGlobalSystem::codeGen', lines: ['리플렉션 등록용 DLL 로드', 'Engine_CodeGenerate 호출'] },
    { id: 'J0', x: 620, y: 112, w: 250, h: 82, color: 'white', shape: 'note', title: '모듈 목록 파일', lines: ['에디터: module_system_info.json', '게임만: runtime_module_info.json', '패키지: JGPackage.json RuntimeModules'], small: true },
    { id: 'L', x: 250, y: 230, w: 300, h: 56, color: 'blue', title: 'ConnectModule(LaunchModule)', lines: ['실행 모듈 연결'] },
    { id: 'Q', x: 290, y: 326, w: 220, h: 80, color: 'white', shape: 'diamond', title: '실행 모듈' },
    { id: 'E1', x: 40, y: 450, w: 320, h: 74, color: 'purple', title: 'HJGEditorModule::StartupModule', lines: ['창 생성', '엔진 모듈 연결(Graphics, GUI, GameFrameWork)'] },
    { id: 'E2', x: 40, y: 564, w: 320, h: 56, color: 'purple', title: 'connectProjectModules', lines: ['GameModules → EditorModules 순서로 연결'] },
    { id: 'G1', x: 540, y: 450, w: 320, h: 74, color: 'purple', title: 'HGameHostModule::StartupModule', lines: ['창 생성', '엔진 모듈 연결(Graphics, GameFrameWork)'] },
    { id: 'G2', x: 540, y: 564, w: 320, h: 56, color: 'purple', title: 'connectModules', lines: ['GameModules 연결(에디터 모듈은 연결 안 함)'] },
    { id: 'JP', x: 40, y: 664, w: 320, h: 62, color: 'yellow', shape: 'note', title: '<프로젝트>.jgproject', lines: ['GameModules · EditorModules'] },
    { id: 'PK', x: 540, y: 664, w: 320, h: 62, color: 'yellow', shape: 'note', title: 'Bin/JGPackage.json', lines: ['LaunchModule · GameModules · RuntimeModules'] }
  ],
  edges: [
    { from: 'C', to: 'R' },
    { from: 'J0', to: 'R', fs: 'l', ts: 'r', dash: true, label: '읽음' },
    { from: 'R', to: 'L' },
    { from: 'L', to: 'Q' },
    { from: 'Q', to: 'E1', fs: 'l', ts: 't', via: [[200, 366]], label: '에디터 — JGEditor', lx: 200, ly: 410, lpos: 'l' },
    { from: 'Q', to: 'G1', fs: 'r', ts: 't', via: [[700, 366]], label: '게임 — GameHost', lx: 700, ly: 410 },
    { from: 'E1', to: 'E2' },
    { from: 'G1', to: 'G2' },
    { from: 'JP', to: 'E2', fs: 't', ts: 'b', dash: true, label: '읽음', lx: 200, ly: 642 },
    { from: 'JP', to: 'G2', fs: 'r', ts: 'b', dash: true, via: [[460, 695], [460, 642], [640, 642]], label: '개발 실행', lx: 460, ly: 668, lpos: 'l' },
    { from: 'PK', to: 'G2', fs: 't', ts: 'b', dash: true, fo: 60, to_off: 60, label: '패키지', lx: 760, ly: 642 }
  ]
};
