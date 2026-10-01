# GameFrameWorks 게임 UI TODO (JGGameWidget · CommonUI식)

갱신 2026-10-01 오후(GG-8 줄바꿈 · GG-9 글자 입력 → 사용자 커밋 `736ddbe` 확인). 설계 `Files/GameUI_설계_2026-10-01.md`, 현황 · 함정 · 검증 방법은 `현황.md` §10.
범위: `Source/Runtime/GameFrameWorks/UI/` — 게임이 플레이어에게 보여 주는 화면(HUD · 메뉴 · 대화상자). 화면 하나 = `JGGameWidget`(JGCLASS, 언리얼 CommonUI 의 ActivatableWidget 자리), 레이어 스택 관리자 `PGameUIManager` 는 `JGGameInstance` 가 가진다. 그리기 기반은 Graphics 2D 경로(`IJGGraphicsCommand::Draw(H2DDrawArguments)`, `draw2d.hlsl`), 호스트는 JGEditor Scene Viewport.
사용자 지시(2026-10-01 아침): "GameUI 를 모듈로 따로 떼는 게 아니고 GameFrameWork 모듈 안에 있는 거고, JGWidget 처럼 JGGameWidget 이용해서, 언리얼의 CommonUI 처럼" → 고른 것: 작성 방식 **유지형(CommonUI식)**, 범위 **이전 + CommonUI 핵심**.
이 파일은 `Document/Memory/Etc/TODO_GameGUI.md`(09-30 밤 ~ 10-01 새벽, 별도 GameGUI 모듈안)를 이어받는다. 항목 번호(GG-*)는 그대로 쓴다.
기준: 엔진 기반 구축 · 과설계 금지. 지금 겪는 문제만 미완료 표에, 나머지는 보류 표에 "다시 볼 조건"과 함께 둔다.

---

## 다음에 할 일

| 순서 | 항목 | 착수 조건 | 완료 기준 |
|---|---|---|---|
| 1 | **GG-9U. 실제 한글 IME 확인** (사용자, 한 번) | 메인 Bin 에디터 + 검증 게임(또는 ProjectAH) | 입력란에 IME 로 한글을 조합하면(ㅎ → 하 → 한) 조합 · 후보 창이 입력란 커서 옆에 뜨고, 확정 글자가 들어간다. 스크립트는 확정 글자(`WM_CHAR`)까지만 본다 |
| 2 | 메인 트리 엔진을 쓰는 게임 프로젝트는 배치 2 재실행 | `GameGUI` 프로젝트가 없어졌고(어젯밤 게임 sln 에는 남아 있다) 새 파일 `UI/GameUITextInput.cpp` 가 생겼다 | 게임 sln 빌드 오류 0 |
| — | ~~GG-C1. 커밋~~ **완료** — 사용자 커밋 `736ddbe` "엔진 구현"(10-01 12:16)에 GG-0~GG-9 전부(GFW `UI/` 26 · `draw2d.hlsl` · Graphics 2D 경로 10 · GUI 3 · `SceneViewport` · GFW `Core/GameInstance` · `GameFrameWorksModule` · 문서 · 캡처 · 검증 게임)가 들어갔다 | — | `git show --name-status 736ddbe` |

구현할 미완료 항목은 없다. 다음 기능은 게임 쪽 요구가 생길 때 보류 표에서 고른다.

### 들어온 요청 (게임 프로젝트 → 엔진) — **반영 (2026-10-01 낮, GG-8 · GG-9)**

| 요청 | 내용 | 결정 | 엔진 항목 |
|---|---|---|---|
| ProjectAH ER-007 (10-01) | 글자 자동 줄바꿈(단어 · 글자 방식, 줄바꿈 높이 측정, 사각형 잘라내기, 말줄임) | 수락 · 말줄임 포함(사용자 "진행시켜") → **반영** | **GG-8** 완료 (GG-H3 의 줄바꿈 부분. 리치 텍스트 · 로컬라이제이션은 보류에 남음) |
| ProjectAH ER-008 (10-01) | 글자 입력(입력란 요소, 키보드 · IME 확정 글자, 편집 키, 붙여넣기) | 수락 · 1차 범위(검토서 §3.4), OS 조합 창을 입력란 옆에, 입력 중 Esc = 포커스 해제 → **반영** | **GG-9** 완료 (글자 입력 포커스만 — 방향 이동 · 패드 GG-H7 은 그대로). 실제 IME 조합은 사용자 확인(위 2) |

검토서 `Files/ER-007_ER-008_검토_2026-10-01.md`, 검증 `Files/2026-10-01_GameUI_ER007_ER008_verify.txt`.

## 미완료 항목

| ID | 항목 | 상태 | 선행조건 | 완료 기준 |
|---|---|---|---|---|
| GG-9U | 실제 한글 IME 조합 확인 (사용자) | 대기 | 없음 | 위 표 1 |

## 보류 (다시 볼 조건이 오면 착수)

| ID | 항목 | 지금 상태 | 다시 볼 조건 |
|---|---|---|---|
| GG-H1 | 드래그 앤 드롭(카드 끌기) | 없음. 캡처(누른 요소가 뗄 때까지 이벤트를 받음)는 있다 | 손패를 끌어 내는 게임 흐름을 만들 때 |
| GG-H2 | 레이아웃 그룹(가로 줄 · 손패 부채꼴) · 늘이기 앵커 | 앵커 · 피벗 · 위치 · 크기만. 줄 세우기는 게임 코드가 위치를 계산 | 같은 줄 세우기 코드가 두 번째로 필요할 때 |
| GG-H3 | 리치 텍스트(굵게 · 색 바꾸기 · 글 사이 아이콘) · 로컬라이제이션 | 자동 줄바꿈 · 잘라내기 · 말줄임은 GG-8 로 됨. 한 글자 요소 = 한 스타일 평문 | 글 사이에 기호 · 아이콘을 넣거나 일부만 강조해야 할 때 · 다국어 문자열 표를 쓸 때 |
| GG-H4 | 글꼴 아틀라스 여러 장 · 부분 갱신 | 1024x1024 RGBA8 한 장. 가득 차면 경고 후 새 글리프를 안 그린다. 새 글리프가 생기면 아틀라스 전체(4MB)를 다시 올린다 | 아틀라스가 차거나(경고 로그), 매 프레임 새 글리프가 생겨 업로드가 보일 때 |
| GG-H5 | 밉맵 · 9-slice | 이미지 텍스처는 밉 1(크게 줄이면 계단) · 테두리 늘리기 없음 | 패널 아트가 들어올 때 · Graphics 5-29 |
| GG-H6 | 트윈 · 애니메이션(위젯 열림 · 닫힘 전환 포함) | 없음. 활성 · 비활성은 즉시 바뀐다 | GFW 2-4 시퀀서 착수 때 |
| GG-H7 | 키보드 · 게임패드 포커스 이동(CommonUI 의 Desired Focus Target · 내비게이션) | 포인터(Move/Down/Up/Wheel/Leave)와 Esc(뒤로가기)만. 비활성 버튼은 10-01 에 됨 | 패드 입력 · 키보드 메뉴 조작 요구가 생길 때. 호스트가 키 · 패드 입력을 넘기는 경로도 같이 |
| GG-H8 | 플랫폼 입력 · 단독 실행 호스트 | 호스트는 에디터 Scene Viewport 하나(월드를 가진 프로젝트 모드에서만 열린다 → 월드 없는 게임은 UI 를 볼 곳이 없다) | D-5 단독 실행 호스트, 또는 월드 없는 화면(타이틀 등)이 필요할 때 |
| GG-H9 | UI 머터리얼(커스텀 셰이더) | 내장 `draw2d.hlsl` 하나(텍스처 × 정점 색) | 게임이 UI 효과 셰이더를 요구할 때 |
| GG-H10 | 데이터(JSON) 레이아웃 · 스타일 에셋 · UI 편집기 | 트리 · 스타일은 코드로만 만든다(스타일은 값 구조체) | UI 나 스타일을 코드 밖에서 만들 요구가 생길 때 |
| ~~GG-H11~~ | ~~색 공간~~ | **닫음 (10-01 오후)** — Graphics 5-14 (a) 8비트 sRGB 출력(13:33 메인 반영 · 미커밋)으로 해소. 완료 이력 GG-H11 행 | — |
| GG-H12 | 에셋 파이프라인 연결(`JGTexture` · 글꼴 에셋) | 파일 경로를 직접 읽는다(`LoadTexture` · `LoadFont`, 경로별 캐시) | 게임 Content 를 에셋으로 패키징할 때 |
| GG-H13 | 탭 목록 · 버튼 그룹(선택 상태 · 라디오) | 버튼은 보통 · 호버 · 누름 · 비활성만(선택 상태 없음) | 탭 화면이나 "하나만 고르기" 버튼 묶음이 필요할 때 |
| GG-H14 | 입력 액션 바(키 · 패드 안내 표시) · 입력 액션 바인딩 | 없음 | GG-H7 과 함께 |
| GG-H15 | 위젯 재사용 풀 · 지연 로드 | `PushWidget<T>` 가 매번 새로 만든다 | 화면 열기 비용이 보이거나 같은 화면을 자주 여닫을 때 |
| GG-H16 | 스택 레이어 대신 큐 · 스위처 컨테이너(CommonUI 의 Queue · Switcher) | 레이어 4개 × 스택만 | 차례로 보여 줄 알림 큐 등 다른 컨테이너 규칙이 필요할 때 |
| GG-H17 | 스크롤 영역(긴 글 · 긴 목록) | 없음. 휠은 UI 위에서 소비만 한다. 긴 글은 `SetMaxLines` + 말줄임이나 잘라내기로 자른다 | 패널 높이를 넘는 글 · 목록(덱 편집 · 카드 목록)을 보여 줄 때 |
| GG-H18 | 글 배치 캐시 | 글자 요소 · 입력란이 그릴 때마다 줄 나누기 · 글자 경계를 다시 계산한다(수백 자는 문제없음) | 긴 글이 많아 프레임 시간에 보일 때 — (글 · 크기 · 폭)별 배치 결과를 요소에 둔다 |
| GG-H19 | 여러 줄 입력란(채팅 기록 · 메모) | 한 줄만. 줄바꿈 문자는 버린다(붙여넣은 여러 줄은 한 줄로 이어진다) | 여러 줄 글을 입력받을 때 |
| GG-H20 | 비밀번호 가림 | 없음 | 비밀번호 · 비밀 코드 입력이 생길 때 |
| GG-H21 | 조합 중 글자를 입력란 안에 그리기 | OS IME 조합 창을 입력란 커서 옆에 띄운다(ImGui 입력란과 같은 방식) | 조합 창이 게임 화면에서 어색하다는 요구가 생길 때(WM_IME_COMPOSITION 을 호스트가 받아야 함) |
| GG-H22 | 마우스 끌어 선택 · 두 번 눌러 단어 선택 · Shift+클릭 · Ctrl+← → 단어 이동 | 누른 자리 커서 · Shift + 키 선택 · 전체 선택만 | 긴 글을 고쳐 쓰는 입력란이 생길 때 |
| GG-H23 | 되돌리기(Ctrl+Z) | 없음 | 긴 글 입력이 생길 때 |
| GG-H24 | BMP 밖 글자(이모지) 입력 | ImGui 글자 큐가 16비트(`IMGUI_USE_WCHAR32` 꺼짐)라 U+FFFD 로 들어오고, 호스트가 버린다. 글꼴 · 배치는 코드포인트 32비트라 그리기는 된다 | 채팅 등에서 이모지 입력이 필요할 때(ImGui 설정을 32비트로 · 글꼴 대체) |

## 완료 이력

| ID | 항목 | 완료일 | 검증 근거 |
|---|---|---|---|
| GG-0 | 설계 문서(별도 GameGUI 모듈안) | 2026-09-30 | `Document/Memory/Etc/Files/GameGUI_설계_2026-09-30.md` — **10-01 GG-7 로 구조 대체** |
| GG-1 | Graphics 2D 그리기 경로(`H2DVertex` · `H2DDrawArguments` · 순수 가상 `IJGGraphicsCommand::Draw` · DX12 구현 · 동적 VB/IB · `draw2d.hlsl` · `GShaderLibrary::GetShaderCode` · `GetDraw2DShader`) | 2026-10-01 새벽 (커밋 `736ddbe`) | `Document/Memory/Etc/Files/2026-10-01_GameGUI_verify.txt`. Graphics 에 그대로 남는다 |
| GG-2 | 글꼴(stb_truetype · 한글 · 아틀라스) · UTF-8 · 이미지(stb_image) | 2026-10-01 새벽 | 위와 같음. 10-01 오전 GFW `UI/` 로 이름만 바꿔 옮김(`PGameUIFont` 등) |
| GG-3 | 최소 요소(요소 트리 · Image · Text · Button · 그리기 목록 · 렌더러) | 2026-10-01 새벽 | 위와 같음. GFW 로 옮기며 캔버스는 관리자에 흡수 |
| GG-4 | 호스트: JGEditor Scene Viewport(월드 뒤 그리기, 포인터 먼저) | 2026-10-01 새벽 | 위와 같음. 10-01 오전 대상만 `JGGameInstance::GetUI()` 로 바꾸고 Esc 추가 |
| GG-5 | 헤드리스 셀프 테스트 | 2026-10-01 새벽 (`gamegui.selftest` 64) | 10-01 오전 `gameui.selftest` 111 로 바뀜 |
| GG-6 | 검증(별도 모듈안) | 2026-10-01 새벽 | 위 기록 |
| GG-H11 | **색 공간(Graphics 5-14 와 함께)**: Graphics 세션이 프레임 버퍼를 8비트(R8G8B8A8_Unorm)로, 장면 렌더러에 디스플레이 패스(선형 FP16 → sRGB 인코딩 `GetDisplayTexture()`)를 넣고, 씬 뷰포트가 게임 UI 를 그 위에 그린다 → UI 색이 코드 값 그대로 보인다(옅게 뜨지 않음). 게임 UI 코드 변경 없음(2D 경로는 대상 형식을 PSO 에 넣는다). 검증 스크립트는 두 색((242,115,26) → (249,179,89))을 찾게 고침 | 2026-10-01 오후 (Graphics 5-14 는 미커밋) | 메인 엔진(13:33 빌드) 검증 게임 전체 다시 빌드 272초 → V2 20단계 통과("Orange pixels 3376 as sRGB display (242,115,26)", 오류 0 · live 0), 디버거 아래 25초 실행 중 D3D12 메시지 0(종료 때 Live Producer 1 · Live Object 12 = 기준 11 + 5-14 로 늘어난 객체 하나 — Graphics 세션 확인: 디스플레이 텍스처일 가능성이 크고 새 누수 경로 아님, 디버그 레이어의 종료 보고), 캡처 `Files/2026-10-01_gameui_er_srgb514.png` |
| GG-8 | **글자 자동 줄바꿈(ProjectAH ER-007)**: `EGameUITextWrap { None, Word, Character }`(기본 None = 지금까지와 같음). Word = 공백에서, 줄보다 긴 단어는 그 안에서, 한자 · 가나는 글자 사이(한글은 어절), Character = 아무 글자 사이. 둘 다 닫는 부호는 줄 첫머리 · 여는 부호는 줄 끝 금지, 바꾼 자리 공백은 버림. 줄 나누기는 논리 단위(실수 글꼴 크기의 글자 너비 — `PGameUIFont::MeasureAdvance/MeasureKerning/MeasureAscent/MeasureLineHeight`, 래스터화 없음)라 배율이 달라도 같은 자리, 그리기는 글리프 자리 = 논리 배치 × 배율(넘치지 않음). `PGameUIFont::LayoutText → HGameUITextLayout`(줄 = 바이트 범위 · 폭 · 말줄임), `PGameUIText::SetWrap/SetMaxLines/SetEllipsis/SetClip/MeasureContent(width, manager)`, 배치를 받는 `AddTextDraw` 오버로드. 버튼 라벨은 예전 함수 그대로 | 2026-10-01 낮 (커밋 `736ddbe`) | `Files/2026-10-01_GameUI_ER007_ER008_verify.txt` — `gameui.selftest` 줄바꿈 38검사, 검증 게임 패널 |
| GG-9 | **글자 입력(ProjectAH ER-008)**: 입력란 `PGameUITextInput`(새 파일 `UI/GameUITextInput.h/.cpp`: 글 · 자리 글 · 최대 길이(글자 수) · 허용 문자 · `HGameUITextInputStyle`(관리자 기본 있음) · `OnTextChanged` · `OnCommitted`, 커서 · 선택 · 누른 자리 커서 · 가로 밀기 · 여백 안 잘라내기 · 깜빡임). 관리자 글자 입력 포커스(누르면 포커스, 다른 곳 누르면 해제, 입력을 못 받게 되면 — 위젯이 빠지거나 덮임 · Menu 위젯 아래 · 숨김 · 꺼짐 — 포커스 없음): `WantsTextInput` · `HandleTextInput(UTF-8)` · `HandleKey(EGameUIKey, bShift)`(Esc = 해제만, Enter = 해제 후 확정, Tab = 같은 위젯 다음 입력란) · `GetTextInputCaretRect` · `GetTextInputSelection` · `Set/ClearTextInputFocus`. GUI(추가만): `EGUIKey` 편집 키, `IsKeyPressed(key, bRepeat)`, Ctrl/Shift/Alt, `GetInputCharacters`, `ClaimTextEditKeys`(ImGui 내비게이션에서 편집 키 가져오기), `SetTextInputPosition`(IME 창), 클립보드, `HGUIImageInput.ScreenPosition`. 호스트 Scene Viewport `handleKeyboard`(입력란 포커스면 글자 · 편집 키 · Ctrl+A/C/X/V · IME 위치, 아니면 Esc = 뒤로가기) | 2026-10-01 낮 (커밋 `736ddbe`) | 위 기록 — `gameui.selftest` 입력 44검사(합계 193), 검증 게임 입력 단계(주소 · 한글 이름 · Backspace · Enter · 입력 중 Esc) 워크트리 · 메인 엔진 두 번 통과, 메인 Bin 회귀 · 런처. 실제 IME 조합은 GG-9U |
| GG-7 | **GFW 이전 + CommonUI 핵심**: `Source/Runtime/GameGUI` 삭제 → GFW `UI/` 24파일. `JGGameWidget`(JGCLASS, `OnInitialize/OnActivated/OnDeactivated/OnUpdate/OnBackAction/OnShutdown`, 입력 모드 Game/Menu, 뒤로가기 처리, `DeactivateWidget`), `PGameUIManager`(레이어 4개 × 스택, `PushWidget<T>` · `PushWidgetByClass` · `PushWidgetInstance` · `RemoveWidget` · `ClearLayer` · `ClearAllWidgets` · `FindWidget<T>`, 입력 라우팅 · Menu 차단 · `HandleBackAction`, 기본 글자 · 버튼 스타일, 글꼴 · 텍스처 캐시, 기준 해상도), `JGGameInstance::GetUI()`(교체 때 넘김), 요소 비활성(`SetEnabled`, 부모가 꺼지면 자식 막음), 스타일 `HGameUITextStyle`(필드별 상속) · `HGameUIButtonStyle`, Esc → 뒤로가기(`HGUI::IsWindowFocused/IsKeyPressed(EGUIKey::Escape)`) | 2026-10-01 오전 (커밋 `736ddbe`) | `Files/2026-10-01_GameUI_GFW_verify.txt` — V1 `gameui.selftest` 111/111, V2 검증 게임(`Files/tools/gameui_demo_testgame/`) 실제 입력, V3 회귀 |

## 커밋 기록 (GG-C1 — 완료, `736ddbe`)

사용자 커밋 `736ddbe` "엔진 구현"(2026-10-01 12:16)에 아래 목록이 모두 들어갔다(`git show --name-status 736ddbe` 로 확인, 10-01 오후). 그 뒤 메인 트리에서 이 파일들에 생긴 변경은 다른 트랙 것이다(게임 UI 변경 아님, 미커밋): `SceneViewport.cpp` — Graphics 세션이 게임 UI 를 sRGB 디스플레이 텍스처(`GetDisplayTexture`)에 그리게 바꿈(5-14 색 공간, 아래 GG-H11), `GUI.h/.cpp` · `Backends/DX12GUIBackend.cpp` · 새 `GUI/Grid/` — DataTable 세션(래퍼 13개 · 그리드 · 에디터 한글 글꼴, 게임 UI 클립보드 함수를 그대로 씀).

- 새 파일: `Source/Runtime/GameFrameWorks/UI/` 26파일(`GameUI.h`, `GameUIDefines.h`, `GameUIStyle.h`, `GameUIUtf8.h`, `GameUIFile.h`, `GameUI{Font,DrawList,Renderer,Element,Image,Text,Button,TextInput,Manager}.{h,cpp}`, `GameWidget.{h,cpp}`, `GameUISelfTest.cpp`), `Source/Shader/draw2d.hlsl`. GG-8 · GG-9 는 이 미추적 파일들 안의 변경 + 새 `GameUITextInput.{h,cpp}`
- 지울 것: `Source/Runtime/GameGUI/`(미추적이었으므로 git 에서는 "추가 안 함"), `Bin/DevelopEngine/GameGUI.{dll,exp,lib}`(미추적)
- 수정(GFW): `Core/GameInstance.{h,cpp}`(UI 관리자 소유 · 틱 · 종료 · `detachUI`), `Core/GameFrameWorksModule.{h,cpp}`(교체 때 UI 넘김 · 주석) — GFW Phase 2 · Server Phase 5 와 같은 파일
- 수정(Graphics, GG-1): `JGGraphicsDefine.h`, `JGGraphicsCommand.h`, `DirectX12/DX12GraphicsCommand.{h,cpp}`, `DirectX12/Classes/CommandList.{h,cpp}`, `DirectX12/DirectX12API.{h,cpp}`, `Classes/ShaderLibrary.{h,cpp}`
- 수정(GUI, Esc · 글자 입력): `GUIDefines.h`(`EGUIKey` — Escape + GG-9 편집 키 18개), `GUI.h/.cpp`(`IsWindowFocused` · `IsKeyPressed(key, bRepeat)` · Ctrl/Shift/Alt · `GetInputCharacters` · `ClaimTextEditKeys` · `SetTextInputPosition` · 클립보드 · `HGUIImageInput.ScreenPosition`, 끝에 추가) — 다른 트랙(편집기 테마 · 콘솔) 변경과 같은 파일
- 수정(호스트 · 도구): `Source/Editor/JGEditor/Widgets/SceneViewport.{h,cpp}`(JGEditor E-1 새 파일로 같이 들어감. GG-9 로 `handleBackKey` → `handleKeyboard` · `forwardTextInput`), `JGEditor.module.json`(GameGUI 의존 없음 = HEAD 와 같아짐), `Source/Programs/JGConsole/Main.cpp`(GameGUI 연결 없음 — HEAD 와 주석 한 줄만 다름: `gameui.selftest` 언급)
- 재생성: `JGEngine.sln`, `jgengine.lua`, `Source/Programs/JGBuildTool/jgengine.lua` — 다른 세션 변경과 섞인다
- 바이너리: 재빌드 Bin(GameFrameWorks · GUI · JGEditor · JGConsole 등)
- 문서: `Document/Memory/GameFrameWorks/{TODO_GameUI.md, 현황.md §10, Files/GameUI_설계_2026-10-01.md, Files/2026-10-01_GameUI_GFW_verify.txt, Files/ER-007_ER-008_검토_2026-10-01.md, Files/2026-10-01_GameUI_ER007_ER008_verify.txt, Files/2026-10-01_gameui_*.png, Files/tools/gameui_demo_testgame/}`, `Document/Memory/Etc/` 옛 GameGUI 문서(이전 표시), `Document/Memory/{진행현황.md, README.md}`, `Document/Memory/Graphics/현황.md`, `Document/GameUI_진행보고_2026-10-01.html`
- 제외: `Bin/DevelopEngine/imgui.ini` · `jg_log.txt`(실행마다 바뀜), `Temp/`
