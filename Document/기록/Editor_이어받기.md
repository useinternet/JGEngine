# Editor 이어받기 메모

갱신 2026-10-24 18:07. 옛 메모(엔진 총괄 공유 메모 폴더의 `editor_handover.md`)를 팀 지침 5절의 새 자리로 옮긴 것.

## 지금 일

| 일 | 단계 | 기다리는 상대 |
|---|---|---|
| Engine-23 언리얼 컨텐츠 브라우저 살펴보기 | 완료(엔진 총괄이 닫음) — 조사 기록 `Document/기록/Editor/Engine-23_언리얼컨텐츠브라우저_조사_2026-10-24.md` | — |
| Engine-24 컨텐츠 브라우저 설계도 · Engine-25 구현 | In-Wait — 보고함 Q-002(엔진 총괄이 올림, 그림 `Saved/UIShots/Engine-24_ContentBrowser_mockups.png` · `_structure.png`). 보고함 글 `Document/기록/Editor/Engine-24_컨텐츠브라우저_설계_2026-10-24_다듬음.txt` | 사용자 — Q-002 시안 고르기. 답 오면 엔진 총괄이 Engine-25 를 GUI · GameFrameWork · Editor 로 나눠 알림 |

상위 일감 Engine-22(사용자 지시 "컨텐츠 브라우저 만들기 ( 언리얼 레퍼런스 ) …"). Engine-25 구현은 설계도 확인 뒤, Engine-26 에셋 가져오기는 GameFrameWork 몫. 코드는 설계도 확인 전에 고치지 않는다.
상태 줄 `Saved/Feedback/status/editor.txt` = "답 기다림 — Q-002 컨텐츠 브라우저 설계도(Engine-24)". 잠금 · 빌드 · 시험 진행 중인 것 없음. 미커밋: 이 메모, `Document/기록/Editor/` 의 세 문서(Engine-23 조사 · Engine-24 설계 · 다듬은 보고함 글 — 엔진 총괄 묶음 커밋 몫).
그림 원본(scratchpad, 저장소 밖): `cb_mock.html` · `cb_struct.html` — headless Chrome 으로 PNG 를 찍었다. 다시 그릴 때는 같은 꼴로, **파일 이름은 영문**(한글 경로는 보고함에 못 붙음).
- 엔진 총괄 요청(다음 판): 시안 비교 그림의 시안 C 아래 흰 띠 — 그림 코드 문제가 아니라 한 번 생긴 캡처 찍힘이었다(같은 HTML 을 다시 찍으면 깨끗함). 고친 판 `Saved/UIShots/Engine-24_ContentBrowser_mockups_v2.png` 를 만들어 둠(붙인 그림은 그대로).

## 10-24 한 일

| 일 | 결과 | 기록 |
|---|---|---|
| 1단계 검토 — DevKit 제거 · DevConsole/DevStatistics 편입 | 보고서 둘. DevKit 권고는 Graphics 답대로 (다) Graphics 안 자체 시험 | `Temp/개편검토/Editor_DevKit제거_검토_2026-10-24.html` · `Editor_DevConsole_DevStatistics편입_검토_2026-10-24.html` |
| DevConsole · DevStatistics → JGEditor/Widgets 편입(H-066 · Engine-3) | 설계도 → 사용자 확인(H-075) → worktree 구현 · 검증 → 메인 커밋 51fb1da | 설계 `Temp/개편검토/Editor_DevConsole_DevStatistics편입_설계_2026-10-24.html`, 검증 `Editor_편입_검증_2026-10-24.txt`, 패치 `Editor_편입_2026-10-24.patch` |
| DevKit 제거(H-065 · H-071 · H-100 · Engine-4) | 설계도 → 사용자 확인 → 편입 위에 구현, Graphics 렌더 시험 패치까지 얹어 Devkit 없이 graphics.selftest OK(16/16) → 51fb1da | 설계 `Temp/개편검토/Editor_DevKit제거_설계_2026-10-24.html`, 검증 `Editor_DevKit제거_검증_2026-10-24.txt`, 패치 `Editor_DevKit제거_2026-10-24.patch` |
| 2단계 위키 문서(H-085) | `C:/obsidian/ProjectJG/02. 위키/JGEngine/04. JGEditor.md` — 절 0~4, 그림 6장(JGEditor_00 ~ 05) | 그림 원본 `Tools/Ops/wiki_svg/specs/JGEditor/`(엔진 총괄이 JGEditor2 에서 이름 바꿈) |

`Temp/개편검토/` 는 git 이 무시하는 자리다. 검증 원본(캡처 · 로그)은 `Temp/wt/editor-merge/Temp/verify/` 에 있었는데, 엔진 총괄이 H-118 로 작업 폴더를 지울 때 함께 없어졌다. 비교 그림 사본은 `Temp/개편검토/Editor_*_검증_*.png` 에 남아 있다.

## 알아 둘 것

- 에디터 창 6개는 모두 `Source/Editor/JGEditor/Widgets/`. 엔진 모듈 연결 목록은 `JGEditor.cpp` `EDITOR_ENGINE_MODULES` = Graphics · GUI · GameFrameWork.
- 창 클래스 이름(`JGDevConsole` · `JGMemoryStatistcs` — 철자 그대로)은 `imgui.ini` 열린 창 복원 키라 바꾸면 사용자 창 배치가 한 번 닫힌 채로 시작한다.
- 에디터 확인 도구: 창 메시지로 클릭 · 글자를 넣고 실제 픽셀로 찍는 PowerShell 스크립트를 썼다(세션 scratchpad `editor_probe.ps1` — 저장소에 없음). 클릭은 이동 · 누름을 붙여 보내야 하고, 가끔 빗나가 두 번 눌러야 했다. DPI 인식(SetThreadDpiAwarenessContext -4) 없이 재면 좌표 · 캡처가 1.25배 어긋난다.
- 원래 있던 일(이번에 안 고침): 저장된 배치 없이 메뉴로 연 DevConsole · 메모리 창이 아주 좁게 뜸(옛 DevConsole 할 일 B-11).
