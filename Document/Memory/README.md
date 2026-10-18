# JGEngine Document/Memory 인덱스

갱신: 2026-09-30. 새 에이전트는 **저장소 맨 위 CLAUDE.md(팀 지침) → 이 문서 → 맡은 분야의 현황.md → TODO.md** 순으로 읽는다.

## 0. 한 줄 요약

엔진 기반 7개 트랙 중 Memory는 완료 처리돼 휴면 상태(2026-09-30)이고, 3개(DevConsole, GameModule(현 DevOps), Server Phase 0~4)는 구현·검증이 끝나 커밋만 남았고, Graphics는 Phase 5 후반(에셋 파이프라인), GameFrameWorks는 Phase 2(월드-렌더 연결) 네 항목 구현·검증(미커밋), GUI는 전용 작업이 멈춘 상태다. 전 트랙의 변경이 한 작업 트리에 미커밋으로 섞여 있어 `git add -A` 한 커밋이 가장 먼저 할 일이다. 자세한 보드는 [진행현황.md](진행현황.md), 사용자 보고서는 [../JGEngine_진행보고_2026-09-30.html](../JGEngine_진행보고_2026-09-30.html).

## 1. 반드시 읽을 것

| 문서 | 내용 |
|---|---|
| [CLAUDE.md](../../CLAUDE.md) | 팀 지침(한 파일): 맨 위 사용자 필수 지침 · 한눈에 보는 약속 표, 엔진 범위 · 문서 · 작업 루프 · 커밋 · 세션 교체 · 보고 · 설계 · 총괄과 대시보드 · 위키. 옛 `지침서.md` 는 안내만 남김 |
| [진행현황.md](진행현황.md) | 분야별 상태·진행률·다음 할 일·사용자 결정 대기·커밋 체크리스트를 한곳에서 관리 |
| 통합 대시보드 엔진 팀 화면 `http://127.0.0.1:47770/#/team/JGEngine` | 사용자용 대시보드(2026-10-02 엔진 대시보드 → 2026-10-17 부터 통합 대시보드, 화면 · 서버는 프로젝트 총괄): 결정할 것 · 진행 중 · 앞으로 · 보류 · 분야별 현황 · 진행 상황(세션 상태) · 필수 지침 · 메모. 데이터 `../Dashboard/data.js` 는 **엔진 총괄만** 고친다. 규칙은 지침서 0절 · 7절, 파일 약속은 `../Dashboard/README.md` |
| `C:/obsidian/ProjectJG/02. 위키/JGEngine/` | 엔진 위키(2026-10-17~): 개요 · 구조도 · 빌드와 실행 · 팀 운영 · 모듈별 장. 엔진 총괄이 고친다 |

## 2. 폴더 구조

```
Document/
  JGEngine_진행보고_2026-09-30.html   사용자 보고서(분야별 흐름, 관리자 관점)
  Server_진행보고_2026-09-30.html     Server(리슨 서버) 트랙 보고서(단계별 진행, 남은 일, 결정 항목)
  GUI_진행보고_2026-09-30.html        GUI(ImGui 모듈) 트랙 보고서(지금 되는 것, 위젯 갱신 경로 결함, 남은 일, 결정 항목)
  GameUI_진행보고_2026-10-01.html     게임 UI(GameFrameWorks JGGameWidget, CommonUI식) 보고서(구조, 사용법, 검증, 한계, 다음 후보)
  GameUI_진행보고_2026-10-02.html     게임 UI 글 서식(ER-010) · 글꼴 아틀라스 페이지 · 대체 글꼴(ER-011) 보고서(쓰는 법, 검증, 알아둘 것)
  GameGUI_진행보고_2026-10-01.html    (이력) 같은 날 새벽의 별도 GameGUI 모듈안 보고서 — 오전에 GFW 로 옮겨 대체됨
  DataTable_설계보고_2026-10-01.html  데이터 테이블(JSON 에셋 + 에디터 엑셀식 편집) 설계 보고(에디터 창 모형, 구조, 결정 6개, 단계) + 10-01 오후 구현 결과(캡처 · 검증 · 사용법)
  아컴호러/                            레퍼런스 게임 자료(요구사항 확인용, 이동하지 않음)
  Memory/
    README.md        이 문서(인덱스)
    진행현황.md      전체 진행 보드
    tools/           공용 도구 3개(crashwalk, net_two_process, capture_devscene.ps1)
    <분야>/          Core · GameFrameWorks · Graphics · DevOps · Server · Etc(GUI · JGEditor · 참고자료)
      현황.md        역할·구현 상태·세션 마지막 상태·함정·검증 방법
      TODO.md        다음 할 일 Top N, 미완료 표, 완료 이력   (Etc 는 TODO_GUI.md · TODO_JGEditor.md · TODO_DevConsole.md(DevConsole 창, JGEditor 담당). GameFrameWorks 는 TODO.md + TODO_GameUI.md + TODO_DataTable.md)
      Files/         설계·분석 문서, 구 TODO(보관), 캡처·로그·결과 txt, Files/tools/ 분야 전용 스크립트
```

## 3. 분야 인덱스

| 분야 | 한 줄 현황 (2026-09-30) | 진행률 | 문서 |
|---|---|---|---|
| GameFrameWorks | 규칙 커널(GameMaster)+월드·액터 계층, 리뷰 버그 15/17 수정. Phase 2 중 2-1 카메라·2-2 정적 메시·2-3 DevView(위젯)·2-5 피킹 시스템 + 월드 뷰 구현·검증(09-30 밤, 커밋 `736ddbe`), `gmtest` 156/156, 게임 프로젝트 에디터에서 실제 클릭 → 엔티티·칸. 두 창은 22:4x C-3으로 JGEditor로 이관(GFW는 순수 로직). 10-01 2-7 GameMaster 흐름 교체(ProjectAH ER-009, `IGameplayFlow` + 기본 흐름, 커밋 `736ddbe`) — `gmtest` 110+64. 10-02 D-19: C26 자체 테스트 저장 파일을 엔진 Temp 로 · 공용 crashwalk 수정(미커밋). 10-02~10-03 D-23 게임 뷰포트 `PGameViewport`(GP-F1·F1b — 에디터 씬 뷰포트·GameHost 가 씀, 미커밋), 10-03 B-4 보드 시야(`TraceLine`·`HasLineOfSight`, 미커밋), GP-F1c 뷰포트 포커스 잃음 → 게임 UI 끌기 취소(미커밋), 저녁 콘솔 명령 안 렌더러 검사 다시 넣기(GP-F1b 후속, 미커밋) — `gmtest` 120+64+31(에디터 안 32, D3D12 0). 남은 2-4·2-6은 선행 조건 대기 | 92% | [현황](GameFrameWorks/현황.md) · [TODO](GameFrameWorks/TODO.md) |
| Core (Core 모듈 — 2026-10-11 `Memory/` 에서 이름 변경. 아래는 메모리 시스템) | **완료 (휴면, 2026-09-30).** 페이지 성장형 풀 재설계, 상주 메모리 428→230MB, 약참조 UAF 제거(4-1) 모두 커밋(`4c8ac73`까지). 남은 항목은 보류 확정, 구체적 문제가 생기면 재개. 09-30 밤 사용자 요청으로 통계 창 재디자인(UI-1) 완료 · 커밋됨(`736ddbe`). **10-11 오디오 재생(D-37): 새 런타임 모듈 `Source/Runtime/Audio`(miniaudio 0.11.25) 완료 · 메인 반영, 미커밋** — 자체 검사 `audio.selftest`, 검증 게임 실제 출력 판정 9/9(패키지 포함) | 100% (오디오 AU-1~4 완료) | [현황](Core/현황.md) · [TODO](Core/TODO.md) · [오디오 TODO](Core/TODO_Audio.md) |
| Graphics | 빌드 복구→첫 드로우→메시 드로우→전송/파이프라이닝→PScene 분리까지 완료. 남은 것은 에셋 저장 크기(5-30, 결정 필요)·머터리얼·색 공간·도구 명령 | 80% | [현황](Graphics/현황.md) · [TODO](Graphics/TODO.md) |
| DevOps (구 GameModule, 2026-10-02 이름 변경) | 빌드 · 툴 · 프로젝트 환경. 엔진-게임 분리 완료 · 커밋(`4c8ac73` · `736ddbe`): 외부 GameProject 생성·솔루션·빌드·JGEditor 로드, 게임 Content `/JGGame/`(R7), 헤드리스 모듈 검사 `JGConsole module.test`(R8), 같은 GUID 에셋 보호, 한글 프로젝트 경로 거부, 엔진 `Game` 더미 모듈 삭제(R4, 솔루션 14 프로젝트). 게임 단독 실행 호스트·R5/B10은 보류. **10-02 빌드 속도 검토(BS-0) 완료** — 전체 빌드 272.6초의 원인은 프로젝트 안 직렬 컴파일 + 파일마다 `Core.h` 묶음 파싱, **10-02 11:50 적용(D-20 "권고대로 진행")**: `/MP` + PCH 서드파티로 전체 다시 빌드 272.6 → 66.0초 · GFW 헤더 수정 219.8 → 22.3초 · Core 헤더 수정 356.1 → 40.2초, 미커밋([보고서](../DevOps_빌드속도_검토_2026-10-02.html)). C-1 게임 템플릿 의존(`Asset` · `Graphics`) 완료(미커밋) | 엔진-게임 분리 100% | [현황](DevOps/현황.md) · [TODO](DevOps/TODO.md) |
| Server | 리슨 서버 Phase 0~4(전송·세션·명령 중계·체크섬·재동기·재접속) 커밋(`4c8ac73`), Phase 5(월드 연결: 게임 인스턴스가 세션 소유 · GameMasterActor 세션 경유 · 컨트롤러 소유 검사 · 실행 인자 `-host`/`-join`) 커밋(`736ddbe`). **5-5 최소 로비(10-02, 미커밋)**: JGEditor 개발용 창 `Windows/Session Lobby` — Host · Join · Leave · Reload World(새 판) · 슬롯 표, 런처 두 개를 창으로만 조작해 판 완주 확인. 10-03 패키지 네트워크 확인(GP-S1) 완료. **10-12 대기방 기능 묶음(D-38, 사용자 (다)) 완료 · 메인 반영(미커밋)**: 세션 공유 데이터(방 정보, ProjectAH ER-025) · 판 시작 전 나간 사람 자리 비우기 · 준비 표시(ER-006), 프로토콜 버전 2(옛 패키지와 같이 못 함). 10-12 저녁 같은 자리로 재접속(ER-026 — 설정 `ReconnectToken` · `GetReconnectToken`, 창을 다시 띄워도 판 중 같은 자리) 완료 · 메인 반영(미커밋). 10-17 규칙 효과 안 상태 되돌리기 같이 하기 시험(D-39 — `net.test rewind` 26, 구현은 GFW) 완료 · 메인 반영(미커밋). 남은 것 5-5 · D-38 · ER-026 · D-39 시험 커밋 | 100% | [현황](Server/현황.md) · [TODO](Server/TODO.md) |
| Etc / DevConsole 창 (2026-10-07~ JGEditor 담당, 콘솔 명령 시스템은 Core) | 콘솔 명령 시스템 Phase 0~4 완료 · 커밋(`4c8ac73`). 사용자 요청으로 09-30 밤 로그 필터 · 명령 미리보기, 10-01 오전 UI 보강(메모리 통계 창과 같은 팔레트 · 레벨 칩 · 로그 줄 강조) · 카테고리 체크박스 드롭다운(All) 추가 — 커밋 `736ddbe` · `efba16a`. 10-02 5-1(D-7 (a)): 프로젝트 없이 띄운 에디터도 GameFrameWorks 를 연결해 콘솔에서 `gmtest` 등 GFW 명령을 쓴다 — 검증 완료, 커밋 대기 | 98% | [현황](Etc/현황.md) · [TODO](Etc/TODO_DevConsole.md) |
| Etc / GUI | 09-30 저녁 1-2 위젯 갱신 훅을 매 프레임 `OnUpdate` 하나로 합침(사용자 결정) + P1 버그 5건(1-3·1-4·1-7·1-8·1-9) 구현·검증(미커밋). 남은 것 1-5 메뉴 경로 키, 1-10 나머지, 1-11 커밋, 3-5 `imgui.ini` 결정 | 32% | [현황](Etc/현황.md) · [TODO](Etc/TODO_GUI.md) |
| Etc / JGEditor | 신규 트랙(09-30 밤): 에디터 호스트와 게임 월드를 보는 에디터 창. GFW C-3 이관 완료(미커밋) — 씬 뷰포트 `JGSceneViewport`(옛 GFW `JGWorldView`) · `JGGameplayDevView`, 메뉴 `Windows/Scene Viewport` · `Windows/Gameplay DevView`, 프로젝트 모드에서 씬 뷰포트 자동 열기. 검증 게임 에디터에서 클릭 → 엔티티·칸 이관 전과 같은 값. 10-01 에디터 레이아웃 복원(열린 창 저장 · 재시작 때 다시 열기, GUI BL-4) 완료(미커밋) | E-1 · E-2 완료 | [현황](Etc/현황.md) §5 · [TODO](Etc/TODO_JGEditor.md) |
| GameFrameWorks / 게임 UI | 10-01(D-12, 오전 사용자 방향 수정): **게임 UI 는 GFW 안 `UI/`** — 화면 `JGGameWidget`(JGCLASS, CommonUI식 활성 · 비활성 · 입력 모드 · 뒤로가기), 레이어 스택 관리자 `PGameUIManager`(`JGGameInstance::GetUI()`), 이미지 · 글자 · 버튼(스타일 · 비활성) · 한글 글꼴, Graphics 2D 그리기 경로, JGEditor Scene Viewport 호스트(포인터 · 키보드). 10-01 낮 ProjectAH 요청 반영: **자동 줄바꿈(ER-007, GG-8)** · **글자 입력란 · IME 확정 글자 · 클립보드(ER-008, GG-9)**. 검증 게임 에디터 실제 입력 20단계(메뉴 · 차단 · Esc · 주소 · 한글 이름 · 입력 중 Esc), 회귀 통과. 커밋 `736ddbe`(10-01 12:16, 사용자), 실제 한글 IME 조합 확인(GG-9U)은 사용자 건너뜀(10-17). 10-02 낮 ProjectAH 요청 반영(사용자 "권고 대로 진행", D-17 · D-18): **글 서식(ER-010, GG-10 — 문단 서식 · 양쪽 맞춤 · 글 안 마크업 구간 서식 · 합성 굵게/기울임 · 장평)** · **글꼴 아틀라스 페이지 · LRU 비우기 · 글꼴 대체(ER-011, GG-11)**, `gameui.selftest` 243/243, 메인 반영 · 미커밋(GG-C2). 같은 날 새벽의 별도 `GameGUI` 모듈안은 대체됨 | GG-0~11 완료 | [현황](GameFrameWorks/현황.md) §10 · [TODO](GameFrameWorks/TODO_GameUI.md) · [설계](GameFrameWorks/Files/GameUI_설계_2026-10-01.md) |
| GameFrameWorks / 데이터 테이블 (2026-10-07~ GFW 담당, 문서 2026-10-11 이동) | 신규 트랙(10-01): 행(키) × 열(타입) 데이터를 `.jgasset`(JSON) 에셋 `JGDataTable` 로 두고 에디터 "Data Table Editor"(메뉴 `Windows/Data Table Editor`)에서 엑셀처럼 편집. **10-01 오후 DT-0~4 구현 · 검증 · 메인 트리 반영, 커밋 대기(DT-C1)**. 결정 D-16: 코드는 GameFrameWorks `Data/`, 스키마는 에셋 안 + C++ 바인딩 검사(`HDataTableView<T>`), 열 7타입(struct 는 나중에 리플렉션 — DT-H1), 에디터 한글 글꼴 포함. 메인 빌드 exit 0, `datatable.selftest` 127/127, 회귀 5종 통과 | 95% | [현황](GameFrameWorks/현황.md) §11 · [TODO](GameFrameWorks/TODO_DataTable.md) · [설계 · 결과 · 사용 안내](GameFrameWorks/Files/DataTable_설계_2026-10-01.md) · [증적](GameFrameWorks/Files/2026-10-01_datatable_verify.txt) |
| Etc / 참고자료 | `Document/아컴호러/` 247파일(2016 코어셋). 요구사항 확인용이며 엔진 설계 기준이 아님. 카드 이미지 산출물은 대부분 저장소 외 | - | [현황](Etc/현황.md) |

진행률 산정 근거는 각 분야 현황.md 3절과 [진행현황.md](진행현황.md)에 있다.

## 4. 공용 도구 (`Document/Memory/tools/`)

| 도구 | 용도 | 쓰는 분야 |
|---|---|---|
| `crashwalk/` (crashwalk.exe, build.bat) | 실행 파일을 띄우고 종료 코드·로그를 stdout으로 받는 검증 러너. 동시 세션이 `jg_log.txt`를 덮어쓰므로 증적은 이 stdout으로 남긴다. 사용: `crashwalk.exe "<exe> <인자>" <작업 폴더> <닫기 초> [멈춤 초=30]`. 2026-10-02 판부터 제한 시간을 매 루프 확인해 D3D12 디버그 레이어 아래에서도 제때 WM_CLOSE 를 보낸다(그 전 판은 닫지 못했다 — `GameFrameWorks/Files/2026-10-02_D-19_crashwalk_C26_verify.txt`). 다시 빌드는 `build.bat`(VS 2022 Community) | 전체 |
| `net_two_process/run_two_process.sh` | 호스트·클라이언트 2프로세스 결정론 검사 | Server, GameFrameWorks |
| `capture_devscene.ps1` | 런처 메인 창 PrintWindow 캡처. 저장소 위치는 스크립트 자리 기준(2026-10-11 옛 `C:\JG\JGEngine` 경로 고침). 사용: `Build\BatchFiles\MainTree.cmd -name=<세션> exec Document\Memory\tools\capture_devscene.ps1 -WaitSeconds 30 -WorkDir <JGDevFeature=1 imgui.ini 를 둔 폴더> -OutPath <png>`(기본 저장 `Temp\Capture\`, 런처 · 작업 폴더가 없으면 종료 1) | Graphics, GUI, JGEditor, DevOps |

분야 전용 스크립트는 각 `<분야>/Files/tools/`에 있다.

## 5. 데스크톱 세션 ↔ 분야 대응

2026-10-07 사용자 결정으로 세션을 통합했다: Data · Memory → **`Core`**(Core 모듈 담당), DevConsole → **`JGEditor`**(에디터에 들어가는 요소 담당), 데이터 테이블 → **`GameFrameWorks`**(코드가 GFW `Data/` 에 있음). 문서 폴더는 2026-10-11 정리했다(6절 이동 매핑).

| 세션(메시지 이름) | 맡는 것 | 문서 |
|---|---|---|
| `Core` (10-07 전 "Data", Memory 세션을 합침) | **Core 모듈 전체** — 메모리 시스템, 콘솔 명령 시스템(`Core/ConsoleCommand`), 플랫폼 · 파일 · Delegate 등 Core 공용, **오디오 모듈 `Audio`(10-11~, D-37)**. 다른 세션이 Core 를 고치면 Core 세션에 알린다 | Core/(TODO 첫 표 = Core 공용 · 콘솔 명령 시스템, 아래는 메모리 시스템. 오디오는 `TODO_Audio.md`) |
| `GameFrameWorks` | GameFrameWorks 모듈 — **데이터 테이블(`Data/`, 10-07 Data 세션에서 넘어옴)** · Asset `Cook/` 쿠킹 콘텐츠 검사 포함. 게임 UI(`UI/`)는 GUI 세션이 만든다 | GameFrameWorks/(TODO_DataTable.md · 현황 §11 = 데이터 테이블) |
| `GUI` | GUI(ImGui) 모듈, 게임 UI(GFW `UI/`) 구현 | Etc/TODO_GUI.md · 현황 §2, GameFrameWorks/TODO_GameUI.md · 현황 §10 |
| `JGEditor` | **에디터에 들어가는 요소** — 에디터 호스트 · 씬 뷰포트 · DevView · Data Table Editor 창 · Session Lobby 창 · **DevConsole 창(`Source/Editor/DevConsole`, 10-07 DevConsole 세션에서 넘어옴)** | Etc/TODO_JGEditor.md · 현황 §5, Etc/TODO_DevConsole.md(창 · UI 항목) |
| `Graphics` | Graphics 모듈 | Graphics/ |
| `DevOps` (10-02 전 "게임 모듈") | 빌드 시스템 · 툴 · 게임 프로젝트 환경 · 빌드 성능. JGConsole 프로그램(쿠킹 명령 · `-game` 옵션) 포함 | DevOps/ |
| `Server` | 네트워크(리슨 서버 · 세션 · 로비) | Server/ |
| `엔진 총괄` | 전체(2026-10-02~): 작업 배정 · 대시보드 데이터 · 대시보드 명령 전달 · CLAUDE.md 맨 위(필수 지침) 동기화. 각 세션은 맡은 작업이 끝나면 여기로 보고(지침서 7절) | 진행현황.md · Dashboard/ |

Etc 참고자료(레퍼런스 게임 정리)는 맡은 세션이 없다.

세션 메시지 이름(2026-10-07 기준): `엔진 총괄` · `Core` · `GameFrameWorks` · `GUI` · `JGEditor` · `Graphics` · `DevOps` · `Server`.

세션 id(2026-10-17 사용자가 세션들을 정리한 뒤 엔진 총괄이 다시 만듦 — 꺼진 세션도 이 `local_…` id 로 세션 메시지를 보내면 깨어난다): 엔진 총괄 `local_ddfc91e6-7f3c-405e-b413-9c2df90f1fff` · Core `local_118e3ca5-185c-46e0-aa68-8b53eb888fbe` · GameFrameWorks `local_3d78be20-97c2-48f9-a240-4c5a415bf40b` · GUI `local_60f8bccb-e42d-42f2-8527-d959ab561908` · JGEditor `local_b2ec9ff2-47e5-4bbf-b68b-eea449623a19` · Graphics `local_52411b8a-d25c-433f-b663-9c10c3f0daf5` · DevOps `local_43f3bbc6-4e82-4983-a6fd-d36c1a3f66e1` · Server `local_823fb100-b91a-4f99-8a89-62be77d347c7`.

게임 쪽(ProjectAH 저장소, 2026-10-03 사용자 결정으로 나뉨): 엔진이 게임 쪽에 맡길 일 · 알림은 **`게임 총괄`**(`local_16022a8f-ba74-4396-b515-7f2116554e40`)에 보낸다. 게임 총괄은 사용자 피드백 나누기 · 진행 · ER 문서 · 엔진과 조율을 맡는다.
- `게임 로직 구현`(10-02 까지 "게임 구현"): 규칙 · 데이터 · 빌드 · 패키징. D-23 GP-P1 결과는 엔진 총괄에 직접 보고한다.
- `게임 연출 구현`: 게임 화면 · 연출. 게임 UI 같은 화면 쪽 쓰는 법(예: ER-014 · ER-015 = D-24 · D-25)은 여기로 바로 보내도 된다.
- 그 밖의 ProjectAH 세션도 게임 총괄을 거친다.
- 게임 쪽 진행은 엔진 문서에 넣지 않는다. 엔진에 필요한 것은 ER 요청서(`ProjectAH/Document/엔진요청/`)로 받는다.

**프로젝트 총괄**(2026-10-17~, 사용자가 두 팀 위에 둠, 세션 `local_425a16ba-de7d-4b3c-b31c-7b37415515ca`, 폴더 `C:/obsidian/ProjectJG`): 통합 대시보드 화면 · 서버, 프로젝트 전체 필수 지침(`C:/obsidian/ProjectJG/01. 운영/전체 필수지침.md`), 세션 구성(`01. 운영/세션 구성.md`). 전달 사항은 대시보드 inbox 명령(`from` = "프로젝트 총괄 (통합 대시보드)")이나 세션 메시지로 온다.

## 6. 이동 매핑 (2026-09-30 · 2026-10-11 재편 전 → 후)

보관 문서(Files/ 안의 옛 문서) · 증적 txt · `Document/*.html` 보고서 내부의 옛 경로 링크는 갱신하지 않았다. 아래 표로 해석한다.

| 옛 위치 | 새 위치 |
|---|---|
| `Document/<분야>_TODO.md` (Graphics/Memory/GameFrameWorks/Network/DevConsole/GUI) | `<분야>/Files/` 에 보관. 살아있는 TODO는 `<분야>/TODO.md` (Network → Server, DevConsole·GUI → Etc/TODO_*.md) |
| `Document/<분야 분석·설계 문서>.md` | `<분야>/Files/` |
| `Document/Memory/2026-*.md`, `*.txt`, `*.png`, `build_*.log` | 소유 분야의 `Files/` (09-16 빌드 로그 3개는 Graphics) |
| `Document/Memory/tools/<분야 전용 도구>` | `<분야>/Files/tools/` |
| `Document/Memory/tools/{crashwalk, net_two_process, capture_devscene.ps1}` | 그대로(공용) |
| `Document/아컴호러/` | 그대로 |
| `Document/Memory/GameModule/` (분야 GameModule, 2026-10-02 이름 변경) | `Document/Memory/DevOps/` — 안의 파일 이름 · 항목 ID(R*/B*/H*/P*/GM-*)는 그대로. 옛 기록 · 소스 주석의 "GameModule R8" 같은 표기는 DevOps 트랙 항목으로 읽는다 |
| `Document/Memory/Memory/` (분야 Memory, 2026-10-11) | `Document/Memory/Core/` — Core 모듈 담당 Core 세션의 문서(2026-10-07 세션 통합). 안의 파일 이름 · 항목 ID 는 그대로. TODO 첫 표 = Core 공용 · 콘솔 명령 시스템 |
| `Etc/TODO_DataTable.md` · `Etc/현황.md` §7 · `Etc/Files/` 의 데이터 테이블 파일 13개 + `Etc/Files/tools/datatable_*` 2개 (2026-10-11) | `GameFrameWorks/TODO_DataTable.md` · `GameFrameWorks/현황.md` §11 · `GameFrameWorks/Files/`(tools 포함). 항목 ID(DT-*)는 그대로 |
| `Etc/TODO_DevConsole.md` · `Etc/현황.md` §1 (2026-10-11) | 그대로 — DevConsole 창은 JGEditor 담당. 콘솔 명령 항목(보류 B-1)만 `Core/TODO.md` 로 |
| `Document/Memory/지침서.md` (2026-10-18 지움) | 저장소 맨 위 `CLAUDE.md`. 옛 기록의 “지침서 N절” → CLAUDE.md: 0 · 1절 → 맨 위 · 1, 2절 → 2(5번 → 4, 6번 → 5), 3절 → 3, 4절 → 6, 5절 → 7, 7절 → 8 |

## 7. 유지 규칙 (요약, 상세는 CLAUDE.md 2 · 4절)

1. 세션 종료·중단 전 `<분야>/현황.md`의 "세션 마지막 상태", `TODO.md`, `진행현황.md`를 갱신한다.
2. TODO 항목 식별자(Phase 5-26, R5, N-1 등)는 바꾸지 않는다.
3. 새 증적은 `<분야>/Files/`, 공용 스크립트는 `tools/`에 둔다.
4. 게임 고유 항목은 삭제하지 않고 TODO의 "게임 프로젝트 영역(엔진 범위 외)" 표로 옮긴다.
5. 커밋 · 푸시는 엔진 총괄이 묶어서 한다. 커밋에 함께 들어가야 할 파일은 TODO의 커밋 체크리스트에 적는다.
