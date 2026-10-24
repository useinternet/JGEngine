// AI — 핵심 클래스 · 개념: 모듈 클래스가 없는 AI 모듈이 빌드 · 실행에 끼는 경로
// 근거: Source/Runtime/AI/AI.module.json · Main.cpp, jgengine.lua(project "AI"),
//       JGBuildTool BuildTool.cpp(CodeGenableModuleSet = SharedLib 전부, writeRuntimeModuleInfo),
//       Core ObjectGlobalSystem.cpp codeGen() · Module.cpp ConnectModule(Link_Module 없으면 언로드 · false)
module.exports = {
  file: 'AI_00_빌드와실행',
  alt: '핵심 클래스 · 개념 — JGBuildTool이 AI.module.json으로 jgengine.lua 프로젝트 · 코드젠 목록을 생성하여 AI.dll이 빌드되고 에디터 실행 시 codeGen이 로드하지만 Link_Module이 없어 건너뛰며, ConnectModule은 실패하고 게임 전용 실행 · 패키지의 런타임 모듈 목록에는 포함되지 않는다',
  type: 'flow',
  w: 940,
  h: 570,
  nodes: [
    { id: 'MJ', x: 320, y: 20, w: 300, h: 56, color: 'yellow', title: 'AI.module.json', lines: ['SharedLib · 의존 Core · Graphics'] },
    { id: 'BT', x: 320, y: 116, w: 300, h: 56, color: 'gray', title: 'JGBuildTool', lines: ['module.json 탐색 · 빌드 파일 생성'] },
    { id: 'LUA', x: 20, y: 240, w: 280, h: 64, color: 'yellow', title: 'jgengine.lua', lines: ['project "AI" · 솔루션에 포함'] },
    { id: 'MSI', x: 330, y: 240, w: 280, h: 64, color: 'yellow', title: 'module_system_info.json', lines: ['SharedLib 전부 → AI 포함'] },
    { id: 'RMI', x: 640, y: 240, w: 280, h: 64, color: 'yellow', title: 'runtime_module_info.json', lines: ['뿌리 GameHost + 게임 모듈의 의존', '→ AI 없음'] },
    { id: 'DLL', x: 20, y: 364, w: 280, h: 64, color: 'purple', title: 'AI.dll', lines: ['Bin/<구성>/ · int dummy()만 있음'] },
    { id: 'CG', x: 330, y: 364, w: 280, h: 64, color: 'blue', title: 'GObjectGlobalSystem::codeGen()', lines: ['에디터 실행 시 목록의 DLL 로드'] },
    { id: 'RG', x: 640, y: 364, w: 280, h: 64, color: 'gray', title: '게임 전용 실행(-game) · 패키지', lines: ['AI.dll을 로드하지 않음'] },
    { id: 'CM', x: 20, y: 488, w: 280, h: 64, color: 'red', title: 'ConnectModule("AI")', lines: ['JG_MODULE_IMPL 없음', '→ Link_Module 없어 언로드 · false'] },
    { id: 'SK', x: 330, y: 488, w: 280, h: 64, color: 'gray', title: 'Link_Module 없음 → 건너뜀', lines: ['등록할 클래스 없음'] }
  ],
  edges: [
    { from: 'MJ', to: 'BT' },
    { from: 'BT', to: 'LUA', fs: 'b', ts: 't', my: 206 },
    { from: 'BT', to: 'MSI', fs: 'b', ts: 't' },
    { from: 'BT', to: 'RMI', fs: 'b', ts: 't', my: 206 },
    { from: 'LUA', to: 'DLL', label: '빌드' },
    { from: 'MSI', to: 'CG', label: '코드젠 목록' },
    { from: 'RMI', to: 'RG', label: '런타임 모듈 목록' },
    { from: 'DLL', to: 'CM', label: '연결 시도', color: 'red' },
    { from: 'CG', to: 'SK', label: 'AI.dll 로드' }
  ]
};
