// 6-1. HFileHelper와 HCoreSystemGlobalValues — 파일 함수와 루트 경로, 프로젝트 · 패키지 서술자
// 코드: Core/FileIO/FileHelper.h/.cpp, ProjectDescriptor.h/.cpp, PackageDescriptor.h/.cpp, Json.h, Jsonable.h, Core/CoreSystem.h/.cpp(resolveRootDirectories)
module.exports = {
  file: 'Core_28_파일시스템구조',
  alt: 'HFileHelper와 HCoreSystemGlobalValues — HFileHelper는 HCoreSystemGlobalValues의 엔진 · 프로젝트 루트로 디렉터리 경로를 만들고 파일을 읽고 쓰며, HCoreSystemGlobalValues는 초기화 시 HProjectDescriptor 또는 HPackageDescriptor로 채워지고, 토큰 경로를 쓰는 모듈은 HFileHelper의 디렉터리 함수를 사용한다',
  type: 'class',
  w: 1040,
  h: 600,
  classes: [
    { id: 'HFH', x: 40, y: 30, w: 420, name: 'HFileHelper', stereo: '파일 · 경로 도우미 · 정적 함수', color: 'blue', members: ['+ReadAllText() · ReadAllBytes()', '+WriteAllText() · WriteAllTextAtomic()', '+Exists() · CreateDirectory() · FileListInDirectory()', '+CombinePath() · AbsolutePath() · NormalizePath()', '+EngineDirectory() · ProjectDirectory()', '+EngineContentDirectory() · GameContentDirectory()', '+SavedDirectory() · EngineShaderDirectory()'] },
    { id: 'GC', x: 540, y: 30, w: 440, name: 'GCoreSystem', stereo: '관리자', color: 'green', members: ['+Create()  // 엔진 · 프로젝트 루트 결정'] },
    { id: 'GV', x: 540, y: 150, w: 440, name: 'HCoreSystemGlobalValues', stereo: '모든 DLL이 공유하는 전역 값', color: 'green', members: ['EngineDirectory · ProjectDirectory', 'ProjectName · bProjectMode', 'PackageLaunchModule · PackageRuntimeModules'] },
    { id: 'HPD', x: 540, y: 320, w: 210, name: 'HProjectDescriptor', stereo: '.jgproject', color: 'purple', members: ['Name · EngineRoot', 'GameModules · EditorModules', '+Load() · ResolveEngineRoot()'] },
    { id: 'HPK', x: 770, y: 320, w: 210, name: 'HPackageDescriptor', stereo: 'JGPackage.json', color: 'purple', members: ['Name · Version · LaunchModule', 'RuntimeModules · GameModules', '+Load() · Save()'] },
    { id: 'IJS', x: 540, y: 490, w: 440, name: 'IJsonable', stereo: 'JSON 직렬화 인터페이스', color: 'orange', members: ['+WriteJson() · ReadJson()  // PJson · PJsonData(rapidjson 래퍼)'] }
  ],
  nodes: [
    { id: 'USR', x: 40, y: 290, w: 420, h: 80, color: 'gray', title: '토큰 경로를 쓰는 모듈', lines: ['Asset(HAssetPath) · Audio(HAudioEngine)', '/JGEngine/ · /JGGame/ → Content 디렉터리'] },
    { id: 'KEY', x: 40, y: 430, w: 420, h: 80, color: 'white', shape: 'note', title: '범례', lines: ['◇— 보관 · —▷ 상속 · → 호출 · 조회 · - - → 값 채움', '+ public · - private'] }
  ],
  edges: [
    { from: 'GC', to: 'GV', fs: 'b', ts: 't', start: 'diamond', end: 'none', label: '보관' },
    { from: 'HFH', to: 'GV', fs: 'r', ts: 'l', fo: 70, to_off: -14, label: '루트 경로 조회' },
    { from: 'HPD', to: 'GV', fs: 't', ts: 'b', fo: 0, to_off: -115, dash: true, label: '에디터 빌드' },
    { from: 'HPK', to: 'GV', fs: 't', ts: 'b', fo: 0, to_off: 115, dash: true, label: '패키지 빌드' },
    { from: 'HPD', to: 'IJS', fs: 'b', ts: 't', fo: 0, to_off: -115, end: 'triangle' },
    { from: 'HPK', to: 'IJS', fs: 'b', ts: 't', fo: 0, to_off: 115, end: 'triangle' },
    { from: 'USR', to: 'HFH', fs: 't', ts: 'b', label: '디렉터리 조회 · 파일 읽기' }
  ]
};
