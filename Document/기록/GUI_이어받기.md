# GUI 이어받기 메모

갱신 2026-10-24 19:35. Engine-53 은 설계도 답을, Engine-63 은 Writer 다듬기를 기다린다. Engine-29 · 47 은 검토 중, Engine-16 은 사용자 지시를 기다린다.

## 지금 일

| 일 | 단계 | 기다리는 상대 |
|---|---|---|
| 메뉴로 처음 연 창 크기(Engine-53) | In-Wait — 설계도 `Document/기록/GUI/Engine-53_창처음크기_설계_2026-10-24.md`(보고함 초안 9절), 그림 `Saved/UIShots/Engine-53_WindowSize_compare.png` · `_structure.png`. 권고: 모든 창 같은 처음 크기 · 가운데(`Widget.cpp` 두 줄). 코드는 안 고침 | 엔진 총괄 보고함 올림 → 사용자 답. 답이 오면 작업 폴더에서 구현 · 확인(설계도 7절) · 위키 03. GUI 3절 한 줄 + 변경 이력 |
| 위키 03. GUI 고치기 가(Engine-63) | In-Wait — 가 몫 위키에 넣음(0절 제공 클래스 · 패키지 조건 · 2절 내부 한 줄 · 5절 "대표 함수" · 7절 GameHost 링크 · 변경 이력 둘째 줄, 그림 00 쪽지 다시 그림). 끊긴 링크 0 | Document Writer 다듬기 → 엔진 총괄 확인. 나 몫(Reference 로 옮길 것 · 새 문서 틀 — code-doc check 틀림 17)은 GUI API Reference(Engine-41) 때 |
| 컨텐츠 브라우저 HGUI 감싸개 셋(Engine-29) | 검토 중 — 본 폴더에 커밋됨(e536612 묶음), 위키 반영글 `Document/기록/GUI/Engine-29_위키반영글_2026-10-24.md` 은 엔진 총괄이 위키에 넣음 | 엔진 총괄 완료 처리 |
| 위키 03. GUI 재검토(Engine-47) | 검토 중 — 수정안 14개 표 `Document/기록/GUI/Engine-47_03GUI_재검토_2026-10-24.md`(9절 기준). 가 몫은 Engine-63 으로 반영 | 엔진 총괄 · 사용자 |
| 디테일 패널 — 위젯 부품 바탕 손질(Engine-16) | In-Wait, 0/4. 완료 조건은 첫 적용 두 곳(Data Table Editor 열 속성 · Gameplay DevView 엔티티 값)으로 고쳐짐 | 사용자 지시(H-107) → 엔진 총괄이 알리면 `start Engine-16`, 설계도부터 |

## 끝낸 일 (10-24)

| 일 | 결과 |
|---|---|
| 1단계 검토 — GUI 모듈 · 위키 대조 | 공유 메모 `GUI_1단계_검토_2026-10-24.md`(엔진 총괄 공유 메모 폴더). 바로잡은 6곳은 새 위키 문서에 넣음 |
| 위젯 부품 정리 설계도 | `Temp/개편검토/GUI_위젯부품정리_설계_2026-10-24.html` — 사용자 H-105 "그대로 둠"으로 쓰지 않음 |
| 위젯 부품 쓰임새 검토 | `Temp/개편검토/GUI_위젯부품_쓰임새검토_2026-10-24.html` — 권고: 첫 부품 때 바탕 손질 3가지 + 디테일 패널(약 3일), 둘째 로그 뷰, 리플렉션 디테일은 Core 부모 속성 읽기 뒤 → Engine-16 |
| 2단계 새 위키 문서 | `02. 위키/JGEngine/03. GUI.md`(엔진 총괄이 옮김) — 8절 · 그림 7장, 원본 `Tools/Ops/wiki_svg/specs/GUI/`(pages.js 키 `GUI`, 엔진 총괄이 GUI2 에서 옮김). 문장 다듬기는 Document Writer |

## 잠금 · 빌드 · 미커밋

- 메인 트리 잠금 · 빌드 · 시험 없음. 메인 트리 코드는 고치지 않았다(코드는 작업 폴더 `Temp/wt/cb-gui` 에만 — 메인 적용 때까지 지우지 않는다).
- Engine-29 커밋 목록: `Source/Runtime/GUI/GUI.h` · `GUI.cpp` · `SelfTest/GUISelfTest.cpp`(새) + `Document/기록/GUI/Engine-29_HGUI감싸개_검증_2026-10-24.txt` · `Engine-29_gui_selftest_editor.png`. 얹은 뒤 PreBuild 한 번(생성 파일은 안 바뀜), `jgengine.bat` 은 넣지 않음.
- 미커밋 문서: 이 메모.
- 찾은 일: Engine-43 JGLauncher 가 RequestExit 종료 코드를 버림(담당 엔진 총괄이 정함) — 에디터에서 `-exit` 시험의 실패는 로그 줄로 본다.

## 짚어 둔 코드 일 (일감 아님 — 지금 문제가 없어 손대지 않음)

- 메뉴 트리 키가 경로 조각 이름만 봄(`Menu/MenuTree.cpp` AddMainMenuItem) — 다른 부모 아래 같은 이름이 부딪힘
- `HGUI::InputText` 512 바이트 고정 버퍼(`GUI.cpp` InputText) — 쓰는 곳 없음
- `PlotBarGroups` · `PlotTest` 쓰는 곳 없음, `Backends/GUIBackends.cpp` 는 include 한 줄뿐
- 에디터 창마다 같은 뜻의 색이 다름(오류 · 경고 · 흐림) — 디테일 패널 때 HGUI 팔레트로 합치기 권함
