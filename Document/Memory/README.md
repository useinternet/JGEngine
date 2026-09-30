# JGEngine Document/Memory 인덱스

갱신: 2026-09-30. 새 에이전트는 **이 문서 → 지침서.md → 맡은 분야의 현황.md → TODO.md** 순으로 읽는다.

## 0. 한 줄 요약

엔진 기반 7개 트랙 중 4개(Memory, DevConsole, GameModule, Server Phase 0~4)는 구현·검증이 끝나 커밋만 남았고, Graphics는 Phase 5 후반(에셋 파이프라인), GameFrameWorks는 Phase 2(월드-렌더 연결) 착수 대기, GUI는 전용 작업이 멈춘 상태다. 전 트랙의 변경이 한 작업 트리에 미커밋으로 섞여 있어 `git add -A` 한 커밋이 가장 먼저 할 일이다. 자세한 보드는 [진행현황.md](진행현황.md), 사용자 보고서는 [../JGEngine_진행보고_2026-09-30.html](../JGEngine_진행보고_2026-09-30.html).

## 1. 반드시 읽을 것

| 문서 | 내용 |
|---|---|
| [지침서.md](지침서.md) | 최우선 원칙: 우리는 게임 엔진을 만든다. 엔진 기반에 집중하고, 레퍼런스 게임(아컴호러 등) 구현에 초점을 두지 않는다. 작업 규칙·문서 규칙 포함 |
| [진행현황.md](진행현황.md) | 분야별 상태·진행률·다음 할 일·사용자 결정 대기·커밋 체크리스트를 한곳에서 관리 |

## 2. 폴더 구조

```
Document/
  JGEngine_진행보고_2026-09-30.html   사용자 보고서(분야별 흐름, 관리자 관점)
  아컴호러/                            레퍼런스 게임 자료(요구사항 확인용, 이동하지 않음)
  Memory/
    README.md        이 문서(인덱스)
    지침서.md        작업 지침
    진행현황.md      전체 진행 보드
    tools/           공용 도구 3개(crashwalk, net_two_process, capture_devscene.ps1)
    <분야>/
      현황.md        역할·구현 상태·세션 마지막 상태·함정·검증 방법
      TODO.md        다음 할 일 Top N, 미완료 표, 완료 이력   (Etc는 TODO_DevConsole.md, TODO_GUI.md)
      Files/         설계·분석 문서, 구 TODO(보관), 캡처·로그·결과 txt, Files/tools/ 분야 전용 스크립트
```

## 3. 분야 인덱스

| 분야 | 한 줄 현황 (2026-09-30) | 진행률 | 문서 |
|---|---|---|---|
| GameFrameWorks | 규칙 커널(GameMaster)+월드·액터 계층 1차 구현, 리뷰 버그 15/17 수정, `gmtest` 111/111. Phase 2(카메라·메시 컴포넌트·DevView·피킹) 착수 대기 | 70% | [현황](GameFrameWorks/현황.md) · [TODO](GameFrameWorks/TODO.md) |
| Memory | 페이지 성장형 풀 재설계 완료, 상주 메모리 428→230MB, 약참조 UAF 제거(4-1). 4-1 커밋과 Phase 4 종결 확정만 남음 → 휴면 예정 | 95% | [현황](Memory/현황.md) · [TODO](Memory/TODO.md) |
| Graphics | 빌드 복구→첫 드로우→메시 드로우→전송/파이프라이닝→PScene 분리까지 완료. 남은 것은 에셋 저장 크기(5-30, 결정 필요)·머터리얼·색 공간·도구 명령 | 80% | [현황](Graphics/현황.md) · [TODO](Graphics/TODO.md) |
| GameModule | 외부 GameProject 생성·솔루션 생성·빌드·JGEditor 로드까지 E2E 검증 완료, 메인 트리 반영. 커밋(exe 2개·템플릿 필수)과 선택 항목(R5/R7/R8)·게임 실행 호스트 결정 남음 | 85% | [현황](GameModule/현황.md) · [TODO](GameModule/TODO.md) |
| Server | 리슨 서버 Phase 0~4(전송·세션·명령 중계·체크섬·재동기·재접속) 구현·검증, `net.test` 66/66, 2프로세스 결정론 OK. Phase 5(월드 연결) 착수 여부 결정 대기 | 75% | [현황](Server/현황.md) · [TODO](Server/TODO.md) |
| Etc / DevConsole | 콘솔 명령 시스템 Phase 0~4 완료(`console.selftest` 57/57), 런처 창·JGConsole 양쪽 실행. 커밋만 남음 | 95% | [현황](Etc/현황.md) · [TODO](Etc/TODO_DevConsole.md) |
| Etc / GUI | 32항목 중 5건만 완료(모두 타 트랙이 처리). 09-21 이후 전용 세션 없음, 1-2 플래그 결정 대기 | 20% | [현황](Etc/현황.md) · [TODO](Etc/TODO_GUI.md) |
| Etc / 참고자료 | `Document/아컴호러/` 247파일(2016 코어셋). 요구사항 확인용이며 엔진 설계 기준이 아님. 카드 이미지 산출물은 대부분 저장소 외 | - | [현황](Etc/현황.md) |

진행률 산정 근거는 각 분야 현황.md 3절과 [진행현황.md](진행현황.md)에 있다.

## 4. 공용 도구 (`Document/Memory/tools/`)

| 도구 | 용도 | 쓰는 분야 |
|---|---|---|
| `crashwalk/` (crashwalk.exe, build.bat) | 실행 파일을 띄우고 종료 코드·로그를 stdout으로 받는 검증 러너. 동시 세션이 `jg_log.txt`를 덮어쓰므로 증적은 이 stdout으로 남긴다 | 전체 |
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
| GUI 시스템 | Etc (TODO_GUI.md) |
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
