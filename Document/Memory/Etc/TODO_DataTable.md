# DataTable TODO (JSON 데이터 테이블 에셋 · 에디터 스프레드시트 편집)

갱신 2026-10-01 (설계 보고, 코드 변경 없음). 설계 `Files/DataTable_설계_2026-10-01.md` · 현황 `현황.md` §7 · 사용자 보고서 `Document/DataTable_설계보고_2026-10-01.html`.
범위: 행(고유 키) × 열(타입) 정적 데이터를 `.jgasset`(JSON) 에셋으로 저장하고, 런타임은 읽기 전용 조회 · C++ 바인딩, 에디터는 엑셀처럼 편집하는 엔진 시스템. 코드는 Core(JSON 보강) · Asset(`DataTable/`) · GUI(그리드 컨트롤 · 한글 글꼴) · JGEditor(창)에 나뉜다.
기준: 엔진 기반 구축 · 과설계 금지. 어떤 테이블을 만들고 무슨 열을 둘지는 게임 영역이다(지침서 1절).

---

## 사용자 결정 (2026-10-01 결정됨)

| ID | 결정 | 권고 | 사용자 결정 (10-01) |
|---|---|---|---|
| DT-D1 | 스키마(열 정의) 위치 | 에셋 안 + C++ 바인딩 검사(엑셀 머리행처럼) | **권고대로** ("ㅇㅋ") |
| DT-D2 | 파일 형식 | `.jgasset`(내용 JSON) | **권고대로** ("ㅇㅋ") |
| DT-D3 | 코드 위치 | Asset 모듈 `DataTable/` | **권고와 다름 — GameFrameWorks 모듈** ("ㄴㄴ GameFrameWorks ㄱㄱ") → `Source/Runtime/GameFrameWorks/Data/`. 덕분에 JGConsole(GFW 를 연결함)에서 `datatable.selftest` 를 바로 돌린다 |
| DT-D4 | 1단계 열 타입 | Bool · Int · Float · String · Enum · AssetRef · RowRef | **이 7개로 시작.** 사용자: "나중에 리플렉션 이용해서 struct 도 지원하고 싶어" → 보류 DT-H1 에 사용자 희망으로 적고, 형식 · 타입 열거는 구조체 타입을 나중에 더할 수 있게 둔다 |
| DT-D5 | 에디터 한글 글꼴(GUI BL-3) 포함 | 포함 | **포함** ("ㄱㄱ") |
| DT-D6 | 착수 범위 | DT-0 ~ DT-4 전부 | **전부** ("ㄱㄱ") |

설계 문서의 "Asset 모듈" 위치는 DT-D3 결정으로 GameFrameWorks `Data/` 로 읽는다(설계 §4 표는 결정 전 기록).

## 다음에 할 일

| 순서 | 항목 | 착수 조건 | 완료 기준 |
|---|---|---|---|
| 1 | DT-D1 ~ D6 결정 (사용자) | 없음 | 이 표의 결정 열이 채워짐 |
| 2 | DT-0 선행(Core JSON · 원자적 저장 · 에디터 한글 글꼴) | 1. 공유 파일 미커밋분이 있으면 격리 워크트리에서 | 설계 §11 DT-0 |
| 3 | DT-1 런타임 → DT-2 편집 모델 | 2 | V1 `datatable.selftest` |
| 4 | DT-3 에디터 → DT-4 검증 | 3 | V2 실제 입력 · V3 회귀 |

## 미완료 항목

| ID | 항목 | 상태 | 선행조건 | 완료 기준 |
|---|---|---|---|---|
| DT-0-1 | Core JSON: 오류 보고판 `PJson::ToObject`(줄:칸, UTF-8 BOM 건너뜀) — `LoadObject` 만 전환, `PJsonData::GetValueType()`(값을 옮기지 않음) · `GetMemberKeys()` | 결정 대기 | DT-D1~D6 | 기존 호출 11곳 동작 그대로, 깨진 JSON 에셋이 줄:칸과 함께 실패, 회귀 통과 |
| DT-0-2 | `HFileHelper::WriteAllTextAtomic`(같은 폴더 임시 파일 → 교체) | 결정 대기 | 같음 | 셀프 테스트(덮어쓰기 · 없는 파일) |
| DT-0-3 | 에디터 한글 글꼴(GUI BL-3: Content 글꼴 → `malgun.ttf`, 기본 글꼴에 한글 범위 합침) + `HGUI::SetClipboardText/GetClipboardText`(게임 UI ER-008 · D-14 가 먼저 만들면 그것을 쓴다) | 결정 대기 | DT-D5 | 한글 문자열 캡처, 아틀라스 크기 · 시작 시간 기록, 다른 창 회귀 없음 |
| DT-1-1 | 값 · 타입(`EDataTableColumnType`, `HDataTableValue`, 텍스트 ↔ 값, JSON ↔ 값) | 결정 대기 | DT-0-1 | V1 값 왕복 |
| DT-1-2 | 스키마(`HDataTableColumn`, `HDataTableSchema`, 열 옵션) | 결정 대기 | 1-1 | V1 |
| DT-1-3 | `JGDataTable` 읽기 · 쓰기 · `FromJsonText`(형식 v1, 모르는 키 보고) | 결정 대기 | 1-2 | 다시 저장 바이트 동일, 파싱 오류 줄:칸 |
| DT-1-4 | 조회 · `ComputeContentHash` · `GetRevision` · `OnChanged` | 결정 대기 | 1-3 | V1 지문 안정 · 변화 |
| DT-1-5 | `HDataTableRowBinder<T>` · `HDataTableView<T>` | 결정 대기 | 1-4 | 바인딩 성공 · 열 없음 · 타입 불일치 · 열거형 이름 |
| DT-1-6 | 검증(`HDataTableIssue`, 키 · 범위 · Enum · RowRef · AssetRef, 가짜 해석기) | 결정 대기 | 1-4 | V1 |
| DT-1-7 | `GAssetDatabase` 연동(타입별 로드된 에셋 목록 · 새 테이블 등록 · 여러 에셋 기다리기) | 결정 대기 | 1-3 | 에디터에서 새 테이블이 재시작 없이 목록에 |
| DT-1-8 | 셀프 테스트 본체(Asset) + `JGConsole datatable.selftest`(JGConsole 의존 `Asset`) + 에디터용 `datatable.validate` | 결정 대기 | 1-6 | 종료 코드 0, 결과 txt. 첫 단계에서 "Asset 미연결 상태로 함수 호출"이 되는지 확인(설계 §10) |
| DT-2-1 | `PDataTableDocument`: 작업 사본 · 저장(검증 → 원자적 쓰기 → 살아 있는 에셋 반영) · 되돌리기 · 바뀐 파일 감지 | 결정 대기 | DT-1 | V1 |
| DT-2-2 | 편집 명령 · 실행 취소(셀 범위 · 행 · 열 · 키 · 정렬 적용, 한도 500) | 결정 대기 | 2-1 | 무작위 명령 열 → 전부 되돌리면 원본 바이트 |
| DT-2-3 | TSV 클립보드 · 붙여넣기 규칙 | 결정 대기 | 2-2 | V1 TSV · 붙여넣기 |
| DT-2-4 | 보기 정렬 · 필터(파일 순서 불변) | 결정 대기 | 2-1 | V1 |
| DT-3-1 | `PGUIGrid`(GUI): 고정 머리 · 키 열, 가상화, 사각 선택, 키보드, 셀 편집기 | 결정 대기 | DT-0-3 | V2 |
| DT-3-2 | `JGDataTableEditor`(JGEditor): 메뉴 `Windows/Data Table Editor`, 탭, 도구 막대, 단축키 | 결정 대기 | 3-1, DT-2 | V2 |
| DT-3-3 | 열 속성 패널 · 새 테이블 대화상자 | 결정 대기 | 3-2 | V2 |
| DT-3-4 | 문제 목록 · 상태 줄 · 바뀐 파일 알림 줄 | 결정 대기 | 3-2 | V2 |
| DT-4-1 | V2 실제 입력 · V3 회귀 · 1만 행 측정 | 결정 대기 | DT-3 | 증적 `Files/` |
| DT-4-2 | 사용 안내(게임 모듈에서 테이블을 만들고 묶는 법) · 보고서 | 결정 대기 | 4-1 | 문서 |
| DT-C1 | 커밋 (사용자) | 대기 | DT-4 | 커밋 체크리스트 |

## 보류 (다시 볼 조건이 오면 착수)

| ID | 항목 | 다시 볼 조건 |
|---|---|---|
| DT-H1 | 목록(List) 열 · **구조체(struct) 열 — 사용자 희망(10-01): "나중에 리플렉션 이용해서 struct 도 지원"**. 선행: JGHeaderTool 의 `JGSTRUCT` 지원 + 타입 없는 값 읽기 · 쓰기(지금 `JGProperty::SetValue<T>` 는 컴파일 타임 타입 필요). 셀은 JSON 객체, 열은 `"Type": "Struct", "StructType": "<이름>"` 식으로 `FormatVersion` 을 올려 더한다 | 사용자가 착수를 정할 때 |
| DT-H2 | 행 상속(원본 + 델타, GFW 방향 문서 C4) | 비슷한 행을 복사해 따로 유지하는 일이 반복될 때 |
| DT-H3 | 여러 파일을 한 테이블로(묶음별 분할) | 한 파일로 다루기 어렵게 커지거나 협업 충돌이 잦을 때 |
| DT-H4 | 스키마 → C++ 행 구조체 코드 생성, 행 타입 등록으로 에디터가 코드 쪽 열을 보여 주기 | 바인딩 반복 · 열 이름 오타가 실제 문제일 때 |
| DT-H5 | `.csv` · `.json` 가져오기 · 내보내기 명령 | 바깥 도구로 대량 수정이 필요할 때 |
| DT-H6 | 키 바꾸기 → 다른 테이블 RowRef 함께 갱신, 참조 찾기 | 키 변경이 잦아질 때 |
| DT-H7 | 테이블 내용 지문을 `PGameMaster::RulesFingerprint` 에(GFW · Server 조정) | 네트워크 게임이 테이블을 규칙에 쓰기 시작할 때 |
| DT-H8 | 실행 중인 게임에 데이터 즉시 반영하는 정책(JGEditor E-H3 월드 재시작과 함께) | 테이블을 고치며 바로 게임에서 보는 일이 반복될 때 |
| DT-H9 | 에디터 종료 때 저장 안 된 변경 확인 · 복구 파일(GUI 닫기 요청 훅 필요) | 변경을 잃는 일이 실제로 생길 때 |
| DT-H10 | 지역화 문자열 테이블(언어별 열) | 지역화 시스템을 시작할 때 |
| DT-H11 | 열린 테이블 목록을 레이아웃과 함께 저장(JGEditor E-2 방식) | 재시작마다 다시 여는 게 불편할 때 |
| DT-H12 | 한글 IME 로 바로 입력할 때 조합 시작에서 편집 시작 | V2 에서 첫 글자 처리가 실제로 어색할 때 |

## 완료 이력

| ID | 항목 | 완료일 | 근거 |
|---|---|---|---|
| DT-S0 | 설계 보고(사용자 요청 "데이터 관련 기반 작업 — .json 기반, 에디터에서 엑셀처럼 편집 — 어떻게 설계할지 보고") | 2026-10-01 | `Files/DataTable_설계_2026-10-01.md`, `Document/DataTable_설계보고_2026-10-01.html`. 코드 변경 · 빌드 없음 |

## 커밋 체크리스트

- 지금(DT-S0, 문서만, 새 파일 — `git add` 필요): `Document/DataTable_설계보고_2026-10-01.html`, `Document/Memory/Etc/{TODO_DataTable.md, Files/DataTable_설계_2026-10-01.md}`, 수정 `Document/Memory/Etc/현황.md`(§7) · `Document/Memory/{진행현황.md, README.md}`
- 구현 뒤(DT-C1): 구현하면서 채운다
