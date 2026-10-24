# GameFrameWork 이어받기 메모

갱신: 2026-10-24 18:27 (GameFrameWork 세션)

## 지금

| 항목 | 상태 |
|---|---|
| 맡은 일 | Engine-26 언리얼 에셋 가져오기 — 끝, "검토 중"(엔진 총괄 확인 기다림). 새 일감 Engine-28 가져오기 진입점(할 일, 설계도 먼저) |
| 상태 줄 | `Saved/Feedback/status/gameframework.txt` = "쉬는 중 — Engine-26 검토 중(엔진 총괄)" |
| 잠금 · 빌드 · 시험 | 없음(메인 트리 잠금 18:24 놓음) |
| Engine-26 자리 | 기록 `Document/기록/GameFrameWork/Engine-26_언리얼에셋가져오기_2026-10-24.md`. git 밖: 내보내기 프로젝트 · 스크립트 · 결과 `Temp/UnrealExport/`, 시험 게임 `Temp/Games/UnrealAssetTest`(Content/Unreal 31개, 캡처 Temp/verify/editor_start.png). Epic 라이선스라 저장소 · 엔진 Content 에 넣지 않음 |
| 미커밋 파일 | 없음 — 2단계는 51fb1da, 뒤따른 주석 정리는 710a7af · 2615da6(엔진 총괄 커밋) |

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
- 소스 파일을 옮기거나 더하면 PreBuild. 코드 생성기는 옛 생성 파일(`Temp/CodeGen/<모듈>/`)을 지우지 않는다. 클래스를 옮기면 옛 `.generation.*` 를 손으로 지운다.

## 알릴 것 · 열어 둔 것

- 2단계 검증 때 에디터 4번 중 1번, 시작할 때 D3D12 DebugLayer 경고("Fence values can never be less than zero") 1줄이 났다. 까닭은 조사하지 않았다(Graphics 확인 몫).
- 옛 이어받기 메모(긴 판, 네트워크 인계 원문 포함)는 엔진 총괄 공유 메모 폴더의 `GameFrameWorks_이어받기_2026-10-24.md` 에 있다.
