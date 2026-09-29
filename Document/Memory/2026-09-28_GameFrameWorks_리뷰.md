# 2026-09-28 작업 기록 — GameFrameWorks 설계 · 코드 리뷰

다른 AI 에이전트나 다음 세션이 이어서 작업할 수 있도록 현재 상황과 이번 작업 내용을 기록한다.
구현 트랙의 기록은 `2026-09-28_턴제게임_프레임워크_방향성.md`, 할 일은 `Document/GameFrameWorks_TODO.md` 다.

## 요청
"최근에 GameFrameWork 작업이 있었어. 전체적으로 설계 리뷰, 코드리뷰 부탁할게"

## 결과
- 보고서 **`Document/GameFrameWorks_설계코드리뷰_2026-09-28.md`** — 결론 · 재현 결과 표(R1–R17) · 설계 문제 D1–D12 · 코드 발견 C1–C27 · 수정 순서.
- 재현 코드 **`Document/Memory/tools/gfw_review_repro/ReviewRepro.cpp`**, 결과 원문 **`Document/Memory/2026-09-28_gfw_review_repro_results.txt`**.
- **소스 변경 없음.** 메인 트리에서 빌드 · 실행하지 않았다(다른 세션이 GameFrameWorks 를 수정 · 빌드하던 중). 커밋은 사용자가 직접.

## 핵심 (자세한 내용은 보고서)
- 설계 방향(방안 B)과 결정론 기본기는 좋다. 결함은 실행 모델 두 곳: 페이즈 전이가 효과 큐 밖에서 동기 진행(D1), 트리거가 Emit 안에서 즉시 반응 · 권한 과다(D2).
- 재현한 버그 17건. 커밋 전 필수: 죽은 ID 가 살아 있는 엔티티 데이터를 덮음(C1), 스냅샷 벡터(C3 — HEAD Core 의 고정 풀에서 약 95 명령에 크래시), 월드 좀비(C4), `GetWorldPosition`(C8).
- **커밋 순서 주의:** GameFrameWorks 는 메모리 트랙 Phase 2(성장형 풀)와 함께 또는 그 뒤에 커밋해야 한다. TODO 0-7 은 따로 커밋하라고 적혀 있다.
- 리뷰 도중(18:36~18:40) 구현 세션이 `Core/GameInstance` · `Actors/GameEntryActor` · `Core/WorldSelfTest` 를 추가하고 모듈을 게임 인스턴스 소유로 바꿨다. 리뷰는 20:32 기준 90개 파일(메인 트리와 동일 확인)을 대상으로 했다.

## 검증 절차 (다시 할 때)
1. `git worktree add --detach <scratchpad>/wt HEAD`
2. 메인 트리의 `Source/Runtime/GameFrameWorks/` 전체를 워크트리에 덮어쓰기(워크트리의 `Main.cpp` 는 지워진다), `Source/Programs/JGConsole/{Main.cpp,JGConsole.module.json}` 복사, `ReviewRepro.cpp` 를 `Source/Programs/JGConsole/` 에 복사하고 `Main.cpp` 에 `simreview` · `simreviewperf` · `simreview256` 분기 추가(보고서 §5).
3. `<wt>/Build/BatchFiles` 에서 `../../Bin/DevelopEngine/JGHeaderTool.exe` → `JGBuildTool.exe` 를 2회(둘 다 종료 시 segfault 139 는 정상).
4. `MSBuild Temp\ProjectFiles\GameFrameWorks.vcxproj` 다음 `JGConsole.vcxproj`, 옵션 `-p:Configuration=DevelopEngine -p:Platform=x64 -p:SolutionDir=<wt 윈도 경로>\ -p:BuildProjectReferences=false`. Git Bash 에서는 `MSYS_NO_PATHCONV=1` 을 붙인다. 두 프로젝트 합쳐 약 2분.
5. `<wt>/Bin/DevelopEngine` 에서 `JGConsole.exe simtest`(기준선 75/75) → `simreview` → `simreviewperf` → `simreview256`.
6. 크래시 스택은 `Document/Memory/tools/crashwalk/crashwalk.exe "<exe> <인자>" <작업 폴더> <초>` — 첫 인자가 명령줄 전체라 인자를 함께 넘길 수 있다.

## 이번에 확인한 사실 (코드에 적혀 있지 않은 것)
- JGConsole 은 `GCoreSystem::Update` 를 부르지 않는다(런처만 부름). 헤드리스 실행 동안 GC · 스케줄러가 한 번도 돌지 않아 GC 객체가 종료 때까지 쌓인다. 한 프로세스에서 시나리오를 여러 개 돌리면 앞 시나리오의 객체가 풀을 채운다 → 헤드리스 러너는 반복마다 `GMemoryGlobalSystem::Flush()` 필요.
- HEAD Core 의 풀에서 `HSimulationState` 한 개는 컨테이너 약 20개(디버그 프록시 16B 각 1개)를 가진다. 스냅샷 벡터 재할당 때 두 벌이 동시에 살아 16B 클래스(4096 블록)가 약 95 명령에서 바닥난다.
- `JGClass::GetChildClasses` 순서는 `ChildTypeSet`(HHashSet) 순회 순서다. 등록 순서에 의미를 두는 코드(큐 프로토타입)는 정렬이 필요하다.
- `JGSimulationCue::CreateInstance` 는 `GetClass()` 기반이라 `JG_GENERATED_CLASS_BODY` 없는 파생은 기본 클래스로 만들어진다.
- 이 환경의 `python` · `python3` 는 Windows Store 스텁이다(실행 안 됨). 스크립트는 perl 을 쓴다. Bash 도구가 명령 안의 `\\` 를 줄이는 일이 있어, 백슬래시가 들어간 perl 정규식은 파일로 쓰고 `\x5c` 로 적는다.

## 다음 작업자에게
- 수정은 보고서 §4 순서를 따른다. 설계 변경(D1 페이즈 큐화, D2 트리거 지연 반응, D3 트리거 인스턴스)은 `결정 필요` 로 사용자 확인 후 착수할 것. TODO 에 반영하지 않았다(구현 세션이 관리 중) — 사용자가 원하면 `GameFrameWorks_TODO.md` 에 항목을 추가한다.
- 수정 후 재현 코드의 R 항목이 전부 `[NOT REPRODUCED]` 가 되는지 확인하고, 가능하면 R 항목을 `simtest` 회귀 테스트로 옮긴다.
- 검증에 쓴 워크트리는 작업 후 `git worktree remove --force` 로 지웠다. 다시 돌릴 때는 위 절차로 새로 만든다.

## 진행 기록
- 2026-09-28 20:40 리뷰 · 재현 · 보고서 작성 완료. 소스 변경 없음.
- 2026-09-29 (구현 세션이 추가) 이름 변경 반영: 코드의 `Simulation` 이름이 GameMaster · Gameplay 로 바뀌었다(대응표 `Document/GameFrameWorks_이름변경안_2026-09-29.md`). `Document/Memory/tools/gfw_review_repro/ReviewRepro.cpp` 도 같은 표로 치환했고, 격리 워크트리에서 변경 전후 `simreview` 결과가 줄 단위로 같음을 확인했다(`2026-09-29_gfw_rename_verify.txt`). 위 절차의 `simtest` 는 `gmtest` 가 되었고 옛 이름도 받는다. 재현 코드 연결은 이제 `#include "GameMaster/GameMaster.h"` 기준이다.
