# Memory TODO

갱신 2026-09-30. `Files/Memory_TODO.md`(2026-09-28 작성, Phase 0~7)를 코드 · git 기준으로 최신화해 통합한 목록. 항목 ID 는 구 TODO 와 같다(다른 문서가 참조). 현황은 `현황.md`.
원칙: **지금 구체적 문제가 있는 항목만 미완료로 둔다.** "나중에 필요할지도" 성격은 보류 표에 두고 재검토 트리거를 적는다. `결정 필요` 항목은 착수 전에 사용자 확인.
검증 루프와 합격 기준은 `현황.md` §6. 커밋은 사용자가 한다.

---

## 다음에 할 일 (순서대로)

| 순서 | 할 일 | 착수 조건 | 완료 기준 |
|---|---|---|---|
| 1 | **4-6. 4-1 커밋**(사용자). 파일: `Source/Runtime/Core/Memory/Memory.h`, `Memory.cpp`, `Files/Memory_TODO.md`, `Files/2026-09-21_Memory_현황분석.md`, `Files/2026-09-29_4-1_weakptr_results.txt`, `Files/build_2026-09-29_4-1_{repro,fix,final}.log`, 이 폴더 `현황.md` · `TODO.md`. **제외**: `Devkit/DevScene.cpp` `GetBounds` 1줄 · `StaticMesh.*`(Graphics), `CoreSystem.cpp` · `Log.cpp` · `GUI.cpp`(DevConsole) | 없음 | `git log -1 -- Source/Runtime/Core/Memory/Memory.h` 가 새 커밋, `git status -- Source/Runtime/Core/Memory` 가 비어 있음 |
| 2 | **Phase 4 나머지 종결 확정**(사용자): 4-1 의 맵 · 뮤텍스 제거, 4-2, 4-3, 4-4 를 "하지 않음(보류)" 으로 닫을지 결정 | 없음 | 결정이 이 파일 보류 표 비고에 기록됨(재개면 해당 항목을 미완료 표로 올림) |
| 3 | **커밋 뒤 회귀 확인 1회**: 커밋된 트리에서 crashwalk 60초 실행 → `현황.md` §6-3 합격 기준. 4-1 의 마지막 확인은 2026-09-29 25초 실행이었다 | 1 완료 | 종료 코드 0, `live blocks 0` · `large live 0`, 정상 상태 성장 로그 없음, D3D12 0 |
| 4 | **7-1 결정**(사용자): 문자열 테이블 수명 정책. 권장 = 현행(영구 인터닝) 유지 · 코드 변경 없음 | 없음 | 결정 기록. 유지면 보류 표 그대로 |
| 5 | **트랙 휴면**: 이후 메모리 작업은 구체적 문제(크래시 · 손상 · 측정된 비용 · 관측된 누수)가 보고될 때 보류 표의 해당 항목을 꺼내 착수한다 | 1~4 완료 | — |

---

## 미완료 항목

| ID | 항목 | 우선순위 | 상태 | 선행조건 | 완료 기준 | 비고 |
|---|---|---|---|---|---|---|
| 4-6 | 4-1 커밋 — "GC 참조 카운터를 제어 블록으로 통합, 약참조 수명 수정(Memory_TODO 4-1)" | 높음 | 사용자 대기 | 없음 | 위 "다음에 할 일" 1 | 구 4-6 은 4-1~4-4 전체 커밋이었으나 4-1 만 남음. `Memory.h` · `Memory.cpp` 두 파일이 코드 전부 |
| 4-D | Phase 4 나머지(4-1 잔여 · 4-2 · 4-3 · 4-4) 종결 결정 | 높음 | `결정 필요` | 없음 | 보류 확정 또는 재개 항목 지정 | 권장: 하지 않음(2026-09-29 코드 확인: 측정된 비용 · 관측된 결함 없음) |
| 7-1 | 문자열 테이블 수명 정책 결정 | 낮음 | `결정 필요` | 없음 | 결정 기록 | 권장: 현행 유지. 상세는 보류 표 7-1 |
| — | 커밋 뒤 회귀 확인 1회(60초) | 중간 | 4-6 뒤 | 4-6 | `현황.md` §6-3 기준 전부 통과 | 결과는 `Files/2026-09-28_memory_baseline.txt` 에 한 줄 또는 새 결과 파일로 |

미완료 코드 작업은 없다. 현재 시점에 메모리 시스템에서 확인된 미해결 결함은 없다.

---

## 보류 (구체적 문제 발생 시 재검토)

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
| 7-1 | 문자열 테이블 수명 정책: `GStringTable::removeOldStringInfos`(`StringTable.cpp:147`) 는 큐 재삽입을 안 해 사실상 무효 = 영구 인터닝. 권장안은 영구 인터닝 확정 + 정리 코드 · `FrameCount` · `weak_ptr` 카운트 제거, `PName` 은 ID 만(릴리스 16B) | `PName` 복사 비용(원자 연산 3회)이 측정되거나 문자열 테이블 성장이 문제가 될 때 | 정리 코드가 동작하지 않는 것 자체는 무해. 결정은 "다음에 할 일" 4 |
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
| 4-1 | 제어 블록 통합 → 축소판(약참조 카운터 수명): `HMemoryControlBlock`, `ReleaseWeakReference`, `Pin` CAS, `Wrap` 약참조 채움, 강제 파괴 시 블록 유지 | 2026-09-29 | `Memory.h:40-46, 401-407, 487-510`, `Memory.cpp:13-27, 125-132`; `Files/2026-09-29_4-1_weakptr_results.txt`(해제 카운터 접근 3 → 0, `weak` 11/11, 제어 블록 45 → 0). **미커밋** |
| 4-5 | Phase 4 검증(4-1 범위) | 2026-09-29 | 같은 결과 파일 §2 · §3, `Files/build_2026-09-29_4-1_final.log`(오류 0, LNK4098 1), 25초 실행 종료 코드 0 · 풀 live 0 |
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
