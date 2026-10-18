# Audio TODO (오디오 재생 — 효과음 · 배경음, 새 런타임 모듈 `Audio`)

갱신 2026-10-17 14:24 — 사람이 듣는 확인을 사용자 건너뜀(10-17 14:21, 통합 대시보드 결정함 — 엔진 총괄 전달) — 지금 구현 그대로(아래 "요청서 완료 기준" 줄). 실제 장치 없는 PC 는 보류 AU-H9 그대로. 그 전 갱신 2026-10-11 14:03 — **AU-1 ~ AU-4 완료 · 메인 반영(14:02), 커밋 `ee36bb1`(10-17)**(커밋은 사용자 — 아래 체크리스트). 사용자 결정 오디오 재생(D-37) "권고대로 진행"(12:51, 엔진 총괄 전달), 담당 Core 세션.
검토서 `Files/ER-022_검토_2026-10-11.md` · 검증 기록 `Files/2026-10-11_audio_D-37_verify.txt` · 검증 도구 `Files/tools/audiocheck/`(README.md) · 현황 `현황.md` §3-5. 요청서 `ProjectAH/Document/엔진요청/ER-022_오디오_재생_효과음_·_배경음.md`.
범위: 게임이 효과음 · 배경음을 내고 묶음(버스)마다 크기를 조절하는 엔진 시스템. 무슨 소리를 언제 낼지는 게임 영역이다(지침서 1절).
기준: 엔진 기반 구축 · 과설계 금지.

---

## 결정 (2026-10-11)

| 항목 | 결정 |
|---|---|
| 만들지 · 무엇으로 | (가) miniaudio 0.11.25(퍼블릭 도메인 또는 MIT-0)를 감싼 새 `Audio` 모듈 — 사용자 "권고대로 진행"(12:51) |
| 담당 | (A) Core 세션 |
| 바깥 파일 내려받기 | 사용자 허락(채팅 13시): `miniaudio.h` 4,108,168바이트 · `LICENSE` 2,597바이트, 출처 GitHub `mackron/miniaudio` 0.11.25 태그 → `Source/ThirdParty/miniaudio/`(sha256 은 검증 기록) |
| 시험 소리 | 코드 · `make_sounds.py` 로 만든 소리와 이 PC 의 Android SDK ogg 를 작업 폴더 · 검증 게임에서만 쓴다. 저장소에는 넣지 않는다 |
| JGConsole 엔진 목록 `Audio` | DevOps 승인 — Core 가 메인 반영 hold 안에서 넣음(DevOps 가 준 주석 그대로) |

## 할 일

| ID | 항목 | 상태 | 근거 |
|---|---|---|---|
| AU-1 | 모듈 뼈대: `Source/Runtime/Audio`(`Audio.module.json` SharedLib · 의존 Core), miniaudio · stb_vorbis(저장소에 있던 1.22) 넣기, 시작 · 종료, 장치 이름 로그, 장치 없음 → Warning + 무음 장치, `-nosound` · 창 없는 실행 → 무음 장치, 창 비활성(`WM_ACTIVATEAPP`) | **완료 2026-10-11** | 작업 폴더 빌드 63초 오류 0, 메인 빌드 오류 0 · 경고 0, Audio 다시 빌드 10.4초 |
| AU-2 | API `HAudio`: 효과음(겹치기 · 미리 풀기 · 크기 · 높낮이) · 배경음(`PlayMusic` 되풀이 · 교체 페이드 · 여러 개) · 멈춤(페이드) · 일시 멈춤 · 이어 · 묶음 크기(Master · Music · Effects) · 창 비활성 모드(Keep · Lower · Mute). 파일: 토큰 경로 → Content, `HFileHelper::ReadAllBytes` 통째 읽기, wav · ogg(mp3 · flac 도 풀림) | **완료 2026-10-11** | `audio.selftest` 52/52, 검증 게임 9/9 |
| AU-3 | 자체 검사 `audio.selftest`(헤드리스 — 장치 없이 섞은 결과를 직접 읽어 확인) + JGConsole 엔진 목록 `Audio` + 회귀 | **완료 2026-10-11** | 메인 52/52 · 회귀 10종 종료 0 · live 0 |
| AU-4 | 검증 게임(`Temp/Games/AudioCheck`) · 실제 출력 녹음(loopback) 판정 · 쿠킹 패키지 · 메인 반영 · 쓰는 법(게임 연출 구현) · 생성 가이드 단락(DevOps) | **완료 2026-10-11**(생성 가이드 9절 "소리 쓰기" · 2 · 5 · 6절은 DevOps 가 10-11 반영 — `DevOps/Files/2026-10-11_Audio_가이드_템플릿배치_검증.txt`) | 작업 폴더 GameHost 2 · 에디터 1 · 메인 GameHost 1 · ReleaseGame 패키지 1 모두 9/9, `-nosound` 무음 |

요청서 완료 기준: (a) 겹치기 · (b) 되풀이 · 페이드 · (c) 묶음 크기 즉시 · (d) 패키지에서 소리 · (e) 안 쓰는 게임 그대로 · 장치 없음 · 헤드리스 — 모두 자동 판정 통과(검증 기록). **사람이 듣는 확인은 사용자 건너뜀(10-17 14:21, 통합 대시보드 결정함) — 지금 구현 그대로.** 실제 장치 없는 PC 는 보류 AU-H9 그대로. 듣기용으로 만든 검증 게임 `Temp\Games\AudioCheck\Bin\DevelopEngine\JGLauncher.exe -game`(단추 11개) · 패키지 `Temp\Games\AudioCheck\Package\ReleaseGame\AudioCheck.exe`(`-audiocheck` = 27초 자동 순서)는 Temp 에 그대로 있다(git 무시).

## 커밋 체크리스트 (오디오 재생 D-37) — 커밋됨 `ee36bb1`(2026-10-17 15:03, 엔진 총괄)

- 새 **미추적** `Source/Runtime/Audio/`(12파일: `Audio.module.json` · `AudioDefines.h` · `Audio.h/.cpp` · `AudioEngine.h/.cpp` · `AudioGlobalSystem.h/.cpp` · `AudioModule.h/.cpp` · `MiniAudio.cpp` · `AudioSelfTest.cpp`)
- 새 **미추적** `Source/ThirdParty/miniaudio/`(`miniaudio.h` · `LICENSE`)
- 수정 `Source/Programs/JGConsole/Main.cpp`(엔진 목록 `"Audio"` 세 줄 — DevOps 승인, DevOps 커밋 목록에도 적힘)
- 다시 생성 `JGEngine.sln` · `jgengine.lua` · `Source/Programs/JGBuildTool/jgengine.lua`(Audio 프로젝트. 다른 트랙의 미커밋 변경과 같은 파일)
- 재빌드 Bin: 새 `Bin/DevelopEngine/Audio.{dll,exp,lib}`, 수정 `JGConsole.exe`(10-11 13:5x)
- 문서 `Core/{TODO_Audio.md, TODO.md, 현황.md, Files/ER-022_검토_2026-10-11.md, Files/2026-10-11_audio_D-37_verify.txt, Files/tools/audiocheck/*}` · `진행현황.md` · `README.md`
- 다른 트랙과 겹치는 소스 없음. 새 모듈이라 클린 체크아웃 · 다른 PC 는 `GenerateProjectFiles.bat` 를 다시 돌린다. 메인 엔진을 쓰는 게임 프로젝트는 엔진 생성 → 게임 배치 2(`GenerateProjectFiles.bat`)

## 보류 (범위 밖 — 검토서 §8)

| ID | 항목 | 다시 볼 조건 |
|---|---|---|
| AU-H1 | 연출 큐 "소리 단계"(타입 있는 단계) | 게임 큐 코드 · UI 묶음 부르기 단계로 부족하다는 요청이 올 때 |
| AU-H2 | 소리 에셋(`.jgasset` — 무작위 높낮이 · 여러 변형 · 기본 묶음) | 같은 소리 설정이 여러 곳에서 되풀이될 때 |
| AU-H3 | 3D 위치 소리 · 울림 같은 효과 | 3D 월드에서 소리 위치가 필요할 때 |
| AU-H4 | 묶음 더하기(목소리 · 분위기 등) | 세 묶음으로 설정 화면을 못 만들 때 |
| AU-H5 | 디스크에서 조금씩 읽는 파일 API(지금은 통째 읽기 — 한 번 읽은 파일은 모듈이 끝날 때까지 메모리에 둔다) | 수백 MB 소리 파일이 생기거나 소리 파일 메모리가 문제로 보일 때 |
| AU-H6 | ogg 곡 중간으로 감기 | 곡 중간 재생 요청이 올 때 |
| AU-H7 | 쿠킹 스모크 무음(`-nosound` 를 스모크 명령에) | 스모크 중 소리가 거슬린다는 말이 나올 때 |
| AU-H8 | 데이터 테이블 AssetRef 의 소리 경로 경고(`HAssetPath` "NOT Support Format") | 게임이 소리 경로를 AssetRef 칸에 쓸 때 |
| AU-H9 | 실제 장치가 없는 PC 에서 miniaudio 가 무음 장치로 여는지(소스로만 확인 — 시험은 `-nosound` · 무음 장치 강제로 대신) | 사용자 확인 결과가 다르거나 장치 없는 PC 에서 시작 오류가 보고될 때 |
| AU-H10 | COM 초기화 방식(miniaudio WASAPI) — 엔진은 지금 COM 을 쓰지 않는다 | 메인 스레드에서 단일 스레드 방식 COM(파일 대화 상자 등)을 쓰게 될 때 |
