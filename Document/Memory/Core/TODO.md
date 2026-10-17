# Core TODO (Core 모듈 — 메모리 시스템 · 콘솔 명령 시스템 · Core 공용)

갱신 2026-10-11(Core 이벤트 구독 버그 완료 · 대리자 보류 3건 추가 — 첫 표. UI-1c 가 `736ddbe` 에 커밋된 것을 git 로 확인해 완료 이력으로. 14시 오디오 재생(D-37) 새 Audio 모듈 완료 — 상세는 `TODO_Audio.md`). 그 밖의 메모리 시스템 부분은 2026-09-30 그대로. `Files/Memory_TODO.md`(2026-09-28 작성, Phase 0~7)를 코드 · git 기준으로 최신화해 통합한 목록. 항목 ID 는 구 TODO 와 같다(다른 문서가 참조). 현황은 `현황.md`.
원칙: **지금 구체적 문제가 있는 항목만 미완료로 둔다.** "나중에 필요할지도" 성격은 보류 표에 두고 재검토 트리거를 적는다. `결정 필요` 항목은 착수 전에 사용자 확인.
검증 루프와 합격 기준은 `현황.md` §6. 커밋 · 푸시는 **엔진 총괄이 묶어서** 한다(2026-10-17 사용자 결정 H-001 — 그 전에는 사용자). Core 세션은 커밋하지 않고, 일이 끝나면 아래 체크리스트에 파일을 적고 엔진 총괄에 알린다.


## Core 공용 · 콘솔 명령 시스템 (2026-10-07~ Core 세션)

2026-10-07 세션 통합으로 이 문서는 Core 모듈 전체의 TODO 다. 아래 "다음에 할 일" 부터는 메모리 시스템(옛 Memory 분야) 내용 그대로다. 2026-10-11 폴더 이름 `Memory/` → `Core/`.

| 항목 | 상태 | 비고 |
|---|---|---|
| 델리게이트 AddLambda 가 왼값을 참조로 저장(ER-023) — 이름 있는 람다 · `std::function` 변수를 `AddLambda` · `CreateLambda` · `BindLambda` 로 걸면 참조로 저장돼, 원래 변수가 사라진 뒤 부르면 매달린 참조로 죽음 | **완료 2026-10-11 14:31 메인 반영, 미커밋** — 게임 쪽 결함 보고, 엔진 총괄 배정(사용자 결정 없이) | `CreateLambda` 가 `std::decay_t` 값 형으로 저장(왼값 복사 · 오른값 이동), `PLambdaDelegate` 생성자 복사 · 이동 둘, `operator+=` 의 `std::move` → `std::forward`(왼값을 옮겨 가 비우던 것). 부르는 쪽 모양 그대로, 게임 UI 움직임 `Call` 도 같이 사본. `delegate.selftest` 람다 저장 9 검사 추가 → 34/34(고치기 전 7 실패 + abort), 회귀 · 에디터 종료 0 · live 0. 참조 저장에 기대던 사용처 없음(엔진 · ProjectAH). 복사할 수 없는 람다를 왼값으로 넘기면 이제 컴파일 오류(사용처 없음). 증적 `Files/2026-10-11_ER-023_lambda_storage_verify.txt`, 커밋 목록은 표 아래(이벤트 구독 버그와 같은 두 파일) |
| Core 이벤트 구독이 객체를 지워도 안 빠지는 버그 — `PSPDelegate::GetOwner` 조건이 뒤집힘(`Misc/Delegate.h:255`) | **완료 2026-10-11 00:19, 미커밋** — 사용자 대시보드 "지금 진행해줘"(00:01) | GUI 세션 발견(10-03). `GetOwner` = 살아 있는 주인 주소(죽었으면 nullptr) → AddSP 구독도 주인 기준 해제(`RemoveObject` · `IsBoundTo` · `ClearIfBoundTo`)로 빠진다. 같은 파일에서 함께 고침: `RemoveObject` 가 바꿔 온 구독을 건너뛰어 같은 주인 구독이 하나 남던 것, 방송 중 해제가 콜백만 지우고 핸들을 남겨 같은 방송에서 빈 콜백 실행 assert 가 나던 것(이제 핸들 무효 → 가장 바깥 방송이 끝날 때 지움). 새 `delegate.selftest` 25/25(고치기 전 8 실패 + assert), 회귀 7종 · 에디터 종료 0 · live 0, 메인은 `MainTree.cmd -name=Core` 로 반영 · 빌드 · 회귀. 기존 사용처(GameHost · ProjectAH 의 `RemoveObject` 는 AddRaw 1개씩, GUI 백엔드 AddSP 는 `RemoveAll`) 결과 같음. 증적 `Files/2026-10-11_delegate_unsubscribe_fix_verify.txt`, 커밋 목록은 표 아래 |
| 방송 중 같은 이벤트에 구독을 더하면 실행 중인 콜백이 옮겨질 수 있음 — 목록 용량이 넘쳐 다시 잡히면 32B 이하 콜백(대리자 안에 저장, 이동 = 바이트 복사)이 있던 옛 목록 메모리가 풀에 반납된다. 그 콜백이 `Add` 뒤에 캡처를 읽으면 해제된 메모리 | 보류 | 위 버그 작업 중 발견(10-11), 기존 동작. 32B 초과(힙 저장)는 객체 주소가 그대로라 안전. 다시 볼 조건: 콜백 안에서 같은 이벤트에 구독을 더하는 코드가 생길 때. 고치는 길 예: 방송 중 추가는 대기 목록에 두고 `Unlock` 에서 붙이기 |
| 주인이 죽은 SP 구독이 목록에 남음 — 주인 기준 해제로는 못 찾고 `RemoveAll`(또는 핸들 `Remove`) 전까지 남는다 | 보류 | 위 버그 작업 중 발견(10-11). 실행해도 아무 일이 없어 안전, 비용은 방송마다 한 칸. 다시 볼 조건: 짧게 사는 객체가 SP 로 구독하고 빼지 않아 목록이 자라는 사용처가 보일 때 |
| `HMulticastDelegate::Compress()` 논리 오류 — 무효 구독을 앞으로 모은 뒤 뒤에서 잘라 유효 구독을 지운다 | 보류 | 위 버그 작업 중 발견(10-11). 호출자 0(`C:\Develop` 전체 검색). 다시 볼 조건: 부르는 코드가 생길 때 — 그때 고치거나 지운다 |
| 오디오 재생(효과음 · 배경음) 검토(ER-022) — 엔진에 오디오 모듈이 없음, 새 런타임 모듈 `Audio` | **완료 2026-10-11 14:02 메인 반영, 미커밋** — 사용자 결정 오디오 재생(D-37) "권고대로 진행"(12:51, 엔진 총괄 전달). 할 일 · 보류 · 커밋 목록은 `TODO_Audio.md`(AU-1~4 완료, 보류 AU-H1~H10), 현황 §3-5, 기록 `Files/2026-10-11_audio_D-37_verify.txt`. 검토 01시 | 권고: miniaudio 0.11.25(퍼블릭 도메인 또는 MIT-0)를 감싼 새 Audio 모듈(의존 Core 만), 1차 = 요청서 필수 전부 약 2.5~3일, 담당 Core. 쿠킹 · 게임 UI 움직임 · 에디터 · GameHost 변경 없음, JGConsole 엔진 목록 한 줄(DevOps). 수락하면 `TODO_Audio.md`(AU-1~4)를 새로 만든다. 검토서 `Files/ER-022_검토_2026-10-11.md` |
| 리플렉션 작은 결함 묶음 — ① `JGEnum::GetValueByEnumName` 이 순번을 값처럼 돌려줌(값이 0 부터 이어질 때만 맞음) ② `GetIndexByEnumName` 이 못 찾으면 -1 대신 항목 수 ③ `JGObject::GetClass()` 가 공유 클래스 정보에 마지막으로 부른 오브젝트의 속성 주소를 덮어씀(오브젝트마다 따로가 아님 · 스레드 안전 아님) ④ `GetChildClasses(true)` 가 자식이 있는 중간 클래스를 결과에서 빠뜨림 | 보류 | Core 아키텍처 그림 설명 문서를 쓰며 찾음(10-17, 조사 에이전트 보고 → 코드 직접 확인 `ObjectGlobals.cpp:288-308 · 331-351`, `ObjectGlobals.h:258-286`). 지금 쓰는 곳: Graphics `Texture.cpp:96-110`(열거형 값이 0 부터 이어져 맞음), GFW `DataTableView.h:171-184`(①② 를 알고 우회), `GameMaster.cpp:495`(`false` 라 ④ 무관), `DevelopUnit.cpp:95`(`GetClass`). 다시 볼 조건: 값이 띄엄띄엄인 열거형을 이름으로 읽을 때, 여러 오브젝트의 `GetClass()` 속성을 함께 쓰거나 다른 스레드에서 부를 때, 손자 클래스까지 `GetChildClasses(true)` 로 찾을 때 |
| 잘못된 GUID 예외(DT-H16) — 손으로 고친 잘못된 `_guid` 를 읽으면 `HGuid::FromString` 이 예외 | 보류 | 쿠킹 검사(GP-A1)가 패키징 전에 잡는다. 다시 볼 조건: 에디터에서 손으로 고친 에셋을 열다 죽는 일이 실제로 생길 때 |
| 런처 실행 인자로 명령 실행(B-1, 옛 DevConsole — `JGLauncher.exe -exec="…"` 류) | 보류 | 다시 볼 조건: Graphics 5-25 를 콘솔 명령으로 바꿔 자동 검증 루프가 리드백 PNG 를 잃을 때. 원문 `Etc/TODO_DevConsole.md` 보류 표 |

콘솔 명령 시스템 구현 현황 · 함정 · 검증은 `Etc/현황.md` §1(DevConsole 창과 함께 적힘), 명령 선언 방법은 `Source/Runtime/Core/ConsoleCommand/ConsoleCommandGlobalSystem.h` 상단 주석, 자체 검사 `console.selftest`.

**커밋 체크리스트 (10-17 문서만 — 코드 변경 없음, 커밋은 엔진 총괄)**
- 사람이 듣는 확인 닫음(D-37, 10-17 14:24): `Core/{현황.md, TODO_Audio.md}` · `진행현황.md`(1절 Core 행 · 3절 D-37 행 · 7절)
- Core 아키텍처 그림 설명(10-17 14:51): 저장소 밖 위키 `C:/obsidian/ProjectJG/02. 위키/JGEngine/모듈/Core 그림 설명.md`(새) · `모듈/Core.md`(링크 한 줄), 저장소 안 `Core/{현황.md, TODO.md}`(보류 한 줄) · `진행현황.md` 7절
- 주의: `Document/Memory/Core/` 폴더 전체가 아직 **미추적**이다(10-11 `Memory/` → `Core/` 이름 바꾼 뒤 커밋 전) — 옛 `Document/Memory/Memory/` 지움과 한 묶음으로 넣어야 한다

**커밋 체크리스트 (Core 이벤트 구독 버그 + 람다 저장(ER-023), 미커밋 — 커밋은 엔진 총괄. 두 일이 같은 두 파일을 고쳐 한 묶음)**
- 수정 `Source/Runtime/Core/Misc/Delegate.h`(HEAD 대비 +74/−32 = 이벤트 구독 +62/−29 + ER-023 +12/−3)
- 새 **미추적** `Source/Programs/JGConsole/DelegateSelfTest.cpp` — GenerateProjectFiles 돌림(JGConsole 은 폴더 단위라 `jgengine.lua` · `JGEngine.sln` 내용 변화 없음)
- 재빌드 Bin(10-11 00:17) — Core 헤더라 엔진 DLL · exe 거의 전부. 다른 트랙의 미커밋 Bin 과 같은 파일이다
- 문서 `Core/{TODO.md, 현황.md, Files/2026-10-11_delegate_unsubscribe_fix_verify.txt, Files/2026-10-11_ER-023_lambda_storage_verify.txt}` · `진행현황.md`. 재빌드 Bin 은 10-11 14:29(ER-023 — Core 헤더라 엔진 DLL · exe 거의 전부)가 마지막
- 다른 트랙과 겹치는 소스 파일 없음. 검사가 기대는 헤더(`ConsoleCommandGlobalSystem.h` · `Core.h` · PCH)는 HEAD 그대로라 커밋 순서와 무관하다(빌드로 확인한 것은 메인 미커밋 전체 상태)

---

## 다음에 할 일

없음 — **트랙 완료 · 휴면**(사용자 완료 처리 2026-09-30). 이후 메모리 작업은 구체적 문제(크래시 · 손상 · 측정된 비용 · 관측된 누수)가 보고될 때 아래 보류 표의 해당 항목을 미완료 항목으로 올려 착수한다.

2026-09-30 처리 결과(구 순서 1~5):

| 구 순서 | 할 일 | 결과 |
|---|---|---|
| 1 | 4-6. 4-1 커밋 | 완료 — `4c8ac73` "문서 정리"(2026-09-30 17:30). 다른 트랙 변경 · 문서 재편과 한 커밋(파일 분리 안 함). 작업 트리 clean |
| 2 | Phase 4 나머지 종결 확정 | 하지 않음(보류 확정, 사용자 2026-09-30) |
| 3 | 커밋 뒤 회귀 확인 1회(60초) | 따로 돌리지 않음. 전 트랙 공통 회귀(`../진행현황.md` 2절 4항: 런처 60초 · 종료 코드 0 · live 0)가 `현황.md` §6-3 기준을 포함하므로 그 결과로 대신한다. 거기서 `live blocks 0` · `large live 0` · 정상 상태 성장 로그 없음이 깨지면 이 트랙을 다시 연다 |
| 4 | 7-1 결정 | 현행(영구 인터닝) 유지, 코드 변경 없음(사용자 2026-09-30) |
| 5 | 트랙 휴면 | 적용(2026-09-30) |

---

## 미완료 항목

| ID | 항목 | 우선순위 | 상태 | 선행조건 | 완료 기준 | 비고 |
|---|---|---|---|---|---|---|
| (없음) | UI-1c(UI-1 커밋)는 `736ddbe`("엔진 구현", 2026-10-01 12:16)에 들어간 것을 2026-10-11 git 로 확인해 완료 이력으로 옮겼다 | | | | | |

미완료 코드 작업은 없다. 4-6 · 4-D · 7-1(결정)은 완료 이력으로, 커밋 뒤 회귀는 전 트랙 공통 회귀로 옮겼다(위 처리 결과). 현재 시점에 메모리 시스템에서 확인된 미해결 결함은 없다.

---

## 보류 (구체적 문제 발생 시 재검토)

2026-09-30 사용자 완료 처리로 이 표 전체가 보류 확정됐다(4-D 결정). 트리거가 관측되면 해당 항목을 미완료 항목으로 올린다.

| ID | 항목 | 재검토 트리거 | 비고 (2026-09-30 코드 기준) |
|---|---|---|---|
| 4-1 잔여 | `AllocatedMemoryBlocks`(unordered_map) · `AllocatedMemoryBlockQueue` · GC 전역 재귀 뮤텍스 제거, `Wrap(ptr)` 를 포인터 연산 O(1) 로 | `Allocate<T>`/`Wrap`/GC 순회 비용이 프로파일에 잡힐 때 | GC 객체 수백 개. 객체당 힙 할당은 풀 블록 1 + 맵 노드 + 제어 블록(4-1 로 atomic 2개 → 1개) |
| 4-2 | 파괴 정책: 참조 0 순간 지연 파괴 목록 push(매 프레임 전체 순회 제거), LIFO 파괴 순서, GPU 펜스 태깅 | 종료 순서 크래시 재발, GC 프레임 비용 측정, 매 프레임 GPU 리소스 생성 · 파괴 패턴 등장 | 지금 종료 코드 0 · D3D12 0. Graphics 5-5 의 `DestroyCommittedResource` 가 GPU 리소스를 프레임 시리얼로 지연 해제하므로 펜스 태깅 필요성은 더 낮다 |
| 4-3 | 종료 누수 리포트(`forceFlush` 시 살아 있는 객체 열거), `GGlobalSystemInstance::GetInstance()` 에 `JG_CHECK`, `DirectX12API.cpp:450-470` 폴백 함수 지역 `static HHashMap` 4개 정리 | 종료 크래시 재발, 폴백 경로 실행이 로그에 관측될 때, 4-1 의 "의도된 누수" 가 0 이 아닌 실행이 나올 때 | `Graphics_TODO.md` 5-17 과 동일 대상. 지금 폴백은 돌지 않는다 |
| 4-4 | `HTaskHandle` 완료 감지 복구(`PTask` 파괴 뒤 `IsCompelete()` 가 false 로 굳음) | `HTaskHandle::IsCompelete()` 를 쓰는 호출자가 생길 때 | 유일한 보유자 `GAssetDatabase` 는 자체 플래그를 쓰고 메서드를 부르지 않는다. `Graphics_TODO.md` 5-18 |
| 5-1 | ImGui/ImPlot 할당자를 풀로(`ImGui::SetAllocatorFunctions`, `DX12GUIBackend.cpp` `CreateContext` 직전) | GUI 메모리 사용량이 문제로 보고될 때 | 후크 존재 · 호출자 0. 1MB 초과는 대형 경로가 받으므로 기술적 선행 조건은 이미 충족 |
| 5-2 | rapidjson 할당자 재정의(`RAPIDJSON_DEFAULT_ALLOCATOR`, `PCH.h` 앞) | JSON 로드 메모리 · 성능 문제 | 구 `결정 필요`. 툴(빌드 · 헤더 툴)도 같은 PCH 를 쓰는지 확인 필요 |
| 5-3 · 5-4 | Phase 5 검증 · 커밋 | 5-1/5-2 재개 시 | |
| 6-1 | 프레임별 클래스 live 델타 경고(N 프레임 연속 증가 시 Warning 1회) | 프레임당 누수가 다시 관측될 때, 정상 상태에서 `grew to` 성장 Info 가 반복될 때 | 지금 수동 관측 수단: 성장 Info · 커밋 문턱 Warning · 위젯 클래스 표 · 트레이스 리플레이(`현황.md` §6) |
| 6-2 | 콜사이트 태깅(`typeid(T).name()`, 트레이스 빌드만) | 6-1 재개 시 | |
| 6-3 | GPU 힙 계측(업로드 페이지 · 디스크립터 페이지 · 스테이징 · PSO 캐시 수)을 위젯 GPU 절로 | GPU 메모리 예산 문제 | Graphics 영역. `HJGGraphicsModule` 노출 필요 |
| 6-4 | 시작 · 종료 스냅샷 비교(클래스별 live) | 4-3 재개 시 | |
| 6-5 · 6-6 | Phase 6 검증 · 커밋 | 6-x 재개 시 | |
| 7-1 | 문자열 테이블 수명 정책: `GStringTable::removeOldStringInfos`(`StringTable.cpp:147`) 는 큐 재삽입을 안 해 사실상 무효 = 영구 인터닝. 권장안은 영구 인터닝 확정 + 정리 코드 · `FrameCount` · `weak_ptr` 카운트 제거, `PName` 은 ID 만(릴리스 16B) | `PName` 복사 비용(원자 연산 3회)이 측정되거나 문자열 테이블 성장이 문제가 될 때 | 정리 코드가 동작하지 않는 것 자체는 무해. **현행 유지로 결정(사용자 2026-09-30)**, 코드 변경 없음 |
| 7-3 | `Cast<T>` 가 리플렉션 판정 실패 시 `RawFastCast`(static_cast) 폴백 → 디버그에서 `dynamic_cast` 검증 | 잘못된 다운캐스트 사례가 나올 때 | |
| 7-4 | `PString::_rawString` 풀 이전 재검토 | 6-4 스냅샷으로 문자열 힙 사용량을 실측한 뒤 이득이 클 때 | 비용 대비 이득이 가장 나쁨(`GetRawString()` 44곳 · `HRawString` 27곳이 `std::string` 노출). 재진입 위험은 1-1 · `JG_POOL_LOG` 가드로 해소 |
| 7-6 | Phase 7 커밋 | 7-x 재개 시 | |
| 신규 | non-const 키 `const HPair<K, V>&` 맵 순회 정리(원소 복사): `Json.h:723/749`, `ObjectGlobalSystem.cpp:286`, `ObjectGlobals.cpp:10`, `Scheduler.cpp:218`, `AssetDatabase.cpp:31/37`, `DX12Shader.cpp:11`, GameFrameWorks 6곳, JGBuildTool/JGHeaderTool | 복사한 원소의 `data()`/포인터를 잡는 사이트가 생기거나 순회 비용이 측정될 때 | 복사만 하는 곳은 정확하다(성능만). 새 코드는 `const auto&`. 실제 UAF 였던 `PipelineState.cpp` 는 Phase 2 에서 수정 |

---

## 완료 이력

| ID | 항목 | 완료일 | 검증 근거 |
|---|---|---|---|
| 0-1 | 기준선 기록 | 2026-09-28 | `Files/2026-09-28_memory_baseline.txt` v1(Allocate = Deallocate 157,892, 메인 피크 2,296, 32B 123), `Files/build_2026-09-28_phase0.log` |
| 0-2 | 고갈 · 초과 재현 코드(임시, 2-6 에서 제거) | 2026-09-28 | `Files/2026-09-28_memtest_stacks.txt`. 코드 원형 `git show fcace8f:Source/Runtime/Devkit/DevScene.cpp` |
| 1-1 | 할당 로그 컴파일 스위치 `JG_MEMORY_TRACE`, `flush_on(warn)`, `%llu`/`%llx` | 2026-09-28 | `MemoryPool.cpp:10-11`, `#if JG_MEMORY_TRACE` 4곳(767 · 805 · 934 · 1011). 기본 빌드 60초 로그 315,973 → 211줄 |
| 1-2 | `PSharedPtr`/`PWeakPtr` 대입 의미론(`assign()`, `operator=(nullptr_t)`, 자기 이동 가드) | 2026-09-28 | `Memory.h:181/267/469/530`, `Files/tools/sharedptr_semantics_test.cpp` 15/15 ALL OK |
| 1-3 | `Allocate<T>` `static_assert`, `enable_if_t` 12곳 | 2026-09-28 | `Memory.h:644, 689`. 전체 빌드 발동 0 |
| 1-4 | Phase 1 검증 | 2026-09-28 | 기준선 v2(피크 v1 과 동일), `Files/build_2026-09-28_phase1.log` |
| 1-5 | Phase 1 커밋 | 2026-09-28 | **git `fcace8f` "메모리 정리"(15:12)** — `Memory.h` · `MemoryPool.cpp` · `Log.cpp` · 문서 · 도구 포함. 구 TODO 미표기, git 로 확인 |
| 2-1 | 청크를 페이지 목록으로: 성장형 클래스 18개, 대형 경로 + 캐시, `Tick` 회수, 실패는 Critical + abort | 2026-09-28 | `MemoryPool.h/.cpp`; `exhaust32` 7/7 · `oversize` 4/4 · `largecache` 7/7 · `emptypages` 9/9; 상주 428.5 → 230.2 MB. `Graphics_TODO.md` 5-23 · 5-27 종결 |
| 2-2 | 클래스별 atomic 카운터 · `GetStatInfo` O(1) · 위젯 클래스 표 | 2026-09-28 | `HMemoryClassStatInfo`/`HMemoryChunkStatInfo`, `MemoryStatistics.cpp`, `Files/2026-09-28_phase2_memwidget_capture.png` |
| 2-3 | TLS 청크 캐시 · 청크 테이블 인덱스 · lock-free 원격 반납 스택 | 2026-09-28 | `stress` 6/6(10라운드 × 10,000 × 양방향, 손상 0, pending 0), `Dismatch` 0 |
| 2-4 | `HAllocator` 완성(`operator==` · `is_always_equal` · `alignof`) | 2026-09-28 | `Allocator.h`; `align` 4/4(1,000개 32 정렬, swap · 이동 대입). `ResourceStagingManager` swap 복원 |
| 2-6 | Phase 2 검증 + 임시 코드 제거 + 드러난 버그 2건 수정 | 2026-09-28 | `Files/2026-09-28_memtest_phase2_results.txt` 37/37, 기준선 v3, `Files/build_2026-09-28_phase2*.log`, `PipelineState.cpp` 키 const, `GUI.cpp` `PlotBarGroups` |
| 2-7 | Phase 2 커밋 | 2026-09-29 | **git `d457b92` "서밋"(14:57)** — `MemoryPool.h/.cpp` · `Allocator.h` · `Memory.h/.cpp` · `MemoryStatistics.cpp` · `PipelineState.cpp` · `ResourceStagingManager.cpp` · `GUI.cpp` · `DevScene.cpp`(훅 제거) · 도구 · 문서. 구 TODO 미표기 |
| 4-1 | 제어 블록 통합 → 축소판(약참조 카운터 수명): `HMemoryControlBlock`, `ReleaseWeakReference`, `Pin` CAS, `Wrap` 약참조 채움, 강제 파괴 시 블록 유지 | 2026-09-29 | `Memory.h:40-46, 401-407, 487-510`, `Memory.cpp:13-27, 125-132`; `Files/2026-09-29_4-1_weakptr_results.txt`(해제 카운터 접근 3 → 0, `weak` 11/11, 제어 블록 45 → 0). 커밋 `4c8ac73`(4-6) |
| 4-5 | Phase 4 검증(4-1 범위) | 2026-09-29 | 같은 결과 파일 §2 · §3, `Files/build_2026-09-29_4-1_final.log`(오류 0, LNK4098 1), 25초 실행 종료 코드 0 · 풀 live 0 |
| 4-6 | Phase 4 커밋(4-1 범위) | 2026-09-30 | **git `4c8ac73` "문서 정리"(17:30)** — `Memory.h` +44/−17 · `Memory.cpp` +28/−3(작업 트리 기록과 일치), 4-1 결과 · 빌드 로그 · 문서 포함. 다른 트랙 변경 · 문서 재편과 한 커밋 |
| 4-D | Phase 4 나머지(4-1 잔여 · 4-2 · 4-3 · 4-4) 종결 결정 | 2026-09-30 | 사용자 완료 처리 → 하지 않음. 보류 표에 재개 트리거와 함께 유지 |
| 7-1(결정) | 문자열 테이블 수명 정책 | 2026-09-30 | 사용자 완료 처리 → 현행(영구 인터닝) 유지, 코드 변경 없음. 항목은 보류 표에 남김 |
| UI-1 | 통계 창 재디자인(사용자 요청): 요약 카드 · 60초 추이 그래프 · 스레드 막대 · 스레드별 탭 클래스 표 · 상태 딱지 · 풍선 도움말, `HGUI` 대시보드 함수 28개 · `EGUIColor` · `EGUIFont` · 큰 글꼴 | 2026-09-30 | `Files/2026-09-30_memstats_ui_verify.txt`(HEAD 워크트리 빌드 오류 0, 실행 7회 종료 0 · `[error]` 0 · live 0, 메인 트리 합친 상태 컴파일 0 오류), 캡처 `Files/2026-09-30_memstats_ui_{before,after,plot_tooltip,class_tooltip,thread1}.png`. 커밋 `736ddbe`(UI-1c) |
| UI-1c | UI-1 커밋 | 2026-10-01 | **git `736ddbe` "엔진 구현"(12:16)** — `MemoryStatistics.h/.cpp` · `GUI.h/.cpp` · `GUIDefines.h` · `DX12GUIBackend.cpp` · `Files/2026-09-30_memstats_ui*` · `Files/tools/memstats_capture.ps1.txt` 포함, HEAD `GUI.cpp` 의 `HGUI::PlotLines` 1(완료 기준 충족). 사용자 일괄 커밋이라 세션 알림이 없었고 2026-10-11 git 로 확인. 그 뒤 `MemoryStatistics.cpp` 변경은 다른 트랙 것(`efba16a`, D-22 ② 미커밋) |
| 7-5 | 문서 갱신 | 2026-09-30 | 이 폴더 `현황.md`(문제 목록 해소 상태는 §9 표) · `TODO.md`. `Graphics_TODO.md` 5-23 · 5-27 종결 확인, 5-17 · 5-18 은 4-3 · 4-4 보류에 맞춰 미종결 |

---

## 제외 항목

| ID | 항목 | 이유 |
|---|---|---|
| 2-5 | 값 타입(`PString` · `PName` · `JGType` · `PSharedPtr` · `PWeakPtr` 등 9개)에서 `IMemoryObject` 분리 | 사용자 결정 2026-09-29, 재검토 조건 없음. 절감은 값당 vptr 8B(메인 청크 풀 할당 336KB 규모라 수십 KB 이하), 실제 오용 `JGType` GC 할당 4곳은 무해, 4-1 · 7-1 의 전제도 아님. 기록 `Files/Memory_2-5_값타입분리_검토_2026-09-29.md` |
| 3-1 | `HDelegate` 힙 할당을 풀로(`SetAllocationCallbacks`) | Phase 3 건너뜀(사용자 2026-09-29). 32B 초과 바인딩만 힙을 타 양이 작고, **적힌 대로 하면 힙 손상**: `Delegate.h:66-67` 의 `Alloc/Free` 가 헤더 `static`(TU 마다 사본)이라 한 TU 만 풀로 바뀐다 |
| 3-2 | 모듈 인터페이스 객체를 풀로 | 시작 시 7회 할당. 효과 없음 |
| 3-3 | `GStringTable` 참조 카운트 제어 블록을 풀로(`allocate_shared`) | 고유 문자열당 24B 1회. 7-1 권장안이 실행되면 할당 자체가 사라짐 |
| 3-4 · 3-5 | Phase 3 검증 · 커밋 | 상위 항목 제외. "통계에 보이게" 목적도 풀 커밋 약 4MB 대 프로세스 Private 230MB 라 달라지지 않음 |
| 7-2 | 스케줄러 `_syncTaskPool` 에서 종료된 작업 erase | `GUI_TODO.md` 2-1(`Unschedule` + 태스크 정리)이 같은 코드(`Scheduler.h:117/167`)를 고친다 → GUI 트랙으로 이관 |
