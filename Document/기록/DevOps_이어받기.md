# DevOps 이어받기 메모

갱신: 2026-10-24 19:01 (DevOps 세션)

## 지금

| 항목 | 상태 |
|---|---|
| 맡은 일 | 끝낸 둘 — Engine-35 게임이 쓰는 엔진 API 목록(Core Engine-34 에 넘김), Engine-49 위키 05. Programs 재검토(엔진 총괄에 넘김). 둘 다 읽기만 했다 |
| 단계 | 둘 다 "검토 중"(엔진 총괄 확인) |
| 다음 할 것 | 엔진 총괄이 시작을 알리면 Engine-43(JGLauncher 가 `RequestExit` 종료 코드를 버림 — `JGLauncher/Main.cpp:43` 이 늘 0) · Engine-18(커밋 전 검사기 — 새 사용자 메모 자리 `Saved/Feedback/memo.md`) · Engine-19(새 클론에서 `.sh` 줄바꿈 확인 — `core.autocrlf=true` · `.gitattributes` 없음). 모두 고치기 전에 설계도부터(CLAUDE.md 7절). Engine-49 수정안(05 를 설계 설명 + Programs Reference + 하우투 3개로 나눔)이 정해지면 그 문서들도 DevOps 몫일 수 있다 |
| 기다리는 상대 | 엔진 총괄(Engine-35 · 49 확인, Engine-43 · 18 · 19 시작 차례) |
| 상태 줄 | `Saved/Feedback/status/devops.txt` = "쉬는 중 — 맡은 일 없음 (Engine-35 · Engine-49 검토 중)" |
| 잠금 · 빌드 · 시험 | 없음(메인 트리 잠금을 쥐지 않았다) |
| 미커밋 파일 | `Document/기록/DevOps/Engine-35_게임이쓰는엔진API_2026-10-24.md` · `Engine-35_api_usage.py` · `Engine-35_gen_doc.py` · `Engine-49_05Programs_재검토_2026-10-24.md`, 이 메모. 커밋은 엔진 총괄 몫 |
| 정리 거리 | `Temp/Games/NoAICheck` — AI 제거 확인용 검증 게임. 엔진 위치가 이제 없는 작업 폴더 `Temp/wt/devops-ai` 를 가리킨다(AI 제거는 51fb1da 로 메인에 들어감). 지울지는 엔진 총괄이 정한다 |

## Engine-35 결과 요약

- 기록: `Document/기록/DevOps/Engine-35_게임이쓰는엔진API_2026-10-24.md` — 1절 한눈에 · 상속 확장점, 2절 쓰지만 내부용으로 보이는 것, 3절 안 쓰지만 공개할 것 같은 것, 4절 센 방법과 한계, 5절 모듈별 표.
- 대상: ProjectAH 231 파일 · 게임 프로젝트 틀 8 · 검증 게임 4개(AudioCheck · GameUIDemo · GfwView · NetDuel), 메인 c6dd99d 기준.
- 요점: 게임은 `<이름>Defines.h` 가 넣는 `Core.h` 로 Core 헤더 거의 전부를 받는다(엔진 PCH 에는 `Core.h` 가 없다 — 19:04 Core 지적으로 바로잡음). 쓰임은 Core 와 GameFrameWork 가 거의 전부이고, Graphics 는 `IRawTexture` 하나, GUI 는 메뉴 등록뿐, GameHost · JGEditor 는 0.
- 다시 만들기: `python -X utf8 -I Document/기록/DevOps/Engine-35_api_usage.py C:/Develop/JGEngine <json>` → `python -X utf8 -I Document/기록/DevOps/Engine-35_gen_doc.py <json> <md>`. Windows python 에는 `C:/` 꼴 경로를 준다(`/c/...` 는 못 읽음).

## Engine-49 결과 요약

- 기록: `Document/기록/DevOps/Engine-49_05Programs_재검토_2026-10-24.md`(기준 86eb96e) — 결론 표, 1절 코드와 어긋난 곳 6, 2절 게임 프로젝트 눈높이 4, 3절 API Reference 로 가는 길 3, 4절 절차 섞임과 나눌 안, 5절 그림 9장(모두 코드와 맞음).
- 요점: 구조도 `JGEngine.md:114 · 135` 가 "절차는 05. Programs 에서" 라고 가리키는데 05 에는 따라 할 절차가 없다. 안 — 05 는 설계 설명으로, 명령줄 · 설명 파일은 「Programs Reference」로, 절차는 하우투 3개(게임 프로젝트 만들고 실행 · 게임 패키지 · 엔진 처음 빌드)로, MainTree · Tools 절은 팀 운영 문서로(약 1.5일).

## 담당

- `Source/Programs`(JGBuildTool · JGHeaderTool · JGConsole(명령 · 쿠킹 · `-game` · `-graphics`) · JGLauncher · JGGameLaunch · JGGameBootstrap), 빌드(`Build/` 배치 · premake · `MainTree.cmd` · `RunEditor.cmd` · `Source/PCH` · `Source/Dummy` · `Source/ThirdParty`), 게임 프로젝트 만들기 · 쿠킹 · 패키지, `Tools/`(공용 · 분야별 검증 도구 자리).
- GameHost 모듈은 GameFrameWork 몫(H-061). DevOps 는 패키지 쪽 — `cook.run` · `JGGameLaunch` · `JGLauncher -game` · `BuildTool.cpp` 의 GameHost 뿌리 규칙.
- 위키 `C:/obsidian/ProjectJG/02. 위키/JGEngine/05. Programs.md` 와 그림 원본 `Tools/Ops/wiki_svg/specs/Programs/`.

## 아직 문서에 넣지 않은 사용자 피드백

- 없음.
