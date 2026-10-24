// 6-1. 패키지 실행 과정 — <Name>.exe(JGGameBootstrap) → JGGameLaunch_Main → GCoreSystem(패키지 모드) → 실행 모듈 → 루프 → 종료 코드
// 코드: Source/Programs/JGGameBootstrap/Main.cpp, Source/Programs/JGGameLaunch/JGGameLaunch.cpp, Core/FileIO/PackageDescriptor.h
module.exports = {
  file: 'Programs_05_패키지실행',
  alt: '패키지 실행 과정 — <Name>.exe(JGGameBootstrap)가 exe 폴더를 패키지 루트로 정하고 Bin을 DLL 검색 경로에 넣은 뒤 JGGameLaunch_Main을 같은 프로세스에서 호출하면, JGGameLaunch가 로그 파일을 고르고 GCoreSystem을 패키지 모드로 생성해 Bin/JGPackage.json의 실행 모듈(기본 GameHost)을 연결하고, Update 반복 뒤 Destroy로 종료 코드를 반환한다',
  type: 'seq',
  colW: 230,
  participants: [
    { id: 'B', label: '<Name>.exe', sub: 'JGGameBootstrap', color: 'purple' },
    { id: 'L', label: 'JGGameLaunch.dll', sub: '패키지 실행 진입점', color: 'purple' },
    { id: 'C', label: 'GCoreSystem', sub: '엔진 시작 · 프레임 · 종료', color: 'green' },
    { id: 'H', label: 'GameHost', sub: '실행 모듈', color: 'green' }
  ],
  steps: [
    { gap: 30 },
    { from: 'B', to: 'B', text: 'exe 폴더 = 패키지 루트 · Bin을 DLL 검색 경로에' },
    { from: 'B', to: 'L', text: 'JGGameLaunch_Main(루트, 명령줄)' },
    { gap: 14 },
    { from: 'L', to: 'L', text: '로그 파일 선택 — Saved/Logs/<Name>.log' },
    { from: 'L', to: 'C', text: 'Create — 패키지 모드' },
    { gap: 14 },
    { from: 'C', to: 'C', text: 'Bin/JGPackage.json — Content 마운트 · 런타임 모듈' },
    { from: 'C', to: 'H', text: 'ConnectModule(LaunchModule)' },
    { from: 'L', to: 'C', text: 'Update 반복 — -smoke=N이면 N 프레임 뒤 종료' },
    { from: 'L', to: 'C', text: 'Destroy' },
    { from: 'L', to: 'B', text: '종료 코드', dash: true }
  ]
};
