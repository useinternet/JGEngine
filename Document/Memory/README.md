# JGEngine Document/Memory 인덱스

갱신: 2026-09-30. 새 에이전트는 **이 문서 → 지침서.md → 맡은 분야의 현황.md → TODO.md** 순으로 읽는다.

## 0. 한 줄 요약

엔진 기반 7개 트랙 중 Memory는 완료 처리돼 휴면 상태(2026-09-30)이고, 3개(DevConsole, GameModule(현 DevOps), Server Phase 0~4)는 구현·검증이 끝나 커밋만 남았고, Graphics는 Phase 5 후반(에셋 파이프라인), GameFrameWorks는 Phase 2(월드-렌더 연결) 네 항목 구현·검증(미커밋), GUI는 전용 작업이 멈춘 상태다. 전 트랙의 변경이 한 작업 트리에 미커밋으로 섞여 있어 `git add -A` 한 커밋이 가장 먼저 할 일이다. 자세한 보드는 [진행현황.md](진행현황.md), 사용자 보고서는 [../JGEngine_진행보고_2026-09-30.html](../JGEngine_진행보고_2026-09-30.html).

## 1. 반드시 읽을 것

| 문서 | 내용 |
|---|---|
| [지침서.md](지침서.md) | 최우선 원칙: 우리는 게임 엔진을 만든다. 엔진 기반에 집중하고, 레퍼런스 게임(아컴호러 등) 구현에 초점을 두지 않는다. 작업 규칙·문서 규칙 포함 |
| [진행현황.md](진행현황.md) | 분야별 상태·진행률·다음 할 일·사용자 결정 대기·커밋 체크리스트를 한곳에서 관리 |
| [../JGEngine_대시보드.html](../JGEngine_대시보드.html) | 사용자용 대시보드(2026-10-02): 결정할 것 · 진행 중 · 앞으로 · 보류 · 분야별 현황 · 필수 지침 · 메모. 데이터 `../Dashboard/data.js` 는 **총괄 세션만** 고친다. 규칙은 지침서 0절 · 7절, 형식은 `../Dashboard/README.md` |

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
    지침서.md        작업 지침
    진행현황.md      전체 진행 보드
    tools/           공용 도구 3개(crashwalk, net_two_process, capture_devscene.ps1)
    <분야>/
      현황.md        역할·구현 상태·세션 마지막 상태·함정·검증 방법
      TODO.md        다음 할 일 Top N, 미완료 표, 완료 이력   (Etc는 TODO_DevConsole.md, TODO_GUI.md, TODO_JGEditor.md, TODO_DataTable.md. GameFrameWorks 는 TODO.md + TODO_GameUI.md)
      Files/         설계·분석 문서, 구 TODO(보관), 캡처·로그·결과 txt, Files/tools/ 분야 전용 스크립트
```

## 3. 분야 인덱스

| 분야 | 한 줄 현황 (2026-09-30) | 진행률 | 문서 |
|---|---|---|---|
| GameFrameWorks | 규칙 커널(GameMaster)+월드·액터 계층, 리뷰 버그 15/17 수정. Phase 2 중 2-1 카메라·2-2 정적 메시·2-3 DevView(위젯)·2-5 피킹 시스템 + 월드 뷰 구현·검증(09-30 밤, 커밋 `736ddbe`), `gmtest` 156/156, 게임 프로젝트 에디터에서 실제 클릭 → 엔티티·칸. 두 창은 22:4x C-3으로 JGEditor로 이관(GFW는 순수 로직). 10-01 2-7 GameMaster 흐름 교체(ProjectAH ER-009, `IGameplayFlow` + 기본 흐름, 커밋 `736ddbe`) — `gmtest` 110+64. 10-02 D-19: C26 자체 테스트 저장 파일을 엔진 Temp 로 · 공용 crashwalk 수정(미커밋). 남은 2-4·2-6은 선행 조건 대기 | 92% | [현황](GameFrameWorks/현황.md) · [TODO](GameFrameWorks/TODO.md) |
| Memory | **완료 (휴면, 2026-09-30).** 페이지 성장형 풀 재설계, 상주 메모리 428→230MB, 약참조 UAF 제거(4-1) 모두 커밋(`4c8ac73`까지). 남은 항목은 보류 확정, 구체적 문제가 생기면 재개. 09-30 밤 사용자 요청으로 통계 창 재디자인(UI-1) 완료 · 커밋 대기 | 100% | [현황](Memory/현황.md) · [TODO](Memory/TODO.md) |
| Graphics | 빌드 복구→첫 드로우→메시 드로우→전송/파이프라이닝→PScene 분리까지 완료. 남은 것은 에셋 저장 크기(5-30, 결정 필요)·머터리얼·색 공간·도구 명령 | 80% | [현황](Graphics/현황.md) · [TODO](Graphics/TODO.md) |
| DevOps (구 GameModule, 2026-10-02 이름 변경) | 빌드 · 툴 · 프로젝트 환경. 엔진-게임 분리 완료 · 커밋(`4c8ac73` · `736ddbe`): 외부 GameProject 생성·솔루션·빌드·JGEditor 로드, 게임 Content `/JGGame/`(R7), 헤드리스 모듈 검사 `JGConsole module.test`(R8), 같은 GUID 에셋 보호, 한글 프로젝트 경로 거부, 엔진 `Game` 더미 모듈 삭제(R4, 솔루션 14 프로젝트). 게임 단독 실행 호스트·R5/B10은 보류. **10-02 빌드 속도 검토(BS-0) 완료** — 전체 빌드 272.6초의 원인은 프로젝트 안 직렬 컴파일 + 파일마다 `Core.h` 묶음 파싱, **10-02 11:50 적용(D-20 "권고대로 진행")**: `/MP` + PCH 서드파티로 전체 다시 빌드 272.6 → 66.0초 · GFW 헤더 수정 219.8 → 22.3초 · Core 헤더 수정 356.1 → 40.2초, 미커밋([보고서](../DevOps_빌드속도_검토_2026-10-02.html)). C-1 게임 템플릿 의존(`Asset` · `Graphics`) 완료(미커밋) | 엔진-게임 분리 100% | [현황](DevOps/현황.md) · [TODO](DevOps/TODO.md) |
| Server | 리슨 서버 Phase 0~4(전송·세션·명령 중계·체크섬·재동기·재접속) 커밋(`4c8ac73`), Phase 5(월드 연결: 게임 인스턴스가 세션 소유 · GameMasterActor 세션 경유 · 컨트롤러 소유 검사 · 실행 인자 `-host`/`-join`) 커밋(`736ddbe`). **5-5 최소 로비(10-02, 미커밋)**: JGEditor 개발용 창 `Windows/Session Lobby` — Host · Join · Leave · Reload World(새 판) · 슬롯 표, 런처 두 개를 창으로만 조작해 판 완주 확인. 남은 것 5-5 커밋. ProjectAH ER-006 은 사용자 결정 전 | 100% | [현황](Server/현황.md) · [TODO](Server/TODO.md) |
| Etc / DevConsole | 콘솔 명령 시스템 Phase 0~4 완료 · 커밋(`4c8ac73`). 사용자 요청으로 09-30 밤 로그 필터 · 명령 미리보기, 10-01 오전 UI 보강(메모리 통계 창과 같은 팔레트 · 레벨 칩 · 로그 줄 강조) · 카테고리 체크박스 드롭다운(All) 추가 — 커밋 `736ddbe` · `efba16a`. 10-02 5-1(D-7 (a)): 프로젝트 없이 띄운 에디터도 GameFrameWorks 를 연결해 콘솔에서 `gmtest` 등 GFW 명령을 쓴다 — 검증 완료, 커밋 대기 | 98% | [현황](Etc/현황.md) · [TODO](Etc/TODO_DevConsole.md) |
| Etc / GUI | 09-30 저녁 1-2 위젯 갱신 훅을 매 프레임 `OnUpdate` 하나로 합침(사용자 결정) + P1 버그 5건(1-3·1-4·1-7·1-8·1-9) 구현·검증(미커밋). 남은 것 1-5 메뉴 경로 키, 1-10 나머지, 1-11 커밋, 3-5 `imgui.ini` 결정 | 32% | [현황](Etc/현황.md) · [TODO](Etc/TODO_GUI.md) |
| Etc / JGEditor | 신규 트랙(09-30 밤): 에디터 호스트와 게임 월드를 보는 에디터 창. GFW C-3 이관 완료(미커밋) — 씬 뷰포트 `JGSceneViewport`(옛 GFW `JGWorldView`) · `JGGameplayDevView`, 메뉴 `Windows/Scene Viewport` · `Windows/Gameplay DevView`, 프로젝트 모드에서 씬 뷰포트 자동 열기. 검증 게임 에디터에서 클릭 → 엔티티·칸 이관 전과 같은 값. 10-01 에디터 레이아웃 복원(열린 창 저장 · 재시작 때 다시 열기, GUI BL-4) 완료(미커밋) | E-1 · E-2 완료 | [현황](Etc/현황.md) §5 · [TODO](Etc/TODO_JGEditor.md) |
| GameFrameWorks / 게임 UI | 10-01(D-12, 오전 사용자 방향 수정): **게임 UI 는 GFW 안 `UI/`** — 화면 `JGGameWidget`(JGCLASS, CommonUI식 활성 · 비활성 · 입력 모드 · 뒤로가기), 레이어 스택 관리자 `PGameUIManager`(`JGGameInstance::GetUI()`), 이미지 · 글자 · 버튼(스타일 · 비활성) · 한글 글꼴, Graphics 2D 그리기 경로, JGEditor Scene Viewport 호스트(포인터 · 키보드). 10-01 낮 ProjectAH 요청 반영: **자동 줄바꿈(ER-007, GG-8)** · **글자 입력란 · IME 확정 글자 · 클립보드(ER-008, GG-9)**. 검증 게임 에디터 실제 입력 20단계(메뉴 · 차단 · Esc · 주소 · 한글 이름 · 입력 중 Esc), 회귀 통과. 커밋 `736ddbe`(10-01 12:16, 사용자), 실제 한글 IME 조합은 사용자 확인(GG-9U). 10-02 낮 ProjectAH 요청 반영(사용자 "권고 대로 진행", D-17 · D-18): **글 서식(ER-010, GG-10 — 문단 서식 · 양쪽 맞춤 · 글 안 마크업 구간 서식 · 합성 굵게/기울임 · 장평)** · **글꼴 아틀라스 페이지 · LRU 비우기 · 글꼴 대체(ER-011, GG-11)**, `gameui.selftest` 243/243, 메인 반영 · 미커밋(GG-C2). 같은 날 새벽의 별도 `GameGUI` 모듈안은 대체됨 | GG-0~11 완료 | [현황](GameFrameWorks/현황.md) §10 · [TODO](GameFrameWorks/TODO_GameUI.md) · [설계](GameFrameWorks/Files/GameUI_설계_2026-10-01.md) |
| Etc / DataTable | 신규 트랙(10-01): 행(키) × 열(타입) 데이터를 `.jgasset`(JSON) 에셋 `JGDataTable` 로 두고 에디터 "Data Table Editor"(메뉴 `Windows/Data Table Editor`)에서 엑셀처럼 편집. **10-01 오후 DT-0~4 구현 · 검증 · 메인 트리 반영, 커밋 대기(DT-C1)**. 결정 D-16: 코드는 GameFrameWorks `Data/`, 스키마는 에셋 안 + C++ 바인딩 검사(`HDataTableView<T>`), 열 7타입(struct 는 나중에 리플렉션 — DT-H1), 에디터 한글 글꼴 포함. 메인 빌드 exit 0, `datatable.selftest` 127/127, 회귀 5종 통과 | 95% | [현황](Etc/현황.md) §7 · [TODO](Etc/TODO_DataTable.md) · [설계 · 결과 · 사용 안내](Etc/Files/DataTable_설계_2026-10-01.md) · [증적](Etc/Files/2026-10-01_datatable_verify.txt) |
| Etc / 참고자료 | `Document/아컴호러/` 247파일(2016 코어셋). 요구사항 확인용이며 엔진 설계 기준이 아님. 카드 이미지 산출물은 대부분 저장소 외 | - | [현황](Etc/현황.md) |

진행률 산정 근거는 각 분야 현황.md 3절과 [진행현황.md](진행현황.md)에 있다.

## 4. 공용 도구 (`Document/Memory/tools/`)

| 도구 | 용도 | 쓰는 분야 |
|---|---|---|
| `crashwalk/` (crashwalk.exe, build.bat) | 실행 파일을 띄우고 종료 코드·로그를 stdout으로 받는 검증 러너. 동시 세션이 `jg_log.txt`를 덮어쓰므로 증적은 이 stdout으로 남긴다. 사용: `crashwalk.exe "<exe> <인자>" <작업 폴더> <닫기 초> [멈춤 초=30]`. 2026-10-02 판부터 제한 시간을 매 루프 확인해 D3D12 디버그 레이어 아래에서도 제때 WM_CLOSE 를 보낸다(그 전 판은 닫지 못했다 — `GameFrameWorks/Files/2026-10-02_D-19_crashwalk_C26_verify.txt`). 다시 빌드는 `build.bat`(VS 2022 Community) | 전체 |
| `net_two_process/run_two_process.sh` | 호스트·클라이언트 2프로세스 결정론 검사 | Server, GameFrameWorks |
| `capture_devscene.ps1` | 런처 메인 창 PrintWindow 캡처 | Graphics, GUI, DevConsole, DevOps |

분야 전용 스크립트는 각 `<분야>/Files/tools/`에 있다.

## 5. 데스크톱 세션 ↔ 분야 대응

| 세션 제목 | 분야 폴더 |
|---|---|
| 그래픽스 시스템 | Graphics |
| 메모리 시스템 | Memory |
| 게임 프레임 워크, GameFramework 설계 및 코드 리뷰 | GameFrameWorks |
| DevOps (10-02 전 제목 "게임 모듈") | DevOps |
| 서버 시스템 | Server |
| DevConsole | Etc (TODO_DevConsole.md) |
| GUI 시스템 | Etc (TODO_GUI.md). 10-01 게임 UI(GameFrameWorks `UI/`, `GameFrameWorks/TODO_GameUI.md` · 현황 §10)도 이 세션이 만들었다 |
| JGEditor | Etc (TODO_JGEditor.md, 현황 §5) |
| 데이터 테이블(데이터 관련 기반 작업) | Etc (TODO_DataTable.md, 현황 §7) |
| 레퍼런스 게임 정리, 카드게임 플레이어 카드 뒷면 이미지 | Etc (참고자료) |
| 엔진 총괄 | 전체(2026-10-02~): 작업 배정 · 대시보드 데이터 · 대시보드 명령 전달 · 지침서 0절(필수 지침) 동기화. 각 세션은 맡은 작업이 끝나면 여기로 보고(지침서 7절) |

세션 메시지 이름(2026-10-02 기준): `엔진 총괄` · `GameFrameWorks` · `GUI` · `Server` · `DevConsole` · `DevOps` · `Graphics` · `JGEditor` · `Data` · `Memory`.

## 6. 이동 매핑 (2026-09-30 재편 전 → 후)

보관 문서(Files/ 안의 옛 문서) 내부의 옛 경로 링크는 갱신하지 않았다. 아래 표로 해석한다.

| 옛 위치 | 새 위치 |
|---|---|
| `Document/<분야>_TODO.md` (Graphics/Memory/GameFrameWorks/Network/DevConsole/GUI) | `<분야>/Files/` 에 보관. 살아있는 TODO는 `<분야>/TODO.md` (Network → Server, DevConsole·GUI → Etc/TODO_*.md) |
| `Document/<분야 분석·설계 문서>.md` | `<분야>/Files/` |
| `Document/Memory/2026-*.md`, `*.txt`, `*.png`, `build_*.log` | 소유 분야의 `Files/` (09-16 빌드 로그 3개는 Graphics) |
| `Document/Memory/tools/<분야 전용 도구>` | `<분야>/Files/tools/` |
| `Document/Memory/tools/{crashwalk, net_two_process, capture_devscene.ps1}` | 그대로(공용) |
| `Document/아컴호러/` | 그대로 |
| `Document/Memory/GameModule/` (분야 GameModule, 2026-10-02 이름 변경) | `Document/Memory/DevOps/` — 안의 파일 이름 · 항목 ID(R*/B*/H*/P*/GM-*)는 그대로. 옛 기록 · 소스 주석의 "GameModule R8" 같은 표기는 DevOps 트랙 항목으로 읽는다 |

## 7. 유지 규칙 (요약, 상세는 지침서 2절)

1. 세션 종료·중단 전 `<분야>/현황.md`의 "세션 마지막 상태", `TODO.md`, `진행현황.md`를 갱신한다.
2. TODO 항목 식별자(Phase 5-26, R5, N-1 등)는 바꾸지 않는다.
3. 새 증적은 `<분야>/Files/`, 공용 스크립트는 `tools/`에 둔다.
4. 게임 고유 항목은 삭제하지 않고 TODO의 "게임 프로젝트 영역(엔진 범위 외)" 표로 옮긴다.
5. 커밋은 사용자가 한다. 커밋에 함께 들어가야 할 파일은 TODO의 커밋 체크리스트에 적는다.
