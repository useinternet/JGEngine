// 패키지 실행 — JGGameBootstrap → JGGameLaunch
module.exports = {
  file: '엔진프로그램_00_패키지실행',
  alt: '패키지 실행(JGGameBootstrap → JGGameLaunch) — 패키지 실행 파일(Name.exe)이 패키지 루트와 DLL 디렉터리를 정하고 JGGameLaunch.dll의 JGGameLaunch_Main을 호출하면, JGGameLaunch가 GCoreSystem을 생성해 JGPackage.json의 실행 모듈(기본 GameHost)을 연결하고 Update 반복 뒤 Destroy로 종료 코드를 반환한다',
  type: 'seq',
  colW: 230,
  participants: [
    { id: 'B', label: '<Name>.exe', sub: 'JGGameBootstrap', color: 'purple' },
    { id: 'L', label: 'JGGameLaunch.dll', sub: '패키지 실행 진입점', color: 'blue' },
    { id: 'C', label: 'GCoreSystem', sub: 'Core 관리자', color: 'green' },
    { id: 'H', label: 'GameHost', sub: '실행 모듈 기본값', color: 'orange' }
  ],
  steps: [
    { gap: 30 },
    { from: 'B', to: 'B', text: 'DPI 인식 설정 · exe 폴더 = 패키지 루트 · SetDllDirectory(Bin)' },
    { from: 'B', to: 'L', text: 'LoadLibrary → JGGameLaunch_Main(루트, 명령줄)' },
    { gap: 14 },
    { from: 'L', to: 'L', text: '에디터 빌드면 종료 코드 4 · 로그 파일 선택 (_2 … _32)' },
    { from: 'L', to: 'C', text: 'Create(패키지 루트, 로그 경로)' },
    { gap: 14 },
    { from: 'C', to: 'C', text: 'Bin/JGPackage.json 읽기 · 런타임 모듈 코드젠' },
    { from: 'C', to: 'H', text: 'ConnectModule(LaunchModule)' },
    { from: 'L', to: 'C', text: 'Update 반복 — smoke면 N 프레임 뒤 종료' },
    { from: 'L', to: 'C', text: 'Destroy → 종료 코드 반환' }
  ]
};
