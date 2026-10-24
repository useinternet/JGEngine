# GameFrameWork 이어받기 메모

갱신: 2026-10-24 19:02 (GameFrameWork 세션)

## 지금

| 항목 | 상태 |
|---|---|
| 맡은 일 | ① Engine-30 컨텐츠 브라우저용 ListAssetFolder · ImportAssetFile — "검토 중". 패치 `Temp/ContentBrowser/GameFrameWork.patch` 를 Editor 세션과 엔진 총괄에 알림(Editor 에 선언 · 동작 답도 보냄). ② Engine-46 위키 02. GameFrameWork 재검토 — "검토 중", 기록 `Document/기록/GameFrameWork/Engine-46_GameFrameWork문서_재검토_2026-10-24.md`(문서는 안 고침). ③ Engine-51(Graphics 의 3D 계획) GameFrameWork 몫 확인 — 19:04 답 보냄: 모두 맞음 + 덧붙임(FBX 위 축 · 단위는 파일 안 설정을 기본값으로, 법선 역전치 · 거울 감김, 머터리얼 이름 접미사, 클립은 뼈 이름 키, 시험 그룹 animation). 구현은 사용자 확인 뒤 단계별 일감. Engine-28(PNG · TGA 가져오기)은 In-Wait |
| 상태 줄 | `Saved/Feedback/status/gameframework.txt` = "Engine-46 GameFrameWork 위키 재검토 — 검토 중" |
| 잠금 · 빌드 · 시험 | 없음. 메인 트리는 쓰지 않았다(워크트리 `Temp/wt/cb-gfw` 에서 빌드 · 시험) |
| Engine-30 자리 | 워크트리 `Temp/wt/cb-gfw`(c6dd99d 위, 분리 HEAD). 새 파일 `Asset/AssetFolder.h` · `.cpp` · `SelfTest/AssetFolderSelfTest.cpp`, gfw.selftest 에 그룹 `asset`. 증적 `Temp/wt/cb-gfw/Temp/verify/cb/`(jg_log · editor/run_*.png) |
| Engine-26 자리 | 기록 `Document/기록/GameFrameWork/Engine-26_언리얼에셋가져오기_2026-10-24.md`. git 밖: 내보내기 프로젝트 · 스크립트 · 결과 `Temp/UnrealExport/`, 시험 게임 `Temp/Games/UnrealAssetTest`(Content/Unreal 31개, 캡처 Temp/verify/editor_start.png). Epic 라이선스라 저장소 · 엔진 Content 에 넣지 않음 |
| 미커밋 파일 | `Document/기록/GameFrameWork/Engine-46_…재검토_2026-10-24.md`(새 기록) · 이 메모. Engine-30 패치 5파일(위 새 파일 3 + `SelfTest/GameFrameWorkSelfTest.h` · `.cpp`) — Editor 패치와 함께 엔진 총괄이 메인에 붙여 커밋. 2단계는 51fb1da, 뒤따른 주석 정리는 710a7af · 2615da6 |

## 담당

- `Source/Runtime/GameFrameWork` 전체: Core · Actors · Components · GameMaster · Network · UI · Viewport · Data · Asset · Audio · SelfTest.
- `Source/Runtime/GameHost`.
- 쿠킹 · JGGameLaunch · `JGLauncher -game` · BuildTool 의 GameHost 뿌리 규칙은 DevOps 몫.

## 10-24 2단계에서 한 일 (모두 51fb1da)

| 일감 | 내용 | 기록(`Temp/개편검토/`) |
|---|---|---|
| Engine-6 | Audio 를 GameFrameWork 로 합침 — 모듈 시작은 등록만, 장치는 첫 소리에 엶 | `GameFrameWorks_Audio합침_*` |
| Engine-7 | Asset · 메시 · 텍스처 에셋 · FBX 가져오기를 GameFrameWork 로. 에셋 DB 는 Graphics 가 연결된 프로세스에서만 등록 | `GameFrameWorks_Asset이전_*` |
| Engine-8 | 자체 시험 모음 `gfw.selftest` + 옛 명령 6개 별칭 | `GameFrameWorks_자체시험모음_*` |
| Engine-11 | GameFrameWorks → GameFrameWork 이름 바꾸기 · 2단계 묶음 검증 | `2단계_묶음_검증_2026-10-24.txt` · `GameFrameWork_이름바꾸기_*` |

게임 팀 고칠 곳 목록은 `Temp/개편검토/게임팀_고칠곳_2026-10-24.md` 다. 게임 팀은 51fb1da 위에서 빌드 · 자체 테스트를 통과했다.

## 위키

- `C:/obsidian/ProjectJG/02. 위키/JGEngine/02. GameFrameWork/` 에 문서 10개가 있다: GameFrameWork 개요, 00. Game UI, 01. 월드와 액터, 02. 규칙 커널(GameMaster), 03. 네트워크, 04. 데이터 테이블, 05. 에셋, 06. 오디오, 07. 자체 시험, 08. GameHost 모듈.
- 그림 원본은 `Tools/Ops/wiki_svg/specs/GameFrameWork` 와 `GameFrameWork_<분야>` 폴더에 있다(17장). 문서 경로는 `pages.js` 가 정한다.
- 문장은 Document Writer 가 다듬는다.

## 시험 · 확인 법

| 무엇 | 명령 | 10-24 결과 |
|---|---|---|
| GameFrameWork 자체 시험 | `JGConsole gfw.selftest`(메인 Bin 은 `MainTree.cmd -name=GameFrameWork exec …`) | 8/8 그룹 · 1,097 검사 · 0 실패 · 2 건너뜀 |
| 고른 그룹 | `gfw.selftest net=session audio` · `-list` | — |
| 옛 이름 | `gmtest` · `gameui.selftest` · `datatable.selftest` · `cookcheck.selftest` · `net.test all` · `audio.selftest` | 예전과 같은 수 |
| 렌더 시험 | `JGConsole -graphics graphics.selftest -exit`(`-exit` 없으면 창이 열린 채 기다림) | 16/16 |
| 2 프로세스 결정론 | `bash Tools/GameFrameWork/net_two_process/run_two_process.sh <Bin> …` | — |
| 소리 · 에셋 확인 게임 | `Tools/Core/audiocheck/README.md`(CreateGameProject + game/ 복사) · `JGLauncher -game -audiocheck -nosound` | 19단계 · 종료 0 |

- 판정은 Error 로그 수가 아니라 실패 수로 한다. 여러 시험이 잘못된 입력을 일부러 넣는다(gfw.selftest 전체에서 Error 19줄).
- 에디터 안에서는 월드 · 네트워크 world 절과 창 없는 소리 검사가 건너뜀으로 센다.

## 함정

- GameMaster · 세션 · GameInstance · GameMasterActor · 컨트롤러를 고치면 `net.test all` 과 2 프로세스(kernel · world)를 꼭 돌린다.
- 메시지 종류 · 필드를 바꾸면 `HGameplayNetProtocol::Version` 을 올린다. 안 올리면 옛 빌드가 조용히 버린다.
- 결정론: 규칙 코드는 상태 + 명령 밖 입력 금지, 부동소수 금지, 컴포넌트 필드는 모두 기본값 초기화.
- 체크섬 값은 상태 칸 · 접속 순서에 따라 바뀐다. 기준은 "모두 같음"이다.
- 엔트리 액터: 규칙 등록은 모든 기계에서, 초기 상태 · StartGame 은 `IsAuthority()` 일 때만.
- PName NAME_NONE 은 "(null)" 로 직렬화된다. 이름 필드의 "없음"은 빈 문자열로 둔다.
- `-host` 는 모든 인터페이스를 연다. 한 기계 시험은 `-bind=127.0.0.1`. 엔진 시험 포트는 47781~ (게임이 47771).
- 에셋 DB 는 Graphics 가 연결된 프로세스에서만 있다. 종료 때 Graphics 보다 먼저 에셋을 놓는다(에디터 · GameHost 는 GameFrameWork 를 먼저 내림).
- ImportAssetFile 은 Graphics 가 있어야 된다(메시 · 텍스처 에셋이 GPU 자원을 만든다). 에셋 DB 는 로드 스레드에서 읽고 메인 스레드 Update 에서 올리므로, 명령 안에서 로드를 기다릴 수 없다 — 자체 시험은 로드 콜백에서 "assetfolder selftest (load)" 를 따로 남기고 임시 폴더(`/JGEngine/__AssetFolderSelfTest`)를 지운다.
- 이미 올라온 경로를 다시 가져오면 파일은 새로 쓰지만 에셋 DB 는 옛 에셋을 그대로 둔다(다시 읽기는 첫 판 밖).
- JGConsole -graphics 의 jg_log 는 끝날 때 한꺼번에 써진다 — 로그를 지켜보며 끝을 기다리는 스크립트는 못 본다.
- 소스 파일을 옮기거나 더하면 PreBuild. 코드 생성기는 옛 생성 파일(`Temp/CodeGen/<모듈>/`)을 지우지 않는다. 클래스를 옮기면 옛 `.generation.*` 를 손으로 지운다.

## 알릴 것 · 열어 둔 것

- Engine-30 은 Editor 의 Engine-31 묶음 검증 뒤 엔진 총괄이 본 폴더에 적용하며 닫는다(19:05 총괄 알림). CLAUDE.md 9절(008cf58): 공개 API 가 바뀌면 같은 작업 안에서 API Reference · 구조도 · 모듈 README · 문서 주석 · 변경 이력. ListAssetFolder · ImportAssetFile 의 Reference 와 문서 주석 보강은 GameFrameWork API Reference(Engine-40) 때 한다 — 지금 헤더 주석에 없는 주의점: 메인 스레드에서만 부른다(에셋 DB 경로 표를 읽음), ImportAssetFile 은 가져오는 동안 부른 스레드를 막는다, 새 에셋의 ClassName 은 다음 프레임 뒤에 채워진다.

- 2단계 검증 때 에디터 4번 중 1번, 시작할 때 D3D12 DebugLayer 경고("Fence values can never be less than zero") 1줄이 났다. 까닭은 조사하지 않았다(Graphics 확인 몫).
- 옛 이어받기 메모(긴 판, 네트워크 인계 원문 포함)는 엔진 총괄 공유 메모 폴더의 `GameFrameWorks_이어받기_2026-10-24.md` 에 있다.
