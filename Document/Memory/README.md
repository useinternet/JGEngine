# JGEngine Document/Memory 인덱스

갱신: 2026-09-30. 새 에이전트는 **이 문서 → 지침서.md → 맡은 분야의 현황.md → TODO.md** 순으로 읽는다.

## 0. 한 줄 요약

엔진 기반 7개 트랙 중 Memory는 완료 처리돼 휴면 상태(2026-09-30)이고, 3개(DevConsole, GameModule, Server Phase 0~4)는 구현·검증이 끝나 커밋만 남았고, Graphics는 Phase 5 후반(에셋 파이프라인), GameFrameWorks는 Phase 2(월드-렌더 연결) 네 항목 구현·검증(미커밋), GUI는 전용 작업이 멈춘 상태다. 전 트랙의 변경이 한 작업 트리에 미커밋으로 섞여 있어 `git add -A` 한 커밋이 가장 먼저 할 일이다. 자세한 보드는 [진행현황.md](진행현황.md), 사용자 보고서는 [../JGEngine_진행보고_2026-09-30.html](../JGEngine_진행보고_2026-09-30.html).

## 1. 반드시 읽을 것

| 문서 | 내용 |
|---|---|
| [지침서.md](지침서.md) | 최우선 원칙: 우리는 게임 엔진을 만든다. 엔진 기반에 집중하고, 레퍼런스 게임(아컴호러 등) 구현에 초점을 두지 않는다. 작업 규칙·문서 규칙 포함 |
| [진행현황.md](진행현황.md) | 분야별 상태·진행률·다음 할 일·사용자 결정 대기·커밋 체크리스트를 한곳에서 관리 |

## 2. 폴더 구조

```
Document/
  JGEngine_진행보고_2026-09-30.html   사용자 보고서(분야별 흐름, 관리자 관점)
  Server_진행보고_2026-09-30.html     Server(리슨 서버) 트랙 보고서(단계별 진행, 남은 일, 결정 항목)
  GUI_진행보고_2026-09-30.html        GUI(ImGui 모듈) 트랙 보고서(지금 되는 것, 위젯 갱신 경로 결함, 남은 일, 결정 항목)
  GameUI_진행보고_2026-10-01.html     게임 UI(GameFrameWorks JGGameWidget, CommonUI식) 보고서(구조, 사용법, 검증, 한계, 다음 후보)
  GameGUI_진행보고_2026-10-01.html    (이력) 같은 날 새벽의 별도 GameGUI 모듈안 보고서 — 오전에 GFW 로 옮겨 대체됨
  DataTable_설계보고_2026-10-01.html  데이터 테이블(JSON 에셋 + 에디터 엑셀식 편집) 설계 보고(에디터 창 모형, 구조, 결정 6개, 단계)
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
| GameFrameWorks | 규칙 커널(GameMaster)+월드·액터 계층, 리뷰 버그 15/17 수정. Phase 2 중 2-1 카메라·2-2 정적 메시·2-3 DevView(위젯)·2-5 피킹 시스템 + 월드 뷰 구현·검증(09-30 밤, 미커밋), `gmtest` 156/156, 게임 프로젝트 에디터에서 실제 클릭 → 엔티티·칸. 두 창은 22:4x C-3으로 JGEditor로 이관(GFW는 순수 로직). 10-01 2-7 GameMaster 흐름 교체(ProjectAH ER-009, `IGameplayFlow` + 기본 흐름, 미커밋) — `gmtest` 110+64. 남은 2-4·2-6은 선행 조건 대기 | 92% | [현황](GameFrameWorks/현황.md) · [TODO](GameFrameWorks/TODO.md) |
| Memory | **완료 (휴면, 2026-09-30).** 페이지 성장형 풀 재설계, 상주 메모리 428→230MB, 약참조 UAF 제거(4-1) 모두 커밋(`4c8ac73`까지). 남은 항목은 보류 확정, 구체적 문제가 생기면 재개. 09-30 밤 사용자 요청으로 통계 창 재디자인(UI-1) 완료 · 커밋 대기 | 100% | [현황](Memory/현황.md) · [TODO](Memory/TODO.md) |
| Graphics | 빌드 복구→첫 드로우→메시 드로우→전송/파이프라이닝→PScene 분리까지 완료. 남은 것은 에셋 저장 크기(5-30, 결정 필요)·머터리얼·색 공간·도구 명령 | 80% | [현황](Graphics/현황.md) · [TODO](Graphics/TODO.md) |
| GameModule | **완료 (커밋 대기 → 휴면, 2026-09-30).** 외부 GameProject 생성·솔루션·빌드·JGEditor 로드(`4c8ac73`), 게임 Content `/JGGame/`(R7), 헤드리스 모듈 검사 `JGConsole module.test`(R8), 같은 GUID 에셋 보호, 한글 프로젝트 경로 거부까지 구현·검증. 09-30 저녁분은 커밋 대기(GM-C2). 09-30 밤 사용자 결정으로 **R4 엔진 `Game` 더미 모듈 삭제** — 메인 트리 적용 · 검증 완료(미커밋, 솔루션 14 프로젝트, `Game` 프로젝트 이름은 예약). 게임 단독 실행 호스트·R5/B10은 보류 | 100% | [현황](GameModule/현황.md) · [TODO](GameModule/TODO.md) |
| Server | 리슨 서버 Phase 0~4(전송·세션·명령 중계·체크섬·재동기·재접속) 커밋(`4c8ac73`). **Phase 5(월드 연결) 구현·검증(09-30 밤, 미커밋)**: 게임 인스턴스가 세션 소유 · GameMasterActor 세션 경유 · 컨트롤러 소유 검사 · 실행 인자 `-host`/`-join`, `net.test` 101/101 · 2프로세스 kernel·world OK · 런처 2개 결투 완주. 남은 것 5-5 최소 로비(결정 대기)·커밋 | 97% | [현황](Server/현황.md) · [TODO](Server/TODO.md) |
| Etc / DevConsole | 콘솔 명령 시스템 Phase 0~4 완료 · 커밋(`4c8ac73`). 사용자 요청으로 09-30 밤 로그 필터 · 명령 미리보기, 10-01 오전 UI 보강(메모리 통계 창과 같은 팔레트 · 레벨 칩 · 로그 줄 강조) · 카테고리 체크박스 드롭다운(All) 추가, 검증 · 메인 트리 빌드 완료(`console.selftest` 64/64), 커밋 대기 | 97% | [현황](Etc/현황.md) · [TODO](Etc/TODO_DevConsole.md) |
| Etc / GUI | 09-30 저녁 1-2 위젯 갱신 훅을 매 프레임 `OnUpdate` 하나로 합침(사용자 결정) + P1 버그 5건(1-3·1-4·1-7·1-8·1-9) 구현·검증(미커밋). 남은 것 1-5 메뉴 경로 키, 1-10 나머지, 1-11 커밋, 3-5 `imgui.ini` 결정 | 32% | [현황](Etc/현황.md) · [TODO](Etc/TODO_GUI.md) |
| Etc / JGEditor | 신규 트랙(09-30 밤): 에디터 호스트와 게임 월드를 보는 에디터 창. GFW C-3 이관 완료(미커밋) — 씬 뷰포트 `JGSceneViewport`(옛 GFW `JGWorldView`) · `JGGameplayDevView`, 메뉴 `Windows/Scene Viewport` · `Windows/Gameplay DevView`, 프로젝트 모드에서 씬 뷰포트 자동 열기. 검증 게임 에디터에서 클릭 → 엔티티·칸 이관 전과 같은 값. 10-01 에디터 레이아웃 복원(열린 창 저장 · 재시작 때 다시 열기, GUI BL-4) 완료(미커밋) | E-1 · E-2 완료 | [현황](Etc/현황.md) §5 · [TODO](Etc/TODO_JGEditor.md) |
| GameFrameWorks / 게임 UI | 10-01(D-12, 오전 사용자 방향 수정): **게임 UI 는 GFW 안 `UI/`** — 화면 `JGGameWidget`(JGCLASS, CommonUI식 활성 · 비활성 · 입력 모드 · 뒤로가기), 레이어 스택 관리자 `PGameUIManager`(`JGGameInstance::GetUI()`), 이미지 · 글자 · 버튼(스타일 · 비활성) · 한글 글꼴, Graphics 2D 그리기 경로, JGEditor Scene Viewport 호스트(포인터 · 키보드). 10-01 낮 ProjectAH 요청 반영: **자동 줄바꿈(ER-007, GG-8)** · **글자 입력란 · IME 확정 글자 · 클립보드(ER-008, GG-9)**. `gameui.selftest` 193/193, 검증 게임 에디터 실제 입력 20단계(메뉴 · 차단 · Esc · 주소 · 한글 이름 · 입력 중 Esc), 회귀 통과. 커밋 대기(GG-C1), 실제 한글 IME 조합은 사용자 확인(GG-9U). 같은 날 새벽의 별도 `GameGUI` 모듈안은 대체됨 | GG-0~9 완료 | [현황](GameFrameWorks/현황.md) §10 · [TODO](GameFrameWorks/TODO_GameUI.md) · [설계](GameFrameWorks/Files/GameUI_설계_2026-10-01.md) |
| Etc / DataTable | 신규 트랙(10-01): 행(키) × 열(타입) 데이터를 `.jgasset`(JSON) 에셋 `JGDataTable` 로 두고 에디터 "Data Table Editor" 에서 엑셀처럼 편집. **설계 보고 완료, 코드 0, 결정 6개 대기(진행현황 D-16)**. 권고: 스키마는 에셋 안 + C++ 바인딩 검사, Asset 모듈, 에디터 한글 글꼴 포함 | 설계 완료 | [현황](Etc/현황.md) §7 · [TODO](Etc/TODO_DataTable.md) · [설계](Etc/Files/DataTable_설계_2026-10-01.md) |
| Etc / 참고자료 | `Document/아컴호러/` 247파일(2016 코어셋). 요구사항 확인용이며 엔진 설계 기준이 아님. 카드 이미지 산출물은 대부분 저장소 외 | - | [현황](Etc/현황.md) |

진행률 산정 근거는 각 분야 현황.md 3절과 [진행현황.md](진행현황.md)에 있다.

## 4. 공용 도구 (`Document/Memory/tools/`)

| 도구 | 용도 | 쓰는 분야 |
|---|---|---|
| `crashwalk/` (crashwalk.exe, build.bat) | 실행 파일을 띄우고 종료 코드·로그를 stdout으로 받는 검증 러너. 동시 세션이 `jg_log.txt`를 덮어쓰므로 증적은 이 stdout으로 남긴다. **2026-10-01 현재 제한 시간에 창을 닫지 못한다**(D3D12 디버그 레이어의 스레드 이름 예외가 끊이지 않음 — `진행현황.md` 6절 리스크, 고치는 방법 `GameFrameWorks/Files/2026-10-01_ER-009_verify.txt` 끝) | 전체 |
| `net_two_process/run_two_process.sh` | 호스트·클라이언트 2프로세스 결정론 검사 | Server, GameFrameWorks |
| `capture_devscene.ps1` | 런처 메인 창 PrintWindow 캡처 | Graphics, GUI, DevConsole, GameModule |

분야 전용 스크립트는 각 `<분야>/Files/tools/`에 있다.

## 5. 데스크톱 세션 ↔ 분야 대응

| 세션 제목 | 분야 폴더 |
|---|---|
| 그래픽스 시스템 | Graphics |
| 메모리 시스템 | Memory |
| 게임 프레임 워크, GameFramework 설계 및 코드 리뷰 | GameFrameWorks |
| 게임 모듈 | GameModule |
| 서버 시스템 | Server |
| DevConsole | Etc (TODO_DevConsole.md) |
| GUI 시스템 | Etc (TODO_GUI.md). 10-01 게임 UI(GameFrameWorks `UI/`, `GameFrameWorks/TODO_GameUI.md` · 현황 §10)도 이 세션이 만들었다 |
| JGEditor | Etc (TODO_JGEditor.md, 현황 §5) |
| 데이터 테이블(데이터 관련 기반 작업) | Etc (TODO_DataTable.md, 현황 §7) |
| 레퍼런스 게임 정리, 카드게임 플레이어 카드 뒷면 이미지 | Etc (참고자료) |

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

## 7. 유지 규칙 (요약, 상세는 지침서 2절)

1. 세션 종료·중단 전 `<분야>/현황.md`의 "세션 마지막 상태", `TODO.md`, `진행현황.md`를 갱신한다.
2. TODO 항목 식별자(Phase 5-26, R5, N-1 등)는 바꾸지 않는다.
3. 새 증적은 `<분야>/Files/`, 공용 스크립트는 `tools/`에 둔다.
4. 게임 고유 항목은 삭제하지 않고 TODO의 "게임 프로젝트 영역(엔진 범위 외)" 표로 옮긴다.
5. 커밋은 사용자가 한다. 커밋에 함께 들어가야 할 파일은 TODO의 커밋 체크리스트에 적는다.
