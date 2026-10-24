// 0-1. HAudio와 GAudioGlobalSystem — 왼쪽 = API → 전역 시스템 → 섞개(miniaudio), 오른쪽 = 처음 소리를 낼 때 장치를 여는 갈림길
// 옛 Audio_00_출력장치선택을 2026-10-24 Audio 합침(H-070 B) 뒤 구조로 다시 그림: 모듈 시작은 등록만, 장치는 처음 Play · PlayMusic · Preload 에서
// 코드: Audio/Audio.h(HAudio), Audio/AudioGlobalSystem.h · .cpp(EnsureInitialized · initialize · onWndProc), Audio/AudioEngine.h(HAudioEngine · EAudioOutput · EAudioBus),
//       Core/GameFrameWorkModule.cpp(RegisterSystemInstance<GAudioGlobalSystem>)
module.exports = {
  file: 'GameFrameWork_Audio_00_오디오구조',
  alt: 'HAudio와 GAudioGlobalSystem — 게임은 정적 API HAudio만 호출하고, GameFrameWork 모듈이 등록한 GAudioGlobalSystem이 miniaudio 섞개 HAudioEngine(Master · Music · Effects 묶음)을 소유하며, 처음 Play · PlayMusic · Preload에서 EnsureInitialized가 창이 없거나 -nosound면 무음 장치를, 아니면 기본 장치를 열고, 장치가 열리면 WM_ACTIVATEAPP으로 창 비활성 시 Master 크기를 줄이며, 열리지 않으면 Warning 로그와 함께 무음으로 동작한다',
  type: 'flow',
  w: 1040,
  h: 580,
  groups: [
    { x: 20, y: 20, w: 400, h: 540, label: '구조' },
    { x: 440, y: 20, w: 580, h: 540, label: '처음 소리를 낼 때 — EnsureInitialized' }
  ],
  nodes: [
    { id: 'API', x: 50, y: 60, w: 340, h: 70, color: 'purple', title: 'HAudio', lines: ['게임이 호출하는 정적 API · 메인 스레드', 'Play · PlayMusic · SetBusVolume'] },
    { id: 'SYS', x: 50, y: 190, w: 340, h: 70, color: 'blue', title: 'GAudioGlobalSystem', lines: ['전역 시스템 · GameFrameWork 모듈이 등록', '창 비활성 처리(SetBackgroundMode)'] },
    { id: 'ENG', x: 50, y: 320, w: 340, h: 84, color: 'green', title: 'HAudioEngine', lines: ['miniaudio 섞개', '묶음 Master · Music · Effects', '출력 Device · NullDevice · Manual'] },
    { id: 'FILE', x: 50, y: 464, w: 340, h: 70, color: 'gray', title: 'HFileHelper', lines: ['Content 토큰 경로의 파일을', '메인 스레드가 메모리로 읽음'] },
    { id: 'C', x: 520, y: 60, w: 260, h: 100, color: 'white', shape: 'diamond', title: '창이 없거나', lines: ['-nosound인가'] },
    { id: 'N', x: 820, y: 82, w: 180, h: 56, color: 'gray', title: 'NullDevice', lines: ['무음 장치 · Info'] },
    { id: 'D', x: 500, y: 210, w: 300, h: 70, color: 'blue', title: 'Device', lines: ['기본 장치', 'WASAPI → DirectSound → WinMM'] },
    { id: 'E', x: 540, y: 320, w: 220, h: 90, color: 'white', shape: 'diamond', title: '실제 장치가 열렸는가' },
    { id: 'W', x: 820, y: 337, w: 180, h: 56, color: 'yellow', title: '무음으로 동작', lines: ['Warning 로그'] },
    { id: 'O', x: 500, y: 464, w: 300, h: 70, color: 'green', title: '실제 출력', lines: ['WM_ACTIVATEAPP 구독', '창 비활성이면 Master 페이드'] }
  ],
  edges: [
    { from: 'API', to: 'SYS', label: '처음이면 EnsureInitialized' },
    { from: 'SYS', to: 'ENG', start: 'diamond', end: 'none', label: '소유' },
    { from: 'ENG', to: 'FILE', label: '소리 파일 읽기' },
    { from: 'C', to: 'N', fs: 'r', ts: 'l', label: '예' },
    { from: 'C', to: 'D', fs: 'b', ts: 't', label: '아니오' },
    { from: 'D', to: 'E' },
    { from: 'E', to: 'W', fs: 'r', ts: 'l', label: '아니오' },
    { from: 'E', to: 'O', fs: 'b', ts: 't', label: '예' }
  ]
};
