# Editor 이어받기 메모

갱신 2026-10-24 19:15. 옛 메모(엔진 총괄 공유 메모 폴더의 `editor_handover.md`)를 팀 지침 5절의 새 자리로 옮긴 것.

## 지금 일

| 일 | 단계 | 기다리는 상대 |
|---|---|---|
| Engine-31 컨텐츠 브라우저 — JGContentBrowser 창(시안 A) | 검토 중 — 세 몫 묶음 검증 끝. 묶음 패치 `Temp/ContentBrowser/ContentBrowser_all.patch`, 검증 기록 `Document/기록/Editor/Engine-31_컨텐츠브라우저_검증_2026-10-24.md` | 엔진 총괄 — 본 폴더 적용 · PreBuild · 커밋 |
| Engine-53 메뉴로 처음 연 창이 아주 작게 뜸(새로 찾은 일) | 할 일 — 엔진 총괄 앞으로 만듦(GUI 몫으로 보임) | 엔진 총괄 — 배정 |
| Engine-48 위키 04. JGEditor 재검토(읽기만, 사용자 Q-004 답) | 검토 중 — 수정안 8줄 `Document/기록/Editor/Engine-48_04JGEditor_재검토_2026-10-24.md`. 위키는 고치지 않았다 | 엔진 총괄 — 수정안 반영 여부 |

Engine-31 기록 끝에 "04. JGEditor 위키 갱신할 곳" 표를 더했다(엔진 총괄 요청, 패치 적용 뒤에 고칠 곳).

상위 일감은 Engine-22(사용자 지시 "컨텐츠 브라우저 만들기 ( 언리얼 레퍼런스 ) …") → Engine-25 구현이다. 설계는 Q-002 에서 시안 A 로 확인받았다(Engine-24).
- Engine-29 GUI 감싸개 · Engine-30 GameFrameWork 함수는 각 세션 패치(`Temp/ContentBrowser/GUI.patch` · `GameFrameWork.patch`)를 그대로 얹었다.
- Editor 몫만 따로 `Temp/ContentBrowser/Editor.patch` 에 있다.

상태 줄 `Saved/Feedback/status/editor.txt` = "Engine-31 … — 검토 중 …". 잠금 · 빌드 · 시험 진행 중인 것 없음(메인 트리는 쓰지 않음).

작업 폴더 `Temp/wt/content-browser`(main c6dd99d 에서, detached). 빌드된 Bin 과 시험 프로젝트 사본이 들어 있다. 지울지는 엔진 총괄이 정한다.
- 시험 프로젝트 사본 `Temp/wt/content-browser/Temp/Games/UnrealAssetTest`:
  - EngineRoot 를 작업 폴더로 바꿨다.
  - 원본 프로젝트에 없는 파일 셋: `Content/Imports/XBot.fbx`, 가져오기로 생긴 `Content/Imports/Beta_Surface.jgasset`, `Content/Data/NewTable.jgasset`.
  - 언리얼 에셋이 들어 있다 — git 에 넣지 않는다.
- 시험 배치 `imgui.ini` 사본: `Temp/wt/content-browser/Temp/verify/imgui_test_layout.ini` · `imgui_engine_layout.ini`.
- 검증 원본: 캡처는 `…/Temp/verify/shots/`, 빌드 · 시험 로그는 `…/Temp/verify/`.

미커밋(엔진 총괄 묶음 커밋 몫):
- 이 메모, `Document/구현현황.md` 의 "지금 하는 일" 한 줄.
- 검증 기록 `Document/기록/Editor/Engine-31_컨텐츠브라우저_검증_2026-10-24.md`.
- 앞 일의 문서 셋(Engine-23 조사 · Engine-24 설계 · 다듬은 보고함 글)은 이미 커밋돼 있다(c6dd99d).

## 10-24 한 일

| 일 | 결과 | 기록 |
|---|---|---|
| 1단계 검토 — DevKit 제거 · DevConsole/DevStatistics 편입 | 보고서 둘. DevKit 권고는 Graphics 답대로 (다) Graphics 안 자체 시험 | `Temp/개편검토/Editor_DevKit제거_검토_2026-10-24.html` · `Editor_DevConsole_DevStatistics편입_검토_2026-10-24.html` |
| DevConsole · DevStatistics → JGEditor/Widgets 편입(H-066 · Engine-3) | 설계도 → 사용자 확인(H-075) → worktree 구현 · 검증 → 메인 커밋 51fb1da | 설계 `Temp/개편검토/Editor_DevConsole_DevStatistics편입_설계_2026-10-24.html`, 검증 `Editor_편입_검증_2026-10-24.txt`, 패치 `Editor_편입_2026-10-24.patch` |
| DevKit 제거(H-065 · H-071 · H-100 · Engine-4) | 설계도 → 사용자 확인 → 편입 위에 구현, Graphics 렌더 시험 패치까지 얹어 Devkit 없이 graphics.selftest OK(16/16) → 51fb1da | 설계 `Temp/개편검토/Editor_DevKit제거_설계_2026-10-24.html`, 검증 `Editor_DevKit제거_검증_2026-10-24.txt`, 패치 `Editor_DevKit제거_2026-10-24.patch` |
| 2단계 위키 문서(H-085) | `C:/obsidian/ProjectJG/02. 위키/JGEngine/04. JGEditor.md` — 절 0~4, 그림 6장(JGEditor_00 ~ 05) | 그림 원본 `Tools/Ops/wiki_svg/specs/JGEditor/`(엔진 총괄이 JGEditor2 에서 이름 바꿈) |
| Engine-23 언리얼 컨텐츠 브라우저 살펴보기 | 완료(엔진 총괄이 닫음) | `Document/기록/Editor/Engine-23_언리얼컨텐츠브라우저_조사_2026-10-24.md` |
| Engine-24 컨텐츠 브라우저 설계도 | 보고함 Q-002 → 사용자가 시안 A 를 고름(18:28) | `Document/기록/Editor/Engine-24_컨텐츠브라우저_설계_2026-10-24.md`, 그림 `Saved/UIShots/Engine-24_ContentBrowser_mockups.png` · `_structure.png` |
| Engine-31 JGContentBrowser 창 | 구현 · 묶음 검증 끝(위 "지금 일") | 검증 기록 위, 캡처 `Saved/UIShots/Engine-31_ContentBrowser_*.png` |

`Temp/개편검토/` 는 git 이 무시하는 자리다. 편입 · DevKit 제거 때의 검증 원본(캡처 · 로그)은 `Temp/wt/editor-merge/Temp/verify/` 에 있었다. 엔진 총괄이 H-118 로 그 작업 폴더를 지울 때 함께 없어졌고, 비교 그림 사본만 `Temp/개편검토/Editor_*_검증_*.png` 에 남아 있다.

## 다음 할 것

- 엔진 총괄이 묶음 패치를 적용하면 엔진 위키 `04. JGEditor.md` 의 창 목록에 Content Browser 를 더한다(엔진 총괄이 맡기면).
- 첫 판에서 미룬 것은 설계도 3절 표대로다: 끌어 놓기 · 이름 바꾸기 · 지우기 · 메시 썸네일 · 목록 보기 · 파일 고르기 창.
- 이번에 찾은 다음 판 후보: 타일 · 경로 막대로 폴더를 옮길 때 트리를 그 폴더까지 펼치기. HGUI 감싸개가 하나 더 있어야 한다.

## 알아 둘 것

- 에디터 창 7개는 모두 `Source/Editor/JGEditor/Widgets/`(Content Browser 포함, 패치 적용 뒤).
- 엔진 모듈 연결 목록은 `JGEditor.cpp` `EDITOR_ENGINE_MODULES` = Graphics · GUI · GameFrameWork.
- 창 클래스 이름(`JGDevConsole` · `JGMemoryStatistcs` — 철자 그대로 · `JGContentBrowser`)은 `imgui.ini` 의 열린 창 복원 키다. 바꾸면 사용자 창 배치가 한 번 닫힌 채로 시작한다.
- 에디터 확인 도구: 세션 scratchpad 의 `editor_probe.ps1` 이다(저장소에 없음).
  - 하는 일: 창 메시지로 클릭 · 더블클릭 · 글자 · 키를 넣고, 실제 픽셀로 찍고, 클립보드를 읽는다.
  - 클릭은 이동 · 누름을 붙여 보내야 한다. 그래도 가끔 빗나가 두 번 눌러야 했다.
  - 도킹 탭 누르기는 먹지 않았다. 대신 `imgui.ini` 도킹 칸의 `Selected=` 에 탭 ID 를 넣었다.
  - 탭 ID = CRC32("#TAB", 씨앗 = CRC32(창 제목)). Content Browser 는 0xBF096F38.
  - DPI 인식(SetThreadDpiAwarenessContext -4) 없이 재면 좌표 · 캡처가 1.25배 어긋난다.
- DevConsole 은 시작할 때 입력 줄 포커스로 자기 탭을 고른다. 같은 도킹 칸에 붙인 다른 창은 가려진다.
- 원래 있던 일(이번에 안 고침): 저장된 배치 없이 메뉴로 연 창이 아주 작게 뜬다. DevConsole · 메모리 창(옛 B-11)과 Content Browser 가 같다 → Engine-53.
- 그림 파일 이름은 영문으로 한다(한글 경로는 보고함에 못 붙음). 언리얼 텍스처가 보이는 캡처는 Saved/ 아래에만 둔다.
