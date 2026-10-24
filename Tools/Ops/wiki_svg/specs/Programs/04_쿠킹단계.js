// 5-1. 쿠킹 단계 — 검사 · 생성 · 빌드 → .staging 에 패키지 구성(모듈 · Bin · exe · Content · 서술자) → 스모크 실행 → 교체 · 보고
// 코드: Source/Programs/JGConsole/Cook/CookCommand.cpp(step* 함수 10개), Cook/CookPlatform.cpp
module.exports = {
  file: 'Programs_04_쿠킹단계',
  alt: '쿠킹 단계 — cook.run은 프로젝트 · 구성 검사, 게임 솔루션 생성, MSBuild 빌드로 준비한 뒤 .staging 폴더에 런타임 모듈 목록 확인, Bin 배치와 import 감사, JGGameBootstrap 사본 <Name>.exe, Content 선택 · 검사 · 복사, JGPackage.json 서술자를 차례로 구성하고, 스모크 실행이 통과하면 출력 폴더와 교체해 Package/<구성>/을 만든다',
  type: 'flow',
  w: 1000,
  h: 430,
  groups: [
    { x: 20, y: 20, w: 600, h: 108, label: '준비' },
    { x: 20, y: 150, w: 960, h: 108, label: '패키지 구성 — <출력>.staging/' },
    { x: 20, y: 280, w: 410, h: 130, label: '확인 · 교체' }
  ],
  nodes: [
    { id: 'C1', x: 40, y: 56, w: 175, h: 56, color: 'purple', title: '1 검사', lines: ['프로젝트 · 구성 · 출력'] },
    { id: 'C2', x: 235, y: 56, w: 175, h: 56, color: 'purple', title: '2 생성', lines: ['게임 솔루션 다시 생성'] },
    { id: 'C3', x: 430, y: 56, w: 175, h: 56, color: 'purple', title: '3 빌드', lines: ['MSBuild <Name>.sln'] },
    { id: 'C4', x: 40, y: 186, w: 175, h: 56, color: 'blue', title: '4 모듈', lines: ['런타임 모듈 목록 확인'] },
    { id: 'C5', x: 230, y: 186, w: 175, h: 56, color: 'blue', title: '5 Bin', lines: ['DLL · import 감사 · CRT'] },
    { id: 'C6', x: 420, y: 186, w: 175, h: 56, color: 'blue', title: '6 exe', lines: ['JGGameBootstrap 사본'] },
    { id: 'C7', x: 610, y: 186, w: 175, h: 56, color: 'blue', title: '7 Content', lines: ['선택 · 검사 · 복사'] },
    { id: 'C8', x: 800, y: 186, w: 165, h: 56, color: 'blue', title: '8 서술자', lines: ['Bin/JGPackage.json'] },
    { id: 'C9', x: 40, y: 318, w: 175, h: 72, color: 'green', title: '9 스모크', lines: ['<Name>.exe -smoke=N', '오류 로그 0'] },
    { id: 'C10', x: 235, y: 318, w: 175, h: 72, color: 'green', title: '10 교체 · 보고', lines: ['staging → 출력', 'report.txt'] },
    { id: 'PKG', x: 480, y: 290, w: 500, h: 110, color: 'orange', align: 'start', title: 'Package/<구성>/', lines: ['<Name>.exe — 패키지 실행 파일', 'Bin/ — 런타임 모듈 DLL · JGGameLaunch.dll · JGPackage.json', 'Content/JGEngine · Content/JGGame — 엔진 · 게임 Content', '<구성>-Symbols/ — PDB(패키지 밖)'] }
  ],
  edges: [
    { from: 'C1', to: 'C2', fs: 'r', ts: 'l' },
    { from: 'C2', to: 'C3', fs: 'r', ts: 'l' },
    { points: [[517, 112], [517, 139], [10, 139], [10, 214], [40, 214]] },
    { from: 'C4', to: 'C5', fs: 'r', ts: 'l' },
    { from: 'C5', to: 'C6', fs: 'r', ts: 'l' },
    { from: 'C6', to: 'C7', fs: 'r', ts: 'l' },
    { from: 'C7', to: 'C8', fs: 'r', ts: 'l' },
    { points: [[882, 242], [882, 269], [10, 269], [10, 354], [40, 354]] },
    { from: 'C9', to: 'C10', fs: 'r', ts: 'l' },
    { from: 'C10', to: 'PKG', fs: 'r', ts: 'l' }
  ]
};
