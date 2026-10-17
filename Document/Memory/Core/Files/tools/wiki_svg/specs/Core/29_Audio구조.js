// 7-1. GAudioGlobalSystem과 HAudioEngine — Audio 모듈의 재생 API · 전역 시스템 · miniaudio 래퍼와 Core 연결
// 코드: Runtime/Audio/Audio.h/.cpp(HAudio), AudioGlobalSystem.h/.cpp, AudioEngine.h/.cpp, AudioModule.cpp, AudioDefines.h
module.exports = {
  file: 'Core_29_Audio구조',
  alt: 'GAudioGlobalSystem과 HAudioEngine — HAudioModule이 GAudioGlobalSystem을 GCoreSystem에 등록하고, GAudioGlobalSystem은 HAudioEngine을 소유하며 창 이벤트를 구독하고, 재생 API HAudio는 HAudioEngine을 호출하며, HAudioEngine은 HFileHelper로 파일을 읽어 miniaudio로 재생한다',
  type: 'class',
  w: 1130,
  h: 520,
  groups: [
    { x: 900, y: 20, w: 210, h: 330, label: 'Core' }
  ],
  classes: [
    { id: 'MOD', x: 40, y: 30, w: 340, name: 'HAudioModule', stereo: 'Audio 모듈 클래스 · IModuleInterface 상속', color: 'purple', members: ['#StartupModule()  // 시스템 등록 · 초기화', '#ShutdownModule()  // 시스템 해제'] },
    { id: 'API', x: 40, y: 170, w: 340, name: 'HAudio', stereo: '재생 API · 정적 함수', color: 'purple', members: ['+Play(path, args) : HAudioHandle', '+Stop() · Pause() · Resume()', '+PlayMusic() · StopMusic()', '+SetBusVolume() · SetBackgroundMode()'] },
    { id: 'GAS', x: 520, y: 30, w: 320, name: 'GAudioGlobalSystem', stereo: '오디오 전역 시스템 · 모듈이 등록', color: 'green', members: ['-_engine : HAudioEngine', '-_wndProcHandle : 창 이벤트 구독', '-_backgroundMode : Keep · Lower · Mute', '+Initialize() · Shutdown()', '#Update()  // HAudioEngine::Update 호출'] },
    { id: 'HAE', x: 520, y: 240, w: 320, name: 'HAudioEngine', stereo: 'miniaudio 래퍼 · 메인 스레드 전용', color: 'blue', members: ['-ma_engine · 버스 3개(Master · Music · Effects)', '-재생 인스턴스 · 원본 사운드 캐시', '-메모리 파일 VFS', '+Play() · Update() · FadeOutputGain()'] }
  ],
  nodes: [
    { id: 'DEF', x: 40, y: 350, w: 340, h: 96, color: 'yellow', title: 'AudioDefines', lines: ['HAudioHandle — 재생 식별자', 'HAudioPlayArguments — 재생 인자(버스 · 볼륨 · 반복 등)', 'EAudioBus · EAudioBackgroundMode'] },
    { id: 'GCS', x: 920, y: 60, w: 170, h: 56, color: 'green', title: 'GCoreSystem', lines: ['전역 시스템 목록'] },
    { id: 'WC', x: 920, y: 150, w: 170, h: 56, color: 'green', title: 'WndProc 이벤트', lines: ['HWindowCallBacks'] },
    { id: 'FH', x: 920, y: 270, w: 170, h: 56, color: 'blue', title: 'HFileHelper', lines: ['파일 · 디렉터리'] },
    { id: 'MA', x: 920, y: 400, w: 170, h: 56, color: 'gray', title: 'miniaudio 0.11.25', lines: ['ThirdParty'] },
    { id: 'KEY', x: 520, y: 420, w: 320, h: 70, color: 'white', shape: 'note', title: '범례', lines: ['◇— 소유 · → 호출 · 사용 · - - → 등록 · 구독', '+ public · # protected · - private'] }
  ],
  edges: [
    { from: 'MOD', to: 'GAS', fs: 'r', ts: 'l', fo: 0, to_off: -27, dash: true, label: '등록 · 초기화' },
    { from: 'GAS', to: 'GCS', fs: 'r', ts: 'l', fo: -14, to_off: 0, dash: true, label: '목록에 추가' },
    { points: [[840, 150], [880, 150], [880, 178], [920, 178]], dash: true, label: '구독', lx: 880, ly: 164, lpos: 'l' },
    { from: 'GAS', to: 'HAE', fs: 'b', ts: 't', start: 'diamond', end: 'none', label: '소유' },
    { from: 'API', to: 'HAE', fs: 'r', ts: 'l', fo: 35, to_off: -35, label: 'GetEngine()로 호출' },
    { from: 'HAE', to: 'FH', fs: 'r', ts: 'l', fo: -5, to_off: 0, label: '파일 읽기' },
    { points: [[840, 345], [880, 345], [880, 428], [920, 428]], label: '재생', lx: 880, ly: 390 },
    { from: 'API', to: 'DEF', fs: 'b', ts: 't', label: '인자 · 핸들' }
  ]
};
