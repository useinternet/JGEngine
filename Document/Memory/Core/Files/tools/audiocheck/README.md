# 오디오 재생 검증 도구 (AudioCheck)

오디오 재생(D-37, 요청 ER-022)의 엔진 밖 검증 장치다(게임 콘텐츠가 아니다). 새 `Audio` 모듈을 게임 모듈이 연결해 효과음 · 배경음 · 묶음 크기 · 창 비활성 처리를 실제 장치로 내고, 기본 출력 장치의 실제 소리 크기를 loopback 으로 재어 단계별로 판정한다.

| 파일 | 내용 |
|---|---|
| `game/AudioCheck.module.json` | 템플릿 의존에 `Audio` 를 더한 것(빌드 · 패키지에 Audio.dll 이 들어간다) |
| `game/AudioCheckModule.cpp` | GameFrameWorks · Audio 연결 → 엔트리 액터 → 월드 → `JGAudioCheckHUD` 를 Game 레이어에 |
| `game/AudioCheckHUD.h/.cpp` | 검증 화면: 왼쪽 단추 11개(효과음 · 8번 겹치기 · 곡 A(wav) · 곡 B(ogg) · 음악 멈춤 · 음악 일시 멈춤 · 분위기 소리 · 효과음/음악/전체 크기 100→50→0 · 창이 뒤로 가면 Keep→Lower→Mute), 위 글자(장치 · 재생 수 · 묶음 크기). `-audiocheck` 면 자동 순서(1~27.5초: 겹치기 → 곡 B → 곡 A 교체 → 음악 묶음 0.25 → 분위기 소리 → 전체 0 → 멈춤 페이드 → 효과음 묶음 0.5 → 창 비활성 Mute · Lower(주 창에 `WM_ACTIVATEAPP`))를 돌며 단계마다 `AUDIOCHECK <epoch ms> t=<초> <단계> playing=…` 를 남기고 끝나면 종료 0 |
| `make_sounds.py` | 시험 소리를 만든다(표준 라이브러리만): `Effect.wav`(0.2초 880Hz) · `MusicA.wav`(6초 아르페지오, 2초 주기) · `Rain.wav`(4초 잡음) |
| `audio_loopback_rms.cpp` · `build.bat` | 기본 출력 장치의 실제 소리 크기(WASAPI loopback RMS)를 50ms 마다 `RMS <epoch ms> <경과 초> <rms>` 로 쓴다. 소리는 저장하지 않는다(수치만). 다른 앱 소리도 섞이므로 판정 전 바닥값을 본다 |
| `analyze_audiocheck.py` | 게임 로그(AUDIOCHECK)와 loopback 기록을 시각으로 맞춰 9가지를 판정 → `AUDIOCHECK RESULT: PASS (9/9)`(종료 0) / `FAIL`(종료 6) |

## 재현

```
Build\BatchFiles\CreateGameProject.bat <dir>\AudioCheck AudioCheck
copy game\* → <dir>\AudioCheck\Source\AudioCheck\        (AudioCheckEntryActor · AudioCheckEditor 는 템플릿 그대로)
python make_sounds.py <dir>\AudioCheck\Content\Sounds
copy "C:\Program Files (x86)\Android\android-sdk\platforms\android-36\data\res\raw\fallbackring.ogg" <dir>\AudioCheck\Content\Sounds\MusicB.ogg
   (아무 짧은 ogg 면 된다 — 저장소에는 넣지 않는다. 이 PC 에 있던 Android SDK 의 소리를 썼다)
<dir>\AudioCheck\GenerateProjectFiles.bat
MSBuild <dir>\AudioCheck\AudioCheck.sln -m -p:Configuration=DevelopEngine -p:Platform=x64
build.bat <엔진 루트> <출력 폴더>                           (audio_loopback_rms.exe — miniaudio.h 는 엔진 Source/ThirdParty/miniaudio)
audio_loopback_rms.exe 33 50 > loop.txt   과 동시에   <dir>\AudioCheck\Bin\DevelopEngine\JGLauncher.exe -game -audiocheck > game.txt
python analyze_audiocheck.py game.txt loop.txt               → AUDIOCHECK RESULT: PASS (9/9)
패키지: JGConsole.exe -runCook -project=<dir>\AudioCheck -config=ReleaseGame → Package\ReleaseGame\AudioCheck.exe -audiocheck
   (패키지 로그는 Package\ReleaseGame\Saved\Logs\AudioCheck.log — 판정에 그 파일을 준다. 끝나면 Saved\ 는 지운다)
무음 확인: ... -audiocheck -nosound → 로그 "Audio: silent mode (-nosound)", device=0, loopback 최대 0
```

## 판정 9가지 (`analyze_audiocheck.py`)

| # | 판정 | 기준 |
|---|---|---|
| (a) | 효과음 8번 겹치기 | 겹친 구간 에너지 ÷ 효과음 하나 에너지의 제곱근 > 2.0(겹치지 않으면 1 이하), 겹치기 끝 재생 수 ≥ 3 |
| (b) | 곡 B(ogg, 흘려 풀기)가 들린다 | 바닥값 × 10 이상 |
| (b) | B → A 교체(2초 페이드) 동안 끊김 없음 · A 되풀이 | 교체 구간 250ms 칸 최소값이 문턱 이상 |
| (c) | 음악 묶음 0.25 | 같은 곡 같은 위상 2초 평균 비 0.20~0.30 |
| (c) | 전체 0 → 무음, 1 → 돌아옴 | 0 일 때 < 1e-4 |
| (b) | 멈춤 1초 페이드 | 줄어들다 무음 |
| (c) | 효과음 묶음 0.5 | 에너지 비의 제곱근 0.45~0.55(짧은 소리는 50ms 칸 경계에 흔들리지 않게 에너지로) |
| 창 | 창 비활성 + Mute → 무음, 활성 → 돌아옴 | 무음 < 1e-4 |
| 창 | 창 비활성 + Lower(0.3) → 0.3, 활성 → 1 | 0.25~0.35 / 0.9~1.1 |

결과(2026-10-11): 작업 폴더 엔진 GameHost 2회 · 에디터 1회, 메인 엔진 GameHost 1회, ReleaseGame 패키지 1회 모두 9/9. 기록 `../../2026-10-11_audio_D-37_verify.txt`.
