# DataTable TODO (JSON 데이터 테이블 에셋 · 에디터 스프레드시트 편집)

갱신 2026-10-17 14:22 — 다음 할 일 3번(Data Table Editor 사용자 손 확인)을 닫음: 사용자 건너뜀(10-17 14:21, 통합 대시보드 결정함 "건너뜀" — 엔진 총괄 전달) — 지금 구현 그대로. 보류 DT-H12 는 그대로. 이전: 2026-10-02 밤 — DT-0 ~ DT-4 는 **커밋됨**(사용자 일괄 커밋 `efba16a`, 10-01 16:58). 10-02 밤 **게임 빌드 파이프라인 D-23 의 GP-A1 쿠킹 검사**(엔진 총괄 배정)를 구현 · 메인 반영 — 미커밋(DT-C2). 증적 `Files/2026-10-02_GP-A1_cookcheck_verify.txt`.
설계 · 결과 `Files/DataTable_설계_2026-10-01.md`(§14 결과 · §15 사용 안내) · 증적 `Files/2026-10-01_datatable_verify.txt` · 캡처 `Files/2026-10-01_datatable_v2_*.png` · 현황 `현황.md` §11 · 사용자 보고서 `Document/DataTable_설계보고_2026-10-01.html`.
범위: 행(고유 키) × 열(타입) 정적 데이터를 `.jgasset`(JSON) 에셋으로 저장하고, 런타임은 읽기 전용 조회 · C++ 바인딩, 에디터는 엑셀처럼 편집하는 엔진 시스템. 코드는 Core(JSON 보강 · 원자적 저장) · **GameFrameWorks `Data/`**(런타임 · 편집 모델) · GUI(그리드 컨트롤 · 한글 글꼴) · JGEditor(창)에 나뉜다.
기준: 엔진 기반 구축 · 과설계 금지. 어떤 테이블을 만들고 무슨 열을 둘지는 게임 영역이다(지침서 1절).

---

## 사용자 결정 (2026-10-01 결정됨)

| ID | 결정 | 권고 | 사용자 결정 (10-01) |
|---|---|---|---|
| DT-D1 | 스키마(열 정의) 위치 | 에셋 안 + C++ 바인딩 검사(엑셀 머리행처럼) | **권고대로** ("ㅇㅋ") |
| DT-D2 | 파일 형식 | `.jgasset`(내용 JSON) | **권고대로** ("ㅇㅋ") |
| DT-D3 | 코드 위치 | Asset 모듈 `DataTable/` | **권고와 다름 — GameFrameWorks 모듈** ("ㄴㄴ GameFrameWorks ㄱㄱ") → `Source/Runtime/GameFrameWorks/Data/`. 덕분에 JGConsole(GFW 를 연결함)에서 `datatable.selftest` 를 바로 돌린다 |
| DT-D4 | 1단계 열 타입 | Bool · Int · Float · String · Enum · AssetRef · RowRef | **이 7개로 시작.** 사용자: "나중에 리플렉션 이용해서 struct 도 지원하고 싶어" → 보류 DT-H1 에 사용자 희망으로 적었다. 형식(`FormatVersion`) · 타입 열거(`EDataTableColumnType`)는 구조체 타입을 나중에 더할 수 있게 뒀다 |
| DT-D5 | 에디터 한글 글꼴(GUI BL-3) 포함 | 포함 | **포함** ("ㄱㄱ") |
| DT-D6 | 착수 범위 | DT-0 ~ DT-4 전부 | **전부** ("ㄱㄱ") |

## 다음에 할 일

| 순서 | 항목 | 착수 조건 | 완료 기준 |
|---|---|---|---|
| 1 | **DT-C2 커밋 (사용자)** — GP-A1 새 파일 5개 | 없음 | 아래 커밋 체크리스트(DT-C2). DevOps 의 `JGConsole/Cook/CookCommand.cpp` 가 `__has_include` 로 이 헤더를 찾아 부르므로 같은 커밋이 좋다 |
| 2 | GP-A1 실제 쿠킹 확인 — DevOps 가 GP-D4 쿠킹을 메인에서 돌린 결과를 받는다 | DevOps 쿠킹 실행 | 검사 단계 통과 · 실패 메시지 형식이 쓸 만한지. GP-G4 전에는 셰이더(Source/Shader)를 검사 입력에서 빼기로 DevOps 와 정했다 |
| 3 | ~~사용자 손 확인(선택): 실제 마우스 · 키보드 · 한글 IME 로 셀 입력~~ | — | **닫음 — 사용자 건너뜀(10-17 14:21, 통합 대시보드 결정함 "건너뜀" — 엔진 총괄 전달) — 지금 구현 그대로**. 10-01 V2 는 메시지 주입이라 IME 조합은 못 봤다. 쓰다가 첫 글자가 어색하면 보류 DT-H12 |
| 4 | 게임 프로젝트 에디터에서 `/JGGame/` 테이블 한 번 만들기 · 묶기(설계 §15) | 게임 쪽에서 테이블이 필요할 때 | 생성 · 저장 · `HDataTableView<T>` 바인드 · 재시작 뒤 로드 |

## 미완료 항목

| ID | 항목 | 상태 | 선행조건 | 완료 기준 |
|---|---|---|---|---|
| DT-C2 | GP-A1 커밋 (사용자) | 대기 | 없음 | 커밋 체크리스트(DT-C2) |

## 보류 (다시 볼 조건이 오면 착수)

| ID | 항목 | 다시 볼 조건 |
|---|---|---|
| DT-H1 | 목록(List) 열 · **구조체(struct) 열 — 사용자 희망(10-01): "나중에 리플렉션 이용해서 struct 도 지원"**. 선행: JGHeaderTool 의 `JGSTRUCT` 지원 + 타입 없는 값 읽기 · 쓰기(지금 `JGProperty::SetValue<T>` 는 컴파일 타임 타입 필요). 셀은 JSON 객체, 열은 `"Type": "Struct", "StructType": "<이름>"` 식으로 `FormatVersion` 을 올려 더한다. 그리드는 셀 팝업 편집 | 사용자가 착수를 정할 때 |
| DT-H2 | 행 상속(원본 + 델타, GFW 방향 문서 C4) | 비슷한 행을 복사해 따로 유지하는 일이 반복될 때 |
| DT-H3 | 여러 파일을 한 테이블로(묶음별 분할) | 한 파일로 다루기 어렵게 커지거나 협업 충돌이 잦을 때 |
| DT-H4 | 스키마 → C++ 행 구조체 코드 생성, 행 타입 등록으로 에디터가 코드 쪽 열을 보여 주기 | 바인딩 반복 · 열 이름 오타가 실제 문제일 때 |
| DT-H5 | `.csv` · `.json` 가져오기 · 내보내기 명령 | 바깥 도구로 대량 수정이 필요할 때 |
| DT-H6 | 키 바꾸기 → 다른 테이블 RowRef 함께 갱신, 참조 찾기 | 키 변경이 잦아질 때 |
| DT-H7 | 테이블 내용 지문을 `PGameMaster::RulesFingerprint` 에(GFW · Server 조정) | 네트워크 게임이 테이블을 규칙에 쓰기 시작할 때 |
| DT-H8 | 실행 중인 게임에 데이터 즉시 반영하는 정책(JGEditor E-H3 월드 재시작과 함께) | 테이블을 고치며 바로 게임에서 보는 일이 반복될 때 |
| DT-H9 | 에디터(앱) 종료 때 저장 안 된 변경 확인 · 복구 파일. 10-01 확인: 창 X 로 닫으면 위젯 인스턴스가 남아 탭 · 변경이 그대로 있고(다시 열면 보임), **앱 종료는 묻지 않고 버린다** — 엔진에 닫기 요청 가로채기 훅이 없다 | 변경을 잃는 일이 실제로 생길 때 |
| DT-H10 | 지역화 문자열 테이블(언어별 열) | 지역화 시스템을 시작할 때 |
| DT-H11 | 열린 테이블 목록을 레이아웃과 함께 저장(JGEditor E-2 방식). 지금은 창만 복원되고 탭은 다시 연다 | 재시작마다 다시 여는 게 불편할 때 |
| DT-H12 | 한글 IME 로 바로 입력할 때 조합 시작에서 편집 시작 | 손 확인에서 첫 글자 처리가 실제로 어색할 때(그 전에는 F2 · Enter 로 편집 시작) |
| DT-H13 | 보기 정렬 · 필터가 바뀔 때 선택을 같은 데이터 행에 유지(지금은 보기 행 번호 기준이라 다른 행으로 간다) | 정렬 · 필터를 켜고 끄며 편집하는 일이 잦을 때 |
| DT-H14 | 한글 글꼴 범위를 KS X 1001 2,350자로 줄이기(아틀라스 1024×2048 = 8 MB, 시작 123 ms → 약 1/4) · 열 폭 저장(그리드는 `NoSavedSettings`) | 에디터 VRAM · 시작 시간이 문제 될 때 / 열 폭을 매번 맞추는 게 불편할 때 |
| DT-H15 | 큰 테이블 필터 입력 속도(1만 행 글자 하나에 131~258 ms, DevelopEngine) — 칸 글 캐시 또는 입력 뒤 잠깐 기다렸다 거르기 | 수만 행 테이블이 생기거나 입력 끊김이 실제로 불편할 때 |
| DT-H16 | (Core — 엔진 총괄에 보고) 에셋 로드의 `_guid` 해석 `HGuid::FromString` 이 "-" 조각 11개 이상 · 숫자 아닌 조각이면 `std::stoull` 예외를 던지고, 16/8비트 범위를 넘으면 조용히 자른다(`Core/Misc/Guid.cpp`, `Json.h:1302` 경로). 쿠킹 검사(GP-A1)는 직접 해석해 미리 잡는다 | 손으로 고친 에셋을 열다 에디터가 죽는 일이 실제로 생길 때 |
| GP-H3 | (설계 5절 후속, Data 담당) 시작 월드에서 참조를 따라가 필요한 에셋만 쿠킹 · 이진 에셋 · 묶음 파일 | 패키지 크기 · 로드 시간이 문제 될 때 |

## 완료 이력

| ID | 항목 | 완료일 | 근거 |
|---|---|---|---|
| DT-S0 | 설계 보고(사용자 요청 "데이터 관련 기반 작업 — .json 기반, 에디터에서 엑셀처럼 편집 — 어떻게 설계할지 보고") | 2026-10-01 | `Files/DataTable_설계_2026-10-01.md`, `Document/DataTable_설계보고_2026-10-01.html` |
| DT-0-1 | Core JSON: `PJson::ToObjectWithError`(줄 · 칸, UTF-8 BOM 건너뜀) — `LoadObject` 만 전환, `PJsonData::GetValueType()` · `GetMemberKeys()`(값을 옮기지 않음), `EJsonValueType` | 2026-10-01 | 기존 `ToObject` 호출 그대로, V1 BOM · 파싱 오류 줄:칸 · 메인 회귀 5종 통과 |
| DT-0-2 | `HFileHelper::WriteAllTextAtomic`(`<path>.tmp` → `fs::rename`) | 2026-10-01 | V1 덮어쓰기 · 없는 파일 · 임시 파일 남지 않음 |
| DT-0-3 | 에디터 한글 글꼴(GUI BL-3): `Content/Fonts/EditorKorean.ttf` → `malgun.ttf` → 경고. 클립보드는 게임 UI(ER-008)의 `HGUI::Get/SetClipboardText` 사용 | 2026-10-01 | 한글 셀 · 필터 캡처, 아틀라스 512×256 → 1024×2048(8 MB) · 123 ms, 메인 에디터 실행 경고 없음 |
| DT-1-1 ~ 1-6 | 값 · 타입, 스키마, `JGDataTable`(형식 v1 · 다시 저장 바이트 동일 · 모르는 키 보고), 조회 · 지문 · Revision · `OnChanged`, `HDataTableRowBinder<T>` · `HDataTableView<T>`, 검증(`HDataTableIssue`, 키 · 범위 · Enum · RowRef · AssetRef) | 2026-10-01 | V1 127/127 |
| DT-1-7 | 에셋 DB 연동 — `GetLoadedDataTables` · `GetLoadedAssetPaths`(Asset 은 고치지 않음, 공개 `_assetsByAssetPath` 를 읽음), 새 테이블 → `LoadAssetAsync` | 2026-10-01 | V2 새 테이블이 재시작 없이 목록에, 재시작 뒤 목록 |
| DT-1-8 | `datatable.selftest` · `datatable.validate [-path=]`(GFW `HAutoConsoleCommand`) | 2026-10-01 | JGConsole 종료 0, validate 0개 · 파일 경로 OK |
| DT-2-1 ~ 2-4 | `PDataTableDocument`(작업 사본 · 저장 · 다시 읽기 · 바뀐 파일 감지), 편집 10종 · 되돌리기 500, TSV · 붙여넣기 규칙, 보기 정렬 · 필터(파일 순서 불변) | 2026-10-01 | V1(무작위 편집 → 전부 되돌리면 원본 바이트 등) · V2 |
| DT-3-1 ~ 3-4 | `PGUIGrid`(GUI `Grid/`), `JGDataTableEditor`(메뉴 `Windows/Data Table Editor`, 탭 · 도구 막대 · 단축키), 열 속성 패널 · 새 테이블 대화상자, 문제 목록 · 상태 줄 · 바뀐 파일 알림 줄 | 2026-10-01 | V2 캡처 10장 · 증적 txt §3. V2 중 고친 것 4건(상태 줄) 포함 |
| DT-4-1 | V2 실제 입력 · V3 회귀 · 1만 행 측정, 메인 트리 반영 · 빌드 · 회귀 | 2026-10-01 | `Files/2026-10-01_datatable_verify.txt` |
| DT-4-2 | 사용 안내(게임 모듈에서 테이블을 만들고 묶는 법) · 보고서 | 2026-10-01 | 설계 §15, `Document/DataTable_설계보고_2026-10-01.html` 결과 절 |
| DT-C1 | DT-0 ~ DT-4 커밋 | 2026-10-01 16:58 | 사용자 일괄 커밋 `efba16a` "게임 구현"(DataTable 소스 · 문서 · 증적 포함 — `git log` 로 확인) |
| GP-A1 | **쿠킹 검사(게임 빌드 파이프라인 D-23, 엔진 총괄 배정)** — 검사 본체 Asset `Cook/AssetCookValidator.*`(`ValidateAssetsForCook`: ① 파싱 · JGObjectType · JGObject · `_guid` ② GUID 두 마운트 통틀어 하나 ③ 마운트 토큰 문자열 참조가 쿠킹 대상 안 — HAssetPath 규칙 · 대소문자 구분 · 자기 경로 칸 제외 + 타입별 검사 자리), 입구 GFW `Data/ContentCookCheck.*`(`ValidateContentForCook(files, mounts, errors, warnings)` — ④ 데이터 테이블 검사를 끼움, 헤더는 Core 형식만 + 토큰 매크로), 자체 검사 `cookcheck.selftest`. 지시서(Asset 모듈)와 달리 입구가 GFW 인 이유: 데이터 테이블이 GFW(D3)라 Asset 이 못 부르고, JGConsole 은 Core · GFW 헤더만 본다 | 2026-10-02 밤 (미커밋) | `Files/2026-10-02_GP-A1_cookcheck_verify.txt`: 워크트리 · 메인 `cookcheck.selftest` 26/26(정상 통과 · 깨진 참조 · 대소문자 · GUID 중복 · 잘못된 GUID · 파싱 오류 · 마운트 밖 · 키 중복 · 읽을 수 없는 테이블 실패, 36MB 실제 에셋 통과 1.9 s), `datatable.selftest` 127/127, 메인 빌드(DevOps 확인 exit 0). DevOps `cook.run` 7단계가 부른다 |

## 커밋 체크리스트 (DT-C2 — GP-A1)

새 파일(`git add` 필요):
- `Source/Runtime/Asset/Cook/AssetCookValidator.h/.cpp`
- `Source/Runtime/GameFrameWorks/Data/ContentCookCheck.h/.cpp`, `Source/Runtime/GameFrameWorks/Data/ContentCookCheckSelfTest.cpp`
- `Document/Memory/GameFrameWorks/Files/2026-10-02_GP-A1_cookcheck_verify.txt`

수정: 재빌드된 `Bin/DevelopEngine` 의 `Asset` · `GameFrameWorks` DLL/lib/exp · `JGConsole.exe`(다른 트랙 변경과 같은 빌드), 문서 `Document/Memory/GameFrameWorks/{TODO_DataTable.md, 현황.md}` · `Document/Memory/진행현황.md`.
같이 들어가면 좋은 것: DevOps 의 `Source/Programs/JGConsole/Cook/`(이 검사를 부르는 쪽).

## 커밋 체크리스트 (DT-C1 — 10-01, `efba16a` 로 커밋됨, 기록용)

새 파일(`git add` 필요):
- `Source/Runtime/GameFrameWorks/Data/` — `DataTable.h/.cpp`, `DataTableAssets.h/.cpp`, `DataTableClipboard.h/.cpp`, `DataTableContent.h/.cpp`, `DataTableDocument.h/.cpp`, `DataTableSchema.h/.cpp`, `DataTableSelfTest.cpp`, `DataTableTypes.h/.cpp`, `DataTableValidation.h/.cpp`, `DataTableView.h` (18개)
- `Source/Runtime/GUI/Grid/GUIGrid.h/.cpp`
- `Source/Editor/JGEditor/Widgets/DataTableEditor.h/.cpp`
- `Document/Memory/Etc/Files/2026-10-01_datatable_verify.txt`, `Document/Memory/Etc/Files/2026-10-01_datatable_v2_*.png`(10장), `Document/Memory/Etc/Files/tools/{datatable_ui_driver.ps1.txt, datatable_uitest_hook.patch.txt}`

수정:
- `Source/Runtime/Core/FileIO/Json.h/.cpp`, `Source/Runtime/Core/FileIO/FileHelper.h/.cpp`, `Source/Runtime/Core/Object/ObjectGlobalSystem.h`
- `Source/Runtime/GUI/GUI.h/.cpp`(끝에 한 묶음 추가), `Source/Runtime/GUI/Backends/DX12GUIBackend.cpp`
- `Source/Editor/JGEditor/JGEditor.cpp`(include + 메뉴)
- 재빌드된 `Bin/DevelopEngine` — Core 헤더가 바뀌어 거의 전부(`Core.lib` · `Asset` · `Graphics` · `AI` · `GUI` · `DevStatistics` · `DevConsole` · `Devkit` · `GameFrameWorks` · `JGEditor` DLL/lib/exp, `JGConsole.exe` · `JGLauncher.exe` · `JGHeaderTool.exe` · `JGBuildTool.exe`), 로그 `Bin/DevelopEngine/jg_log.txt` · `Build/BatchFiles/jg_log.txt`
- 문서: `Document/DataTable_설계보고_2026-10-01.html`, `Document/Memory/Etc/{TODO_DataTable.md, 현황.md, TODO_GUI.md, TODO_JGEditor.md, Files/DataTable_설계_2026-10-01.md}`, `Document/Memory/{진행현황.md, README.md}`

넣지 않는 것: 워크트리 전용 `imgui_impl_win32.cpp` `JG_UITEST` 훅 · 테스트 테이블(`Content/DataTableTest/`) · 임시 측정 코드 — 메인에 없다(grep 0). `Bin/DevelopEngine/imgui.ini` 는 실행 전 상태로 되돌려 바뀌지 않았다.
