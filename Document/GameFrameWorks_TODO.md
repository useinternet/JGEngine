# GameFrameWorks 할 일 목록 (순차 진행용)

작성 2026-09-28. 근거는 `Document/GameFrameWorks_구현현황_2026-09-28.md` 와 설계 문서(`게임프레임워크_설계방안_요약`, `설계방안B_상세설계`, `설계방안B_인터페이스_구현가이드`).
위에서부터 순서대로 진행한다. 한 항목이 끝나면 `[x]`, 한 단계가 끝나면 `Document/Memory/` 에 진행 상황을 한 줄 추가한다.
`결정 필요` 표시는 착수 전 사용자 확인.

---

## Phase 0. 1차 구현 빌드 통과

- [x] **0-1. 폴더 · 파일 작성** — 완료 2026-09-28. Core/ GameMaster/ Components/ Actors/ 83개 파일(헤더 44 · cpp 39), JGConsole gmtest 진입.
- [x] **0-2. 코드젠 · 프로젝트 재생성** — 완료 2026-09-28. `Temp/CodeGen/GameFrameWorks/` 11 클래스, `module_system_info.json` 에 GameFrameWorks, JGConsole 이 GameFrameWorks 링크.
- [x] **0-3. GameFrameWorks.vcxproj 빌드 오류 0** — 격리 워크트리(HEAD `fcace8f` + 이 구현)에서 완료 2026-09-28. 첫 빌드 오류 17건(이벤트 한 인자 생성의 most vexing parse 5곳, `GameplayAgent.h` include 누락) 수정. 작업 트리 재확인은 0-6 과 함께.
- [x] **0-4. JGConsole.vcxproj 빌드 오류 0** — 격리 워크트리에서 완료.
- [x] **0-5. `JGConsole.exe gmtest` 종료 코드 0** — 격리 워크트리에서 60/60 통과, 로그 `[error]` 0. 작업 트리 재확인은 0-6 과 함께.
- [x] **0-6. JGLauncher 회귀** — 완료 2026-09-28 17:40. 작업 트리에서 GameFrameWorks · JGConsole 빌드 오류 0, `gmtest` 60/60, 런처 30초 실행 · 종료 코드 0 · 로그 오류 0(`2026-09-28_gfw_launcher_capture.png`, `build_2026-09-28_gameframeworks.log`). 런처는 GameFrameWorks 를 연결하지 않고 코드젠 DLL 로만 로드한다(클래스 목록에 JGActor 등 등장).
- [x] **0-8. 게임 인스턴스 · 엔트리 액터** — 완료 2026-09-28 18:00. 월드 소유를 게임 모듈에서 엔진으로: `Core/GameInstance`(`JGGameInstance`: LoadWorld · UnloadWorld · GetWorld · SetEntryClass · OnWorldLoaded/Unloading, 게임 파생 가능), `Actors/GameEntryActor`(`JGGameEntryActor`: OnEnterWorld/OnExitWorld), 모듈은 생명주기만(`ReplaceGameInstance<T>`). `Core/WorldSelfTest` 15개 헤드리스 검사 추가(`gmtest` 가 함께 실행). GameMasterActor 의 큐 없는 이벤트 소진이 프레임당 1개였던 것을 같은 프레임에 연속 소진으로 수정.
- [x] **0-9. 이름 변경 Simulation → GameMaster · Gameplay** — 완료 2026-09-29 11:55. 규칙: 파사드 `PGameMaster` 와 그것을 든 월드 액터 `JGGameMasterActor` 및 시스템 단위 이름(폴더 `GameMaster/` · 정의 파일 · 자체 테스트 · 로그 카테고리)은 GameMaster, 그 밖의 타입은 Gameplay. 엔티티 액터 `JGGameplayEntityActor`, 콘솔 명령 `gmtest`(옛 이름 `simtest` 도 받음). 타입 62 · 파일 72 이동, 리뷰 재현 코드 함께 치환. 변경 전후 자체 테스트 · 리뷰 재현 결과 줄 단위 동일, 런처 회귀 통과. 대응표 `Document/GameFrameWorks_이름변경안_2026-09-29.md`, 증거 `Document/Memory/2026-09-29_gfw_rename_verify.txt`.
- [ ] **0-7. 커밋** — 사용자가 직접. **순서: 리뷰 기록대로 메모리 트랙 Phase 2(성장형 풀)와 함께 또는 그 뒤** — HEAD 의 고정 풀에서는 스냅샷 스택이 약 95 명령에서 크래시한다(리뷰 R16). 이 트랙의 변경: `Source/Runtime/GameFrameWorks/**`(신규 89개, `Main.cpp` 삭제), `GameFrameWorks.module.json`, `Source/Programs/JGConsole/{Main.cpp,JGConsole.module.json}`, `Document/GameFrameWorks_*`, `Document/Memory/2026-09-28_*`, `Bin/DevelopEngine/GameFrameWorks.{dll,lib,exp}`. **`Source/Runtime/Core/Memory/*` 와 `Devkit/DevScene.cpp` 의 변경은 메모리 트랙(다른 세션)의 것이다.**

## Phase 1. 커널 보강

- [ ] **1-1. `getJGObject` 미등록 타입 처리** — `Core/FileIO/Json.cpp:43` 이 기반 객체를 반환해 `Cast` 실패 → 널 역참조(지난 조사 §5-4-9). 실패를 실패로. Core 변경이므로 메모리 세션과 충돌 확인 후.
- [ ] **1-2. `PGameplayHeadlessRunner`** — 명령 스크립트 파일(`seed` · 명령 줄 · `expect checksum`) 실행. `JGConsole.exe simrun <file> -repeat N`.
- [ ] **1-3. 트리거 정렬 키 확장** — 우선순위 → 소유자(행동자 먼저) → 부여 시각. 지금은 우선순위 → 등록 순.
- [ ] **1-4. 이벤트 인과 묶음 정보** — `HGameplayEvent` 에 같은 효과에서 나온 이벤트 그룹 번호. 시퀀서 병렬 묶음의 근거.
- [ ] **1-5. 상태 스키마 버전 · 마이그레이터 테스트**
- [ ] **1-6. 결정 필요: 체크섬을 JSON 해시 대신 필드 직접 해시로** — 성능이 문제될 때.

## Phase 2. 월드 · 연출

- [ ] **2-1. `JGCameraComponent` · `JGCameraActor`** — 보드 궤도 · 고정 카메라. `PCamera` 빈 클래스와 관계 정리.
- [ ] **2-2. `JGStaticMeshComponent`** — Graphics 핸들 발급 API 필요(`PScene`/`PJGGraphicsAPI`). Graphics 트랙과 조율.
- [ ] **2-3. `JGGameplayDevViewComponent`** — ImGui 상태 인스펙터 · 이벤트 로그 · 되돌리기 · 체크섬. GUI 위젯 경로(`JGWidget`) 로 붙일지 결정 필요.
- [ ] **2-4. GameMasterActor 시퀀서 확장** — 인과 기반 병렬 묶음, 페이싱, 스킵 키.
- [ ] **2-5. 피킹** — `HRay` 대 보드 평면/헥스, 카드 사각형. Controller 에 연결.
- [ ] **2-6. 프리팹(액터 서브트리 에셋 + 델타)** — 지난 조사 §6-3. 다형 직렬화는 이미 동작.

## Phase 3. 게임 레이어 첫 검증

- [ ] **3-1. 게임 모듈 템플릿** — `Sim/` `View/` 폴더, StartupModule 등록, 리플렉션 자동 등록 통과 확인.
- [ ] **3-2. ImGui 수직 슬라이스** — 한 전투를 끝까지. 시퀀서 이벤트 정보량 검증.
- [ ] **3-3. 헤드리스 자동 플레이 통계**

## 백로그

- GameMasterActor 의 액터 파괴를 사망 연출 뒤로 미루는 옵션
- `IGameplayBoard` 시야 규칙 기본 구현(격자 브레젠험)
- 멀티플레이 대비: 명령 스트림 직렬화 규약 문서화
- 툴: 리플렉션 인스펙터로 상태 편집
