# DataTable 설계 — JSON 데이터 테이블 에셋과 에디터 스프레드시트 편집 — 2026-10-01

상태: **구현 완료 · 메인 트리 반영 · 커밋 대기 (2026-10-01).** 결정(DT-D1~D6)은 §3 표 아래, 실제 파일 · 설계와 달라진 점 · 함정 · 검증 결과는 §14, 게임 코드에서 쓰는 법은 §15. §0~§13 은 설계 당시 기록이다(코드 위치 "Asset" 은 DT-D3 결정으로 GameFrameWorks `Data/`).
요청(2026-10-01 사용자): "데이터 관련 기반 작업을 진행할까 해. .json 기반으로 할 거고, 에디터에서 해당 에셋을 엑셀처럼 편집할 수 있도록 할 거야. 관련해서 어떻게 설계할 건지 보고해줘."
사용자 보고서 `Document/DataTable_설계보고_2026-10-01.html` · 할 일과 결정 `../TODO_DataTable.md` · 현황 `../현황.md` §7.

---

## 0. 결론

| 항목 | 설계 |
|---|---|
| 시스템 | **DataTable** — 행(고유 키) × 열(타입이 있는 값)로 된 정적 데이터. 어떤 보드 · 카드 게임에도 그대로 쓰인다(지침서 1절 판단 기준). 어떤 테이블을 만들고 무슨 열을 둘지는 게임 영역이다 |
| 저장 | `JGDataTable : JGAsset` 를 기존 `.jgasset`(내용은 JSON)으로 저장. 열 정의(스키마)와 행을 한 파일에 둔다. 행 = 열 이름을 키로 한 JSON 객체, 값마다 한 줄(셀 하나를 고치면 diff 도 한 줄) |
| 스키마 위치 (권고 DT-D1) | **에셋 안.** 에디터는 게임 코드 없이 테이블을 열고 열을 추가 · 수정한다(엑셀 머리행처럼). C++ 는 멤버 포인터로 필요한 열만 묶고(`HDataTableView<T>`), 묶을 때 열 이름 · 타입을 검사해 어긋나면 오류를 낸다 |
| 런타임 | 읽기 전용. 키 → 행 O(1), 순회 순서 = 파일의 행 순서(결정론). `ComputeContentHash()` 로 내용 지문 |
| 편집 | 편집 모델 `PDataTableDocument`(명령 · 실행 취소 · TSV 클립보드 · 검증)는 UI 없는 로직이라 Asset 모듈에 두고 헤드리스로 검증한다. 창 `JGDataTableEditor` 는 JGEditor, 그리드 컨트롤 `PGUIGrid` 는 GUI 모듈 |
| 엑셀처럼 | 셀 선택 · 키보드 이동 · 바로 입력 · 범위 선택 · 복사/붙여넣기(엑셀 · 구글 시트와 TSV 호환) · 행/열 추가 · 삭제 · 이동 · 실행 취소 · 정렬 · 필터 · 저장. 수식 · 서식 · 차트는 하지 않는다 |
| 선행 | Core JSON 보강(파싱 오류 위치 · 값 종류 묻기 · 객체 키 나열, 추가만), 에디터 한글 글꼴(GUI BL-3). 지금 에디터 글꼴은 영문뿐이라 한글 셀이 보이지 않는다 |

---

## 1. 요구사항

| # | 요구 | 출처 |
|---|---|---|
| R1 | JSON 저장. 사람이 읽고, 텍스트 도구 · git diff · 다른 에이전트가 고칠 수 있다 | 사용자 |
| R2 | 에디터에서 엑셀처럼 편집(셀 단위 · 키보드 · 복사/붙여넣기 · 실행 취소) | 사용자 |
| R3 | 열마다 타입. 잘못된 값은 입력 때 거르고, 깨진 참조 · 범위 위반은 표시한다 | R2 에서 |
| R4 | 런타임은 키로 빠르게 찾고, 순회 순서가 늘 같다 | GameMaster 결정론(GFW 현황) |
| R5 | 다른 테이블의 행, 다른 에셋을 참조하고 깨진 참조를 찾는다 | 방향 문서 C3 |
| R6 | 기존 에셋 파이프라인(GUID · `/JGEngine/` · `/JGGame/` · 비동기 로드)을 그대로 쓴다 | Asset 모듈 |
| R7 | 헤드리스 검증(JGConsole). 네트워크 양쪽이 같은 데이터인지 확인할 수단 | 지침서 3절, Server |
| R8 | 게임 모듈이 없거나 빌드가 깨져도 에디터가 데이터를 열고 고칠 수 있다 | 설계 판단(DT-D1) |

---

## 2. 출발점 — 지금 코드에서 확인한 것

| 영역 | 사실 | 근거 | 설계에 주는 영향 |
|---|---|---|---|
| 에셋 | `.jgasset` 은 JSON(`{"JGObjectType", "JGObject"}`), `LoadObject` 가 타입 이름으로 객체를 만든다. `GAssetDatabase` 는 시작 때 두 Content 의 `.jgasset` 을 모두 비동기로 올린다 | `Core/Object/ObjectGlobalSystem.h` `SaveObject/LoadObject`, `Asset/AssetDatabase.cpp:189-212` | 새 에셋 타입 하나로 저장 · 로드 · GUID · 경로가 따라온다 |
| 손 직렬화 선례 | `JGTexture` 는 `JG_GENERATED_SIMPLE_BODY` + 직접 `WriteJson/ReadJson`, 열거형은 이름 문자열로 저장 | `Graphics/Classes/Texture.h:35`, `Texture.cpp:25` | `JGDataTable` 도 같은 방식(형식을 우리가 정한다) |
| 리플렉션 | 값 읽기 · 쓰기가 컴파일 타임 타입을 요구한다(`JGProperty::SetValue<T>` · `GetValue<T>`) — 타입을 모르는 범용 값 편집 경로가 없다. `JGSTRUCT` 는 빈 매크로(HeaderTool 토큰에 없음). 프로퍼티 파서는 공백으로 나눠 `HHashMap<PName, int32>` 같은 타입을 못 읽고, 메타 값의 공백을 지운다. `EPropertyType` 은 쓰는 곳이 없다 | `Core/Object/ObjectGlobals.h:80-115`, `ObjectDefines.h:8`, `JGHeaderTool/Class/HeaderToolConstants.h:8-12`, `HeaderTool.cpp:575-596, 727` | UE DataTable 처럼 "C++ 구조체가 스키마"로 가려면 리플렉션 확장(구조체 · 타입 없는 값 핸들 · 파서)이 먼저다 → DT-D1 |
| JSON | `PJson::ToObject` 는 파싱 오류를 무시하고 true 를 돌려준다(호출 11곳). 값을 읽으면 원본에서 **이동**한다(같은 값을 두 번 못 읽는다 — 종류를 "시험 삼아" 읽어 볼 수도 없다). 객체 키를 나열하는 API 가 없고, 종류를 묻는 API 는 `IsString` 하나다. 쓰기는 `PrettyWriter`(값마다 한 줄), 키 순서 = 넣은 순서 | `Core/FileIO/Json.cpp:320-330`, `Json.h:69, 813-814`, `Json.cpp:306` | DT-0-1: 오류 위치 보고 · 값 종류 묻기 · 키 나열을 **추가**한다(기존 호출 동작은 그대로) |
| 파일 | 텍스트 모드 `std::ofstream/ifstream`, 좁은 문자 경로, BOM 처리 없음, 바로 덮어쓴다 | `Core/FileIO/FileHelper.cpp:11-50` | 내용 UTF-8 은 그대로 통과. 파일 이름은 ASCII(한글 프로젝트 경로 거부와 같은 이유), 읽을 때 BOM 을 건너뛰고, 저장은 임시 파일 → 교체 |
| 에디터 GUI | ImGui 1.91.1 docking(테이블 API · `ImGuiListClipper` 있음). ImGui 는 GUI 모듈 안에서만 부르고 다른 모듈은 `HGUI` 래퍼를 쓴다. 키 래퍼는 Escape 하나, 클립보드 래퍼 없음. 글꼴은 기본 영문 글꼴뿐. 위젯은 클래스마다 인스턴스 하나 | `GUI/Imgui/imgui.h:30`, `GUI/GUIDefines.h:33-37`, `GUI/Backends/DX12GUIBackend.cpp:53`, `GUI/GUIModule.h:19` | 그리드는 GUI 모듈 안의 독립 컨트롤로 만든다(래퍼 수십 개를 늘리는 대신). 한글 글꼴이 선행(GUI BL-3). 여러 테이블은 한 창 안의 탭 |
| JGConsole | 시작 때 GameFrameWorks 만 연결한다. 모듈의 `HAutoConsoleCommand` 는 그 모듈이 **연결돼야** 등록된다. Asset 을 연결하면 `GAssetDatabase` 가 Content 전체(36MB 메시 포함)를 올린다 | `JGConsole/Main.cpp:13-16`, `Core/Misc/Module.h` `RegisterAutoConsoleCommands`, `Asset/AssetModule.cpp` | 셀프 테스트 본체는 Asset, JGConsole 명령 파일이 그것을 부른다(§10) |
| GFW | 게임 상태는 값 타입 + 손 직렬화 `IJsonable`. 네트워크 일치 검사 `RulesFingerprint` 는 등록 목록만 해시한다(데이터 내용 없음). 방향 문서는 "정의 에셋(JSON, 불변)" C3 · C4 를 계획했고, GFW TODO 는 콘텐츠 정의를 게임 영역으로 분류했다 | `GameMaster/State/GameplayComponentTable.h`, `GameMaster/GameMaster.cpp:371`, 방향 문서 §5-1, GFW `TODO.md` 게임 영역 표 | 엔진은 테이블 시스템을 주고, 규칙 상태에는 행 키만 둔다. 지문 연결은 GFW · Server 와 조정할 보류 항목(DT-H7) |

---

## 3. 사용자 결정 (권고 포함)

| ID | 결정 | 권고 | 대안 | 권고 이유 |
|---|---|---|---|---|
| DT-D1 | 스키마(열 정의)를 어디에 둘까 | **(B) 에셋 안 + C++ 바인딩 검사** | (A) C++ 구조체가 스키마(UE DataTable 방식) / (C) B + 스키마에서 C++ 행 구조체 코드 생성 | A 는 리플렉션 확장(구조체 · 타입 없는 값 핸들 · 파서)이 선행이고, HeaderTool · Core 를 건드려 모든 모듈이 다시 빌드된다. B 는 지금 JSON 위에서 바로 되고, 에디터가 게임 DLL 없이 동작하며, 엑셀 머리행처럼 열을 테이블에서 정한다. C 는 바인딩 반복이 실제로 귀찮아질 때 B 위에 얹는다(DT-H4) |
| DT-D2 | 파일 형식 | **`.jgasset`(내용 JSON)** | 별도 `.json` 확장자 | `.jgasset` 이 이미 JSON 이고 GUID · 토큰 경로 · 시작 로드가 그대로 된다. `.json` 은 `HAssetPath` · `GAssetDatabase` 가 확장자 하나(`JG_ASSET_FORMAT`)만 알아 에셋 시스템을 넓혀야 한다. 바깥 `.json` · `.csv` 는 나중에 가져오기 · 내보내기로(DT-H5) |
| DT-D3 | 코드 위치 | **Asset 모듈 `Source/Runtime/Asset/DataTable/`** | 새 런타임 모듈 `DataTable` | 의존이 Core 뿐인 범용 에셋 타입이다. 새 모듈이면 `module.json` · 에디터 엔진 모듈 목록 · 게임 템플릿 의존(GFW C-1) · JGConsole 연결이 같이 늘어난다 |
| DT-D4 | 1단계 열 타입 | **Bool · Int · Float · String · Enum · AssetRef · RowRef** | 목록(List) 열 포함 / 중첩 구조 셀 | 스프레드시트 셀에 맞는 것만. 목록 · 중첩은 셀 편집이 표가 아니게 된다 → 다른 테이블 참조(정규화)로 먼저 풀고, 필요하면 다음 단계(DT-H1) |
| DT-D5 | 에디터 한글 글꼴(GUI BL-3)을 이 작업에 포함할까 | **포함** | 영문 데이터만 | 지금 에디터 글꼴로는 한글 셀 · 설명이 보이지 않는다. 게임 UI 와 같은 정책(Content 글꼴이 있으면 그것, 없으면 `C:/Windows/Fonts/malgun.ttf`)으로 기본 글꼴에 한글을 합친다 |
| DT-D6 | 착수 범위 | **DT-0 ~ DT-4 전부**(선행 → 런타임 → 편집 모델 → 에디터 → 실제 입력 검증) | 런타임(DT-0 · DT-1)만 먼저 | 엑셀식 편집이 요청의 핵심이고, 편집 모델과 그리드는 런타임 위에서만 검증된다 |

**사용자 결정(2026-10-01):** D1 · D2 · D4 · D5 · D6 권고대로. **D3 은 권고와 다름 — GameFrameWorks 모듈**(`Source/Runtime/GameFrameWorks/Data/`). D4 는 7개 타입으로 시작하되 "나중에 리플렉션 이용해서 struct 도 지원하고 싶다"(→ TODO 보류 DT-H1 에 사용자 희망으로).

---

## 4. 구조

> DT-D3 결정으로 아래 그림 · 표의 "Asset" 은 **GameFrameWorks `Data/`** 로 읽는다. 실제 파일 목록은 §14-1.

```
JGEditor   JGDataTableEditor — 창 "Data Table Editor", 메뉴 Windows/Data Table Editor
             탭(열린 테이블) · 도구 막대 · 그리드 · 문제 목록 · 열 속성 · 새 테이블 대화상자
             그리드 원본(IGUIGridSource)을 구현하고, 그리드 이벤트를 문서 명령으로 바꾼다
   ↓
GUI        PGUIGrid — 데이터를 모르는 스프레드시트 컨트롤(ImGui 테이블 · 가상화 · 선택 · 키보드 · 셀 편집기)
           HGUI::SetClipboardText / GetClipboardText, 에디터 한글 글꼴(BL-3)
   ↓
Asset      DataTable/ — 런타임: JGDataTable(에셋) · 스키마 · 값 · 검증 · HDataTableView<T>
                        편집 모델: PDataTableDocument(명령 · 실행 취소 · 저장/되돌리기 · 바뀐 파일 감지) · TSV 클립보드
   ↓
Core       PJson 보강(파싱 오류 위치 · BOM · 값 종류 · 키 나열), 임시 파일 → 교체 저장
```

규칙: 런타임 · 편집 로직은 Asset, ImGui 는 GUI, 창 · 메뉴는 JGEditor. 사용자 결정(2026-09-30 "위젯은 JGEditor, 런타임은 순수 로직")과 같은 선이다.

| 모듈 | 파일(계획) | 내용 |
|---|---|---|
| Core | `FileIO/Json.h/.cpp` (추가만) | `PJson::ToObject` 오류 보고판(줄:칸, BOM 건너뜀) — `LoadObject` 만 새 함수로 바꾸고 기존 호출 11곳은 그대로. `PJsonData::GetValueType()`(이동 없음), `GetMemberKeys()` |
| Core | `FileIO/FileHelper.h/.cpp` (추가만) | `WriteAllTextAtomic`(같은 폴더 임시 파일 → `fs::rename` 교체) |
| Asset | `DataTable/DataTableTypes.h/.cpp` | `EDataTableColumnType`, `HDataTableValue`, 텍스트 ↔ 값, JSON ↔ 값 |
| Asset | `DataTable/DataTableSchema.h/.cpp` | `HDataTableColumn`, `HDataTableSchema` |
| Asset | `DataTable/DataTable.h/.cpp` | `JGDataTable`(JGCLASS, `JGAsset` 파생) — 읽기 · 쓰기 · 조회 · 지문 · 변경 알림 |
| Asset | `DataTable/DataTableView.h` | `HDataTableRowBinder<T>`, `HDataTableView<T>`(템플릿, 헤더만) |
| Asset | `DataTable/DataTableValidation.h/.cpp` | `HDataTableIssue`, `ValidateDataTable`, 참조 해석기(에셋 DB 또는 테스트용 가짜) |
| Asset | `DataTable/DataTableDocument.h/.cpp` | `PDataTableDocument`, 편집 명령, 실행 취소 |
| Asset | `DataTable/DataTableClipboard.h/.cpp` | TSV 인코딩 · 디코딩, 붙여넣기 계획 |
| Asset | `DataTable/DataTableSelfTest.h/.cpp`, `DataTableCommands.cpp` | 셀프 테스트 본체(내보낸 함수), 에디터 DevConsole 용 `datatable.validate` |
| Asset | `AssetDatabase.h/.cpp` (추가만) | 타입별 로드된 에셋 목록, 여러 에셋을 기다리는 도우미 |
| GUI | `Grid/GUIGrid.h/.cpp` | `PGUIGrid`, `IGUIGridSource`, `HGUIGridCell`, `HGUIGridEvent` |
| GUI | `Backends/DX12GUIBackend.cpp`, `GUI.h/.cpp` | 한글 글꼴, 클립보드 래퍼 — `HGUI::GetClipboardText/SetClipboardText` 는 게임 UI 글자 입력(ER-008, 진행현황 D-14, GUI 세션)도 추가할 예정이다. 먼저 생긴 것을 같이 쓰고 따로 만들지 않는다 |
| JGEditor | `Widgets/DataTableEditor.h/.cpp`, `JGEditor.cpp` | `JGDataTableEditor`, 메뉴 등록 |
| JGConsole | `DataTableCommands.cpp`, `JGConsole.module.json`(의존 `Asset` 추가) | `datatable.selftest` 가 Asset 의 셀프 테스트 함수를 부른다 |

---

## 5. 파일 형식

```json
{
    "JGObjectType": "JGDataTable",
    "JGObject": {
        "JGObject": { "Name": "SampleTable" },
        "JGAsset": { "_guid": "…", "AssetPath": { "AssetPath": "/JGGame/Data/SampleTable.jgasset" }, "Version": 0 },
        "JGDataTable": {
            "FormatVersion": 1,
            "KeyColumn": "Id",
            "Columns": [
                { "Name": "Count",   "Type": "Int",      "Default": 0, "Min": 0 },
                { "Name": "Rate",    "Type": "Float",    "Default": 1.0 },
                { "Name": "Enabled", "Type": "Bool",     "Default": true },
                { "Name": "Label",   "Type": "String",   "Default": "", "Description": "표시 이름" },
                { "Name": "Kind",    "Type": "Enum",     "EnumValues": [ "A", "B", "C" ], "Default": "A" },
                { "Name": "Mesh",    "Type": "AssetRef", "AssetClass": "JGStaticMesh", "Default": "" },
                { "Name": "Next",    "Type": "RowRef",   "Table": "/JGGame/Data/OtherTable", "Default": "" }
            ],
            "Rows": [
                { "Id": "Row_A", "Count": 3, "Rate": 1.5, "Enabled": true, "Label": "첫 행", "Kind": "B", "Mesh": "/JGGame/Meshes/Box", "Next": "Row_X" }
            ]
        }
    }
}
```

(실제 저장은 `PrettyWriter` 라 값마다 한 줄이다. 위는 읽기 쉽게 줄였다. `JGObject` · `JGAsset` 절은 지금 모든 에셋과 같다.)

1. 행 = 객체. 키는 `KeyColumn` 과 열 이름, 순서는 스키마 순서. **모든 셀을 적는다**(기본값이어도). 기본값을 바꿔도 이미 있는 데이터의 뜻이 바뀌지 않는다.
2. 행 순서 = 파일 순서 = 런타임 순회 순서. 에디터의 보기 정렬은 파일 순서를 바꾸지 않는다("정렬을 행 순서로 적용" 명령만 바꾼다).
3. 같은 내용이면 저장 바이트가 같다(넣는 순서 고정, 실수는 rapidjson 최단 왕복 표기). 셀 하나 수정 = diff 한 줄, 다른 셀 수정끼리는 git 이 깨끗이 병합한다.
4. 인코딩 UTF-8(BOM 없이 쓴다, 읽을 때 BOM 은 건너뜀). 파일 · 폴더 이름은 ASCII.
5. 저장은 같은 폴더 임시 파일에 쓴 뒤 교체한다(쓰다가 죽어도 원본이 남는다).
6. 테이블 복제는 새 GUID 를 준다(같은 GUID 는 `GAssetDatabase` 가 거부 — GameModule GM-A1).
7. `FormatVersion` 이 오르면 로더가 옛 버전을 읽어 올린다.
8. 열 이름 · 키는 대소문자를 구분한다.
9. 새 테이블은 `JGAsset` 의 `AssetPath` 를 토큰 경로로 채운다(`Sample.jgasset` 의 `"(null)"` 경고 D-10 을 되풀이하지 않는다).

---

## 6. 열 타입과 값 규칙

| 타입 | JSON 값 | 텍스트 입력(셀 · 붙여넣기) | C++ 바인딩 | 열 옵션 | 검증 |
|---|---|---|---|---|---|
| Bool | `true` / `false` | `true/false/1/0`(대소문자 무시, 엑셀 `TRUE/FALSE`) | `bool` | — | — |
| Int | 정수(64비트로 보관) | 부호 + 숫자만(소수점 거부) | `int32`(범위 검사) · `int64` | `Min` · `Max` | 범위 밖 = 경고 |
| Float | 실수(double 로 보관) | `.` 소수점, NaN · Inf 거부 | `float32` · `float64` | `Min` · `Max` | 범위 밖 = 경고. 규칙 계산(결정론)에는 Int 를 쓴다(방향 문서 규칙 3 "수치는 정수") |
| String | 문자열 | 그대로(줄바꿈 허용) | `PString` · `PName` | — | — |
| Enum | 이름 문자열 | 목록 중 하나. 대소문자를 무시하고 찾아 정식 표기로 저장 | `JGENUM` 열거형(이름 → 값) · `PName` | `EnumValues`(테이블 안 목록) 또는 `EnumType`(리플렉션 열거형 이름 — 그 모듈이 올라와 있어야 목록이 보인다) | 목록 밖 = 오류 표시(목록을 줄인 경우) |
| AssetRef | 토큰 경로 문자열(`/JGGame/…`) | 경로 | `HAssetPath` | `AssetClass`(고르기 목록 거르기) | 없는 에셋 = 경고. 빈 값 = 없음 |
| RowRef | 행 키 문자열 | 키 | `PName` | `Table`(비우면 같은 테이블) | 없는 키 = 경고. 빈 값 = 없음 |

키 열: 첫 열 고정. 비어 있으면 안 되고 고유하며 공백 · 제어 문자를 쓰지 않는다. **빈 키 · 중복 키는 오류(저장 막음)** — 런타임 색인이 깨진다. 나머지 문제는 경고(저장 가능, 만드는 중인 데이터를 허용).
타입 오류(Int 열에 "abc")는 입력 · 붙여넣기 때 거부해서 문서에 들어가지 않는다.
키는 안정적인 ID 로 다룬다. 바꾸면 다른 테이블의 참조와 저장 파일이 깨질 수 있다(함께 갱신은 DT-H6).

---

## 7. 런타임 API (스케치)

```cpp
JG_DECLARE_MULTICAST_DELEGATE(POnDataTableChanged, const JGDataTable&);

JGCLASS()
class ASSET_API JGDataTable : public JGAsset
{
	JG_GENERATED_SIMPLE_BODY   // JGTexture 처럼 직렬화는 직접 쓴다
public:
	const HDataTableSchema& GetSchema() const;
	int32 GetRowCount() const;
	const PName& GetRowKey(int32 inRowIndex) const;
	int32 FindRowIndex(const PName& inKey) const;                  // 없으면 -1
	const HDataTableValue& GetValue(int32 inRowIndex, int32 inColumnIndex) const;

	uint64 GetRevision() const;                                    // 내용이 바뀔 때마다 +1
	uint64 ComputeContentHash() const;                             // 스키마 + 행. 서식 · 공백과 무관
	POnDataTableChanged OnChanged;                                 // 에디터 저장 · 다시 읽기 뒤, 메인 스레드

	// 헤드리스 · 테스트: AssetDatabase 없이 JSON 글에서 만든다
	static PSharedPtr<JGDataTable> FromJsonText(const PString& inText, HList<HDataTableIssue>* outIssues);

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
};
```

```cpp
// 게임 코드: 필요한 열만 멤버 포인터로 묶는다 (예시 이름은 중립)
struct HSampleRow
{
	int32       Count = 0;
	float32     Rate  = 1.0f;
	PString     Label;
	ESampleKind Kind  = ESampleKind::A;   // JGENUM
	HAssetPath  Mesh;
	PName       Next;

	static void BindColumns(HDataTableRowBinder<HSampleRow>& binder)
	{
		binder.Bind("Count", &HSampleRow::Count);
		binder.Bind("Rate",  &HSampleRow::Rate);
		binder.Bind("Label", &HSampleRow::Label);
		binder.Bind("Kind",  &HSampleRow::Kind);
		binder.Bind("Mesh",  &HSampleRow::Mesh);
		binder.Bind("Next",  &HSampleRow::Next);
	}
};

HDataTableView<HSampleRow> view;
HList<HDataTableIssue> issues;
if (view.Bind(table, &issues) == false)
{
	// 열 없음 · 타입 불일치: 어느 열이 무엇을 기대했는지 issues 와 로그에
}
const HSampleRow* row = view.Find(PName("Row_A"));
for (int32 i = 0; i < view.GetCount(); ++i)
{
	const PName& key = view.GetKey(i);
	const HSampleRow& value = view.Get(i);
}
```

- `Bind` 가 한 번에 모든 행을 `HList<T>` 로 바꾼다(행 순서). `Find` 는 키 색인(해시)으로 찾고, 순회는 늘 목록 순서다(해시 맵을 순회하지 않는다).
- 바인딩이 받는 C++ 타입 → 기대하는 열 타입: `bool`→Bool, `int32/int64`→Int, `float32/float64`→Float, `PString`→String, `PName`→String · Enum · RowRef, `JGENUM` 열거형→Enum(이름이 열거형에 없으면 오류), `HAssetPath`→AssetRef. 없는 열 · 다른 타입은 실패, 테이블에만 있는 열은 무시.
- 뷰는 `GetRevision()` 을 기억한다. 테이블이 바뀌면 `IsUpToDate()` 가 false — 쓰는 쪽이 `OnChanged` 에서 다시 `Bind`.
- 로드: 테이블은 시작 때 다른 에셋처럼 비동기로 올라온다. 쓰는 쪽은 `LoadAssetAsync(path, callback)` 콜백 뒤에 `Bind`(이미 올라와 있으면 바로 불린다). 여러 테이블을 다 기다리는 작은 도우미를 DT-1-7 에서 `GAssetDatabase` 에 더한다.
- 규칙 상태(GameMaster)에는 행 키만 두고 값은 뷰에서 읽는다(방향 문서 §4 "불변 데이터는 정의 에셋에").
- 스레드: 로드 스레드는 처음 읽을 때만 쓰고, 등록 뒤 읽기 · 편집 반영은 메인 스레드뿐이라 잠금이 없다.

---

## 8. 편집 모델 — `PDataTableDocument` (Asset, UI 없음)

| 기능 | 설계 |
|---|---|
| 작업 사본 | 문서는 테이블 내용(스키마 + 행)의 사본을 고친다. 저장 = 검증 → 파일에 쓰기(임시 → 교체) → 살아 있는 에셋에 내용 넣기 → `Revision+1` · `OnChanged` |
| 명령 | 셀 값 설정(범위) · 행 삽입 · 삭제 · 이동 · 복제 · 키 바꾸기 · 열 추가 · 삭제 · 이름 · 순서 · 타입 · 옵션 바꾸기 · 정렬을 행 순서로 적용. 사용자 동작 하나 = 실행 취소 하나(붙여넣기 100셀도 하나) |
| 실행 취소 | 명령마다 바뀐 셀의 옛 값 · 새 값만 기록(전체 복사 아님). 한도 500. 저장해도 기록은 남고, "저장 안 됨"은 기록 위치 ≠ 저장한 위치로 판단 |
| 타입 바꾸기 | 변환 실패 셀은 기본값 + 문제 목록. 되돌리기로 원래대로 |
| 검증 | 편집 뒤 다시 계산(1만 행 기준 시간은 DT-4 에서 측정). 결과 `HDataTableIssue { 심각도, 행, 열, 메시지 }` |
| 클립보드(TSV) | 엑셀 · 구글 시트와 같은 규칙: 열 = 탭, 행 = 줄바꿈(CRLF 도 읽음, 끝 줄바꿈 무시). 탭 · 줄바꿈 · `"` 가 든 셀은 `"…"` 로 감싸고 안의 `"` 는 `""` |
| 붙여넣기 | 초점 셀부터 덮어쓴다. **모든 셀이 타입 검사를 통과해야** 들어간다(하나라도 실패하면 아무것도 바꾸지 않고 실패 셀 목록을 보인다). 한 셀을 복사해 범위에 붙이면 범위를 채운다. 블록이 키 열부터 시작하면 마지막 행 뒤는 새 행(키 = 붙인 값)으로 붙고, 아니면 행이 모자라 거부. 열이 넘치면 거부 |
| 보기 | 정렬(열 머리) · 필터(글자 포함, 대소문자 무시)는 보기 순서표만 바꾼다. 파일 순서는 명령으로만 바뀐다 |
| 바뀐 파일 | 열린 문서의 파일 시각을 1초마다 본다. 저장 안 된 변경이 없으면 다시 읽고(선택 유지), 있으면 알림 줄 "디스크 파일이 바뀌었습니다 — 다시 읽기(내 변경 버림) / 그대로(저장하면 덮어씀)". 텍스트 편집기 · git · 다른 에이전트가 고친 파일을 위해 |
| 모르는 키 | 파일 행에 스키마에 없는 키가 있으면(손으로 고친 파일) 경고 "열 X — 저장하면 사라짐", 저장 때 한 번 확인 |

---

## 9. 에디터 UI

### 9-1. `PGUIGrid` (GUI 모듈) — 데이터를 모르는 그리드

```cpp
enum class EGUIGridEditor { None, Text, Checkbox, Combo };

struct HGUIGridCell
{
	PString Text;
	bool    bAlignRight = false;   // 숫자
	bool    bMuted      = false;   // 기본값과 같음 → 흐리게
	bool    bError      = false;   // 문제 셀 → 빨간 테두리
	PString Tooltip;
};

class IGUIGridSource
{
public:
	virtual ~IGUIGridSource() = default;
	virtual int32   GetRowCount() const = 0;
	virtual int32   GetColumnCount() const = 0;                       // 열 0 = 키 열(고정)
	virtual PString GetColumnHeader(int32 inColumn) const = 0;
	virtual void    GetCell(int32 inRow, int32 inColumn, HGUIGridCell& outCell) const = 0;
	virtual EGUIGridEditor GetEditor(int32 inRow, int32 inColumn) const = 0;
	virtual void    GetComboItems(int32 inRow, int32 inColumn, HList<PString>& outItems) const {}
};

class GUI_API PGUIGrid : public IMemoryObject
{
public:
	// 그리고, 이번 프레임에 일어난 일(편집 확정 · 단축키 · 선택 바뀜)을 돌려준다. 원본은 그 뒤에 고친다(즉시 모드 방식).
	void Draw(const PString& inId, const IGUIGridSource& inSource, HList<HGUIGridEvent>& outEvents);
	const HGUIGridSelection& GetSelection() const;   // 기준 셀 + 초점 셀(사각 범위)
	void SetSelection(const HGUIGridSelection& inSelection);
	void ScrollToCell(int32 inRow, int32 inColumn);
};
```

- ImGui 테이블: 머리 행 + 키 열 고정(`TableSetupScrollFreeze(1, 1)`), 열 크기 조절 · 숨기기, `ImGuiListClipper` 로 보이는 행만 그린다(셀 글자도 보이는 칸만 만든다).
- 선택: 클릭 · Shift+클릭 · 끌기 = 사각 범위, 행 번호 = 행 전체, 열 머리 = 열 전체(정렬 · 열 명령은 머리 오른쪽 클릭 메뉴).
- 그리드에 초점이 있을 때 ImGui 자체 키보드 이동(NavEnableKeyboard)과 겹치지 않게 키를 그리드가 먼저 처리한다.
- 편집기: 텍스트(한 줄, 긴 글은 여러 줄 팝업) · 체크박스(Space) · 콤보(Enum · RowRef · AssetRef, 글자로 거르기).
- 이벤트 방식이라 그리드는 실행 취소 · 타입 검사를 모른다. 데이터 테이블 말고 다른 표(로그 · 통계)에도 쓸 수 있다.

| 키 | 동작 |
|---|---|
| ← → ↑ ↓ / Shift+방향키 | 이동 / 범위 넓히기 |
| Tab · Shift+Tab / Enter · Shift+Enter | 오른쪽 · 왼쪽 / 아래 · 위 (편집 중이면 확정한 뒤 이동) |
| 글자 입력 | 셀 내용을 지우고 편집 시작(엑셀처럼) |
| F2 · 두 번 클릭 | 내용을 둔 채 편집 |
| Esc | 편집 취소 |
| Delete | 선택 범위를 기본값으로 |
| Ctrl+C · Ctrl+X · Ctrl+V | 복사 · 잘라내기 · 붙여넣기(TSV) |
| Ctrl+Z · Ctrl+Y | 실행 취소 · 다시 실행 |
| Ctrl+S | 저장 |
| Ctrl+F | 필터 칸으로 |
| Ctrl+Shift+= · Ctrl+- | 행 삽입 · 행 삭제 |

### 9-2. `JGDataTableEditor` (JGEditor) — 창 "Data Table Editor"

- 메뉴 `Windows/Data Table Editor`. 위젯은 클래스마다 하나라(`GUIModule.h:19`) 여러 테이블은 창 안의 탭이다(탭 이름 뒤 `*` = 저장 안 됨).
- 위 도구 막대: 테이블 고르기(로드된 `JGDataTable` 목록, 글자로 거르기) · 새 테이블 · 저장 · 되돌리기 · 실행 취소 / 다시 실행 · 행 추가 · 필터 칸.
- 가운데: 그리드. 열 머리 = 이름 + 타입(예 `Count · Int`), 설명은 툴팁. 기본값과 같은 셀은 흐리게, 문제 셀은 빨간 테두리 + 툴팁.
- 오른쪽(접을 수 있음): 열 속성 — 이름 · 타입 · 기본값 · 옵션 · 설명. 열 머리를 고르면 보인다.
- 아래: 문제 목록(오류 · 경고 수, 누르면 그 셀로) · 상태 줄(행 수 · 보이는 행 수 · 선택 범위 · 저장 안 됨 · 파일 경로).
- 새 테이블: Content(`/JGGame/` 또는 `/JGEngine/`) · 폴더 · 이름(ASCII) · 키 열 이름(기본 `Id`) → 파일 생성 → `LoadAssetAsync` → 탭으로 연다.
- 탭을 닫거나 다른 테이블로 바꿀 때 저장 안 된 변경이 있으면 확인한다. 에디터 종료 때의 확인 · 복구 파일은 보류(DT-H9 — GUI 에 닫기 요청 훅이 없다).

---

## 10. 검증

| 단계 | 내용 | 통과 기준 |
|---|---|---|
| V1 헤드리스 `JGConsole datatable.selftest` | 값 텍스트 ↔ 값 왕복 · JSON 읽기 · 쓰기 왕복(다시 저장한 바이트 동일) · BOM · 파싱 오류 줄:칸 · 빈 키 · 중복 키 · 모르는 키 · Enum · RowRef · AssetRef 검증(가짜 해석기) · 바인딩 성공 · 열 없음 · 타입 불일치 · 편집 명령 무작위 열 → 전부 되돌리면 원본 바이트 · TSV(따옴표 · 탭 · 줄바꿈 · CRLF · 끝 줄바꿈 · 엑셀 표본) · 붙여넣기 규칙(키 열 시작이면 행 추가 · 아니면 거부 · 한 셀 채우기 · 타입 실패면 무변경) · 지문 안정 · 변화 · 1만 행 시간 측정 | 전부 통과, 숫자는 결과 txt 로 |
| V2 에디터 실제 입력 | 엔진 에디터와 게임 프로젝트 에디터에서 테이블 생성 → 열 추가 → 입력 · 이동 · 범위 선택 → 엑셀 형식 클립보드 붙여넣기(PowerShell `Set-Clipboard`) → 실행 취소 / 다시 → 저장 → 파일 diff 확인 → 재시작 뒤 같은 내용 → 바깥 수정 감지 → 한글 표시 · IME 입력 | 캡처 + 파일 diff + 로그 오류 0 |
| V3 회귀 | `gmtest`, `net.test all`, `console.selftest`, `gameui.selftest`, 런처 60초 | 기준선과 같음 |

- 셀프 테스트 실행 경로: JGConsole 은 GameFrameWorks 만 연결하고, Asset 을 연결하면 `GAssetDatabase` 가 Content 전체를 올린다. 그래서 셀프 테스트 본체는 Asset(`DataTableSelfTest.cpp`, 내보낸 함수)에 두고 JGConsole 명령 파일 `DataTableCommands.cpp` 가 부른다(JGConsole 의존에 `Asset` 추가). Asset DLL 은 시작 때 리플렉션 codeGen 이 `Link_Module` 하므로 연결 없이 함수를 부를 수 있다 — **구현 첫 단계에서 확인**한다.
- 도구 주의(이전 세션 기록): 이 PC 에서 crashwalk 가 창을 닫지 못한다(진행현황 6절, WM_CLOSE 를 직접 보낸다). ImGui 백엔드는 실제 커서로 뷰포트를 정해 PostMessage 클릭이 먹지 않는다(DevConsole 6-2) → 실제 입력은 SendInput 류 또는 워크트리 전용 스니펫으로.

---

## 11. 단계

| 단계 | 항목 | 크기 | 완료 기준 |
|---|---|---|---|
| DT-0 선행 | 0-1 Core JSON: 오류 보고판 `ToObject`(줄:칸 · BOM) — `LoadObject` 만 전환, `GetValueType` · `GetMemberKeys` / 0-2 `WriteAllTextAtomic` / 0-3 에디터 한글 글꼴(GUI BL-3) + 클립보드 래퍼 | 小 | 기존 회귀 통과 + 한글 문자열 표시 캡처 + 아틀라스 크기 · 시작 시간 기록 |
| DT-1 런타임 | 1-1 값 · 타입 / 1-2 스키마 / 1-3 `JGDataTable` 읽기 · 쓰기 · `FromJsonText` / 1-4 조회 · 지문 · 변경 알림 / 1-5 `HDataTableView<T>` / 1-6 검증 / 1-7 `GAssetDatabase` 연동(타입별 목록 · 새 테이블 등록 · 여러 에셋 기다리기) / 1-8 셀프 테스트 · 명령 | 中 | V1 의 런타임 부분 |
| DT-2 편집 모델 | 2-1 문서 · 저장 · 되돌리기 · 바뀐 파일 감지 / 2-2 명령 · 실행 취소 / 2-3 TSV · 붙여넣기 / 2-4 보기 정렬 · 필터 | 中 | V1 의 편집 부분 |
| DT-3 에디터 | 3-1 `PGUIGrid` / 3-2 `JGDataTableEditor` · 메뉴 · 탭 / 3-3 열 속성 · 새 테이블 / 3-4 문제 목록 · 상태 줄 | 大 | V2 |
| DT-4 검증 · 문서 | V2 · V3, 사용 안내(게임에서 테이블을 만들고 묶는 법), 보고서 | 小 | 증적 txt · 캡처 |

크기: 小(며칠) · 中(1~2주) · 大(수 주) — 1인 기준 거친 추정(방향 문서와 같은 기준).

---

## 12. 하지 않는 것 · 보류

하지 않는 것: 수식 · 셀 서식 · 차트 · 조건부 서식, `.xlsx` 직접 읽기(엑셀과는 TSV 클립보드로 주고받는다), 테이블 안 스크립트.

아래는 설계 시점의 보류 목록이다. 살아 있는 목록은 `../TODO_DataTable.md` 보류 표.

| ID | 보류 항목 | 다시 볼 조건 |
|---|---|---|
| DT-H1 | 목록(List) 열 · 중첩 구조 셀(팝업 편집) | 다른 테이블 참조로 풀기 어려운 데이터가 실제로 나올 때 |
| DT-H2 | 행 상속(원본 + 델타, 방향 문서 C4) | 비슷한 행을 복사해 따로 유지하는 일이 반복될 때 |
| DT-H3 | 여러 파일을 한 테이블로(묶음별 분할) | 한 파일로 다루기 어렵게 커지거나 협업 충돌이 잦을 때 |
| DT-H4 | 스키마 → C++ 행 구조체 코드 생성(DT-D1 의 C), 행 타입 등록으로 에디터가 코드 쪽 열을 보여 주기 | 바인딩 반복 · 열 이름 오타가 실제 문제일 때 |
| DT-H5 | `.csv` · `.json` 가져오기 · 내보내기 명령 | 바깥 도구로 대량 수정이 필요할 때 |
| DT-H6 | 키 바꾸기 → 다른 테이블 RowRef 함께 갱신, 참조 찾기 | 키 변경이 잦아질 때 |
| DT-H7 | 테이블 내용 지문을 `RulesFingerprint` 에 넣기(GFW · Server 조정) | 네트워크 게임이 테이블을 규칙에 쓰기 시작할 때 |
| DT-H8 | 실행 중인 게임에 데이터 즉시 반영하는 정책(월드 재시작 E-H3 와 함께) | 테이블을 고치며 바로 게임에서 보는 일이 반복될 때 |
| DT-H9 | 에디터 종료 때 저장 안 된 변경 확인 · 복구 파일 | 변경을 잃는 일이 실제로 생길 때(GUI 에 닫기 요청 훅 필요) |
| DT-H10 | 지역화 문자열 테이블(언어별 열) | 지역화 시스템을 시작할 때 |
| DT-H11 | 열린 테이블 목록을 레이아웃과 함께 저장 | 재시작마다 다시 여는 게 불편할 때 |
| DT-H12 | 한글 IME 로 바로 입력할 때 조합 시작에서 편집 시작 | V2 에서 첫 글자 처리가 실제로 어색할 때(그 전에는 F2 · Enter 로 편집 시작) |

---

## 13. 위험 · 공유 파일

| 위험 | 대응 |
|---|---|
| 공유 파일: `Json.h/.cpp`(Graphics 5-30 미커밋), `FileHelper.cpp` · `AssetDatabase.cpp`(GameModule R7 · GM-A1 미커밋), `GUI.h/.cpp` · `DX12GUIBackend.cpp`(GUI · Memory · DevConsole · 게임 UI 미커밋), `JGEditor.cpp`, `JGConsole/Main.cpp` 계열 | 추가만 Edit, 통째 덮어쓰기 금지. 큰 커밋(D-1) 전이면 격리 워크트리에서 빌드 · 검증한 뒤 반영(Graphics 현황 §5 "동시 세션 중 빌드"). 새 파일을 만들면 바로 PreBuild |
| 게임 UI 글자 입력(ER-008, D-14)과 GUI 의 같은 영역 | 그 작업이 `HGUI` 에 편집 키 · 보조 키 · 입력 글자 · 키 소유권 · IME 위치 · 클립보드를 추가한다(검토서 `GameFrameWorks/Files/ER-007_ER-008_검토_2026-10-01.md` §3, 키 소유권 · IME 위치는 `imgui_internal.h` 를 `GUI.cpp` 안에서만). `PGUIGrid` 는 GUI 모듈 안이라 ImGui 를 직접 쓰지만, 키 소유권 · IME 위치 처리는 같은 도우미를 쓰고 따로 만들지 않는다. 착수 때 GUI 세션 진행 상태를 확인한다 |
| `PJsonData` 읽기가 값을 옮긴다 | 로더는 키마다 한 번만 읽고, 종류는 `GetValueType` 으로 먼저 묻는다. V1 이 같은 파일 두 번 로드 · 왕복으로 확인 |
| `ToObject` 의미 변경의 파급(호출 11곳 — 빌드 도구 · 네트워크 메시지 포함) | 기존 함수는 그대로 두고 오류 보고판을 새로 만들어 `LoadObject` 만 바꾼다 |
| 한글 글꼴 아틀라스(한글 음절 11,172자) | 기본 글꼴(작은 크기)에만 합친다(큰 글꼴은 숫자용). 아틀라스 크기 · 시작 시간을 측정하고, 문제면 범위를 줄인다 |
| 한글 IME 로 "바로 입력" | 조합 중 글자는 OS IME 창에 보이고 확정된 글자가 WM_CHAR 로 온다. 첫 글자가 어색하면 F2 · Enter 로 편집을 시작하도록 안내(DT-H12) |
| 큰 테이블 | 보이는 행만 그리고, 셀 글자도 보이는 칸만 만든다. 1만 행에서 측정 |
| 에디터 종료 때 저장 안 된 변경 | 탭 닫기 확인 + 탭 `*` 표시. 종료 훅 · 복구 파일은 DT-H9 |
| JSON 병합 충돌 | 값 한 줄 · 순서 고정이라 다른 셀 수정끼리는 깨끗이 병합된다. 같은 자리 행 추가는 충돌 — 일반 텍스트 병합으로 푼다 |

---

## 14. 구현 결과 (2026-10-01)

격리 워크트리(`4c8ac73` + 그때 메인 트리 미커밋분 스냅샷)에서 구현 · 빌드 · V1~V3 검증 → 메인 트리(`736ddbe`)에 3-way 로 반영 → 메인 PreBuild · 전체 빌드 · 헤드리스 회귀. 증적 `2026-10-01_datatable_verify.txt`(같은 폴더), 캡처 `2026-10-01_datatable_v2_*.png` 10장.

### 14-1. 파일

| 모듈 | 파일 | 내용 |
|---|---|---|
| Core | `FileIO/Json.h/.cpp` (추가만) | `EJsonValueType`, `PJsonData::GetValueType()` · `GetMemberKeys()`(값을 옮기지 않음), `PJson::ToObjectWithError`(BOM 건너뜀, 오류 `"line L, column C: 이유"`, 실패하면 빈 객체). 기존 `ToObject` 와 호출 11곳은 그대로 |
| Core | `FileIO/FileHelper.h/.cpp` (추가만) | `WriteAllTextAtomic`(`<path>.tmp` 에 쓰고 `fs::rename`, 실패하면 임시 파일 지움) |
| Core | `Object/ObjectGlobalSystem.h` | `LoadObject` 만 `ToObjectWithError` 로 — 깨진 에셋은 `Fail Load at <path>, JSON line L, column C: …` |
| GFW | `Data/DataTableTypes.h/.cpp` | `EDataTableColumnType`(7개 + Count), `HDataTableValue`(Bool · Int(int64) · Float(float64) · Text), 이름 ↔ 타입, ASCII 소문자 · 포함 검색, `DataTableSameText` |
| GFW | `Data/DataTableSchema.h/.cpp` | `HDataTableColumn`(텍스트 ↔ 값 `ParseText/FormatText`, 형식 바꾸기 `ConvertFrom`, JSON 값 읽기 · 쓰기, 타입에 맞는 옵션만 저장), `HDataTableSchema`, 열 이름 · 키 규칙 |
| GFW | `Data/DataTableContent.h/.cpp` | 형식 v1 읽기 · 쓰기(문제를 모아 보고: 모르는 키 "저장하면 사라짐", 빠진 값 · 다른 타입 → 기본값), `HDataTableIssue`, `ComputeDataTableContentHash`(FNV-1a 64) |
| GFW | `Data/DataTableValidation.h/.cpp` | `IDataTableReferenceResolver`, `ValidateDataTableContent` — 오류: 빈 · 중복 · 잘못된 키, 열 이름 / 경고: 범위 · Enum · RowRef · AssetRef · 없는 테이블(열마다 50건까지) |
| GFW | `Data/DataTable.h/.cpp` | `JGDataTable : JGAsset`(JGCLASS) — 조회 · 키 색인 · `GetRevision` · `OnChanged` · `ComputeContentHash` · `InitializeNew` · `ApplyContent` · `FromJsonText/LoadFromFile/ToJsonText/SaveToFile` |
| GFW | `Data/DataTableView.h` | `HDataTableRowBinder<T>` · `HDataTableView<T>`(헤더만). 받는 멤버: `bool` · `int32`(범위 검사) · `int64` · `float32/64` · `PString` · `PName` · `HAssetPath` · 리플렉션 열거형 |
| GFW | `Data/DataTableDocument.h/.cpp` | `PDataTableDocument`(작업 사본 · 편집 10종 · 되돌리기 500 · 저장 · 다시 읽기 · 바뀐 파일 감지), `ApplyDataTableEdit`(편집마다 역편집을 돌려준다) |
| GFW | `Data/DataTableClipboard.h/.cpp` | TSV 인코딩 · 디코딩(엑셀 · 구글 시트 규칙), 복사 · 지우기 · 붙여넣기 계획 |
| GFW | `Data/DataTableAssets.h/.cpp` | 올라온 테이블 · 에셋 경로 목록(에셋 DB 를 읽기만), 새 테이블 토큰 경로(ASCII), 에셋 DB 참조 해석기 |
| GFW | `Data/DataTableSelfTest.cpp` | 콘솔 명령 `datatable.selftest`(127 검사) · `datatable.validate [-path=<토큰 또는 파일 경로>]` |
| GUI | `Grid/GUIGrid.h/.cpp` | `PGUIGrid`, `IGUIGridSource`, `HGUIGridCell` · `HGUIGridSelection` · `HGUIGridEvent`(17종). 데이터를 모르는 그리드 |
| GUI | `GUI.h/.cpp` (추가만, 끝에 한 묶음) | `BeginDocumentTabItem` · `OpenPopup` · `BeginPopup` · `BeginPopupModal` · `EndPopup` · `CloseCurrentPopup` · `MenuItem` · `SetNextItemWidth` · `BeginDisabled/EndDisabled` · `TextWrapped` · `SetKeyboardFocusHere` · `SameLineAt`. 클립보드 · 콤보는 게임 UI(ER-008)가 먼저 만든 것을 썼다 |
| GUI | `Backends/DX12GUIBackend.cpp` | 기본 글꼴에 한글 합침(GUI BL-3): `Content/Fonts/EditorKorean.ttf` → 없으면 `C:/Windows/Fonts/malgun.ttf` → 없으면 경고 한 줄. 14px, 큰 글꼴(26px)에는 합치지 않음 |
| JGEditor | `Widgets/DataTableEditor.h/.cpp`, `JGEditor.cpp` | `JGDataTableEditor`(창 "Data Table Editor"), 메뉴 `Windows/Data Table Editor` |

### 14-2. 설계와 달라진 점

| 설계 | 실제 | 이유 |
|---|---|---|
| Asset 모듈 `DataTable/` | GameFrameWorks `Data/` | 사용자 결정 DT-D3. 그래서 JGConsole 의존 추가 · `DataTableCommands.cpp` 가 필요 없다(JGConsole 이 GFW 를 연결하고, 명령은 GFW 의 `HAutoConsoleCommand`) |
| `GAssetDatabase` 에 타입별 목록 · 여러 에셋 기다리기 추가 | Asset 은 고치지 않음 | 목록은 GFW `GetLoadedDataTables` · `GetLoadedAssetPaths` 가 공개 멤버 `_assetsByAssetPath` 를 읽는다. 새 테이블은 파일을 만든 뒤 `LoadAssetAsync` 로 등록. 기다리기 도우미는 쓰는 곳이 없어 만들지 않았다 |
| 그리드 고정 = 키 열 1개 | 행 번호 열 + 키 열(`ScrollFreeze(2, 1)`) | 행 번호(파일 순서)가 정렬 · 필터 중에도 보여야 한다 |
| 탭 이름 뒤 `*` = 저장 안 됨 | ImGui 문서 탭 점(`•`) | `ImGuiTabItemFlags_UnsavedDocument` 표준 표시 |
| 형식 바꾸기 | 확인 없이 적용, 바뀌지 않는 값은 기본값 + 빨간 상태 줄 "N value(s) could not convert … (Ctrl+Z to undo)" | 되돌리기 한 번으로 원래대로(설계 §8 과 같음) |
| 객체 이름 | 테이블의 `JGObject.Name` 은 형식 이름(`"JGDataTable"`) | `JGObject::ReadJson` 이 이름을 되살리지 않아, 에셋 이름을 쓰면 다시 저장한 바이트가 달라진다 |
| 행 이동 | 보기 정렬 · 필터 중에는 막음("Rows move only in file order") | 보기 순서와 파일 순서가 다를 때 "위로"의 뜻이 모호하다 |
| UI 글 | 영어 | DevConsole 규칙. 한글 글꼴 범위 밖 글자(`—` 등)는 `?` 로 나온다 |

### 14-3. 함정 (다음 작업자용)

| 함정 | 대응 |
|---|---|
| `PString ==` 는 문자열 표 ID 비교 — 기본 생성한 빈 `PString` ≠ `PString("")` | 값 · 열 · 행 비교는 `DataTableSameText`(원문 비교) |
| `PString::ToLower` 는 바이트마다 `::tolower` — 디버그 CRT 가 UTF-8 바이트(음수 char)에서 assert | `DataTableToLowerAscii`(ASCII 만 바꿈) |
| `JGEnum::GetIndexByEnumName` 은 없을 때 -1 이 아니라 원소 수, `GetValueByEnumName` 은 0..N 연속 열거형에서만 맞다 | 바인더는 `GetEnumNameByIndex(index) != name` 으로 없음을 판정하고 `GetEnumNameByValue` 로 왕복 확인 |
| `JGENUM` 항목은 `JGENUMMETA()` 가 있어야 목록에 나온다 | 셀프 테스트는 메타가 있는 `ETextureFilterMode` 를 쓴다 |
| JGHeaderTool 은 코드 **문자열 안**의 `JGENUM` 같은 토큰도 선언으로 읽어 generation 파일을 깨뜨린다 | 문자열 · 주석에 리플렉션 토큰을 쓰지 않는다. 깨진 generation 파일은 지우고 PreBuild |
| `PJsonData::FindMember/GetData` 는 값을 옮긴다 | 키마다 한 번만 읽고, 종류는 `GetValueType` 으로 먼저. float64 읽기는 `IsDouble` 이 필요해 정수 표기는 Int 경로로 |
| ImGui 팝업은 연 곳과 같은 ID 범위에서 `Begin` 해야 한다 | 탭 내용 안에서 요청만 표시하고, 다음 그리기에서 창 수준에서 `OpenPopup`(저장 확인 창) |
| dllexport 클래스는 복사 연산을 모두 만들어 `HSTLUniquePtr` 목록에서 C2280 | 위젯의 복사 생성 · 대입을 `delete` |
| `HGUI::SameLine(x)` 의 x 는 간격 | 창 왼쪽 기준 위치는 `SameLineAt` |
| ImGui Win32 백엔드는 Ctrl · Shift 를 `GetKeyState` 로 읽고, 멀티 뷰포트에서 마우스가 올라간 뷰포트를 실제 커서로 정한다 | PostMessage 자동 입력은 `AttachThreadInput` + `SetKeyboardState`(뒤에 keyup), 워크트리 전용 `JG_UITEST` 훅(`imgui_impl_win32.cpp`, **메인에 넣지 않음**). 실제 마우스 · 키보드에는 영향 없음 |
| `%TEMP%` 아래 워크트리 빌드는 MSB8029 로 헤더 변경을 못 본다 | 헤더를 고치면 `-t:Rebuild` |

### 14-4. 검증

| 단계 | 결과 |
|---|---|
| V1 `datatable.selftest` | **OK (127/127)** — 워크트리 · 메인 트리 모두. 1만 행 · 3,338 KB JSON: 쓰기 845 ms · 읽기 763 ms · 검증 52 ms · 바인드 77 ms(DevelopEngine 구성, 워크트리 마지막 실행) |
| V2 에디터 실제 입력(워크트리, 메시지 주입) | 새 테이블 대화상자 → 파일 생성(`AssetPath` 토큰 경로) · 열 추가 · 저장 · 바깥 수정 자동 다시 읽기 · 재시작 뒤 레이아웃으로 창 복원 · 열기 목록 · 행 추가 · 엑셀 형식 TSV 붙여넣기 3×7(한글 · `""` 따옴표 · `TRUE/FALSE`) · 바로 입력 · 한글 글자(WM_CHAR) · Space 체크 · Enum/RowRef 콤보(두 번 클릭 · F2) · Esc 취소 · 중복 키 오류 → 저장 막힘 · 깨진 RowRef 경고 · 실행 취소 / 다시(툴바 · Ctrl+Z, 상태 줄에 무엇을 되돌렸는지) · 열 머리 메뉴 · 내림차순 보기 정렬(파일 순서 그대로) · 한글 필터 "1 of 3 rows" · 범위 끌기 + Ctrl+C → TSV(탭 · CRLF · 따옴표) · 저장 파일 확인 · 종료 0 → 재시작 → 값 유지 · 열 속성 패널 String→Int(3값 기본값) → 되돌리기 · 탭 닫기(깨끗하면 바로, 저장 안 됐으면 Save and close / Discard / Cancel) · 저장 안 된 상태의 바깥 수정 알림 줄(Keep mine → 저장이 덮어씀 / Reload → 디스크 값) · 칸 메뉴 · 행 복제 · Ctrl+- 행 삭제. 로그 `[error]` 0 · live blocks 0 |
| V2 에서 고친 것 | 실행 취소로 오류가 풀린 뒤에도 빨간 실패 문구가 남던 것(성공한 편집이 지움, 실행 취소 · 다시는 `Undo: <동작>`), 탭이 없을 때 상태 문구가 도구 막대 줄에 붙던 것, 탭을 닫으면 `Closed <경로>`, 행 복제 상태 문구 |
| V3 회귀(워크트리) | `gameui.selftest` OK (111/111) · `console.selftest` OK (64/64) · `gmtest` OK(World 64 passed) · `net.test all` OK(101 passed) · 에디터 실행 종료 0 |
| 메인 트리 | PreBuild exit 0 → 솔루션 전체 빌드 exit 0(14 프로젝트, 327 s, 경고는 기존 LNK4098 2건뿐) → `datatable.selftest` OK (127/127) · `gameui.selftest` OK (193/193) · `console.selftest` OK (64/64) · `gmtest` OK · `net.test all` OK(101 passed), 모두 종료 0 · live blocks 0 → 에디터 실행 · 메뉴 `Windows/Data Table Editor` 열림 · 종료 0 · `[error]` 0 · 한글 글꼴 경고 없음(사용자 `imgui.ini` 는 실행 전 상태로 되돌림) |

검증하지 못한 것: 실제 손 입력(이번 V2 는 PostMessage 주입 + 워크트리 전용 훅), 한글 IME **조합** 입력(확정 글자 WM_CHAR 만 확인 — DT-H12), 게임 프로젝트 에디터에서의 테이블(`/JGGame/`).

### 14-5. 측정 (DevelopEngine 구성, 워크트리 임시 측정 코드 — 측정 뒤 지움)

| 항목 | 값 |
|---|---|
| 에디터 글꼴 아틀라스 | 한글 없음 512×256(RGBA32 0.5 MB) · 3~4 ms → 한글 합침 1024×2048(**8 MB**) · 123~125 ms(시작 때 한 번). 문제가 되면 KS X 1001 2,350자 범위로 줄인다(보류 DT-H14) |
| 1만 행 테이블(2.9 MB, 6열) 열기 | 보기 · 검증 첫 계산 58 ms |
| 편집 하나(칸 · 실행 취소) 뒤 보기 · 검증 다시 계산 | 47~66 ms |
| 필터 글자 입력 한 번 | 131 ms(`행`, 첫 열에서 맞음) ~ 258 ms(`행 99`, 111행 — 모든 칸 글을 만든다). 끊김이 문제면 칸 글 캐시 · 입력 뒤 잠깐 기다렸다 거르기(보류 DT-H15) |
| 보기 정렬(Int 열) | 90 ms |
| 프레임 | 1만 행 탭을 연 채 가만히 110 fps(탭 없음 129 fps) — 보이는 행만 그린다 |
| 1만 행 헤드리스(V1) | 쓰기 869 ms · 읽기 795 ms · 검증 56 ms · 바인드 82 ms(메인 트리) |

---

## 15. 사용 안내 — 게임 모듈에서 테이블을 만들고 묶는 법

1. 에디터 메뉴 `Windows/Data Table Editor` → `New...` → Content(`/JGGame/` · `/JGEngine/`) · 경로(`Data/Items` — ASCII) · 키 열 이름(기본 `Id`) → `Create`. 파일 `Content/Data/Items.jgasset` 이 생기고 탭으로 열린다.
2. `+ Column`(또는 열 머리 오른쪽 클릭 → Insert column) 으로 열을 만들고, 열 머리를 누르면 오른쪽 패널에서 타입 · 기본값 · 범위 · Enum 목록 · RowRef 테이블 · AssetRef 클래스를 바꾼다. 행은 `+ Row` · 칸 메뉴 · 엑셀에서 복사한 블록 붙여넣기(키 열부터 붙이면 행이 늘어난다).
3. `Ctrl+S` 저장. 오류(빈 · 중복 키)가 있으면 저장이 막히고, 경고(범위 · 깨진 참조)는 저장된다. 텍스트 편집기 · git 으로 고친 파일은 열린 탭이 1초 안에 다시 읽는다.
4. 게임 코드 — 필요한 열만 멤버 포인터로 묶는다(예시 이름은 중립). 게임 모듈 `module.json` 의존에 **`Asset` 을 더한다**(`Data/DataTable.h` 가 `Asset.h` 를 include — 게임 템플릿 기본 의존은 Core · GameFrameWorks 뿐이고 include 경로가 전이되지 않는다, GFW C-1 · GFW 시험 게임들과 같음).

```cpp
#include "Data/DataTableView.h"

struct HItemRow
{
	int32      Count = 0;
	PString    Label;
	EItemKind  Kind = EItemKind::A;   // JGENUM + JGENUMMETA 항목
	HAssetPath Mesh;
	PName      Next;

	static void BindColumns(HDataTableRowBinder<HItemRow>& binder)
	{
		binder.Bind("Count", &HItemRow::Count);
		binder.Bind("Label", &HItemRow::Label);
		binder.Bind("Kind",  &HItemRow::Kind);
		binder.Bind("Mesh",  &HItemRow::Mesh);
		binder.Bind("Next",  &HItemRow::Next);
	}
};

HDataTableView<HItemRow> _items;

void loadItems()
{
	POnLoadCompelete onLoaded;
	onLoaded.BindLambda([this](PWeakPtr<JGAsset> inAsset)
		{
			PSharedPtr<JGDataTable> table = Cast<JGDataTable>(inAsset.Pin());
			HList<HDataTableIssue> issues;
			if (table == nullptr || _items.Bind(table, &issues) == false)
			{
				return;   // 열 없음 · 타입 불일치는 issues 와 로그(HDataTableView<…>::Bind)에
			}
			// 에디터에서 저장하면 OnChanged → 다시 묶는다
			table->OnChanged.AddLambda([this](const JGDataTable&)
				{
					// _items.Bind(...) 를 다시 부른다
				});
		});
	GAssetDatabase::GetInstance().LoadAssetAsync(HAssetPath("/JGGame/Data/Items.jgasset"), onLoaded);
}

// 조회: 키는 해시 색인, 순회는 늘 파일의 행 순서
const HItemRow* row = _items.Find(PName("Row_A"));
for (int32 i = 0; i < _items.GetCount(); ++i)
{
	const HItemRow& item = _items.Get(i);
}
```

5. 바인딩 규칙: 없는 열 · 다른 타입은 실패(로그에 열 이름과 기대 타입), 테이블에만 있는 열은 무시. `int32` 는 범위 밖 값이면 실패, `float32/64` 는 Int 열도 받는다, `PString/PName` 은 String · Enum · RowRef · AssetRef 를 받는다. `IsUpToDate()` 가 false 면 테이블이 바뀐 것.
6. 헤드리스 확인: `JGConsole.exe datatable.validate`(경로 없이 = 엔진 · 게임 Content 의 모든 테이블) 또는 `-path=/JGGame/Data/Items.jgasset`(파일 경로도 된다) — 형식 · 키 · 범위 · 참조 문제를 출력하고, 오류가 있거나 읽지 못한 파일이 있으면 실패(종료 코드 1). 10-01 메인 트리: 테이블 0개 → OK, 워크트리 `Sample.jgasset` 파일 경로 → `OK (3 rows)`.
