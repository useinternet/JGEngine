# Document Writer 이어받기

엔진 팀 Document Writer(엔진 총괄 하위)의 이어받기 메모다. 맡는 일은 보고함 글 다듬기와 엔진 위키 문장 다듬기다.

## 지금

- 맡은 일감 없음. 상태 줄(`Saved/Feedback/status/writer.txt`)은 "쉬는 중 — 맡은 일 없음".
- 잠금 · 빌드 · 시험 · 감시 없음. 고친 코드 없음.

## 끝낸 일

| 날짜 | 일 | 결과 |
|---|---|---|
| 2026-10-24 | 렌더링 파이프라인 · 3D 검토 보고서(Engine-27) 다듬기 — Graphics 요청 | `Document/기록/Graphics/Engine-27_렌더링파이프라인_3D검토_2026-10-24_다듬음.md`. 데모 내용을 더한 새 초안(18:42:46 판)으로 다시 다듬음. 빠진 것 없음. 보호 부분 기계 대조 같음. 그림 2 이름 차이는 Graphics 가 고침(18:43:57) — 남은 확인 없음 |
| 2026-10-24 | 컨텐츠 브라우저 설계도(Engine-24) 보고함 글 다듬기 — Editor 요청 | `Document/기록/Editor/Engine-24_컨텐츠브라우저_설계_2026-10-24_다듬음.txt`. 빠진 것 없음. Editor 에 확인 둘(감싸개 셋/그림 "둘", 가져오기 예제 "지금" 블록) 넘김. 보고함에는 Editor → 엔진 총괄이 올림 |
| 2026-10-24 | 엔진 위키 문장 다듬기(H-109 ②) | 문서 18개, 135군데. 전 · 후 기록은 `C:/obsidian/ProjectJG/03. 대시보드/data/polish/JGEngine/`, 요약은 같은 폴더 `2026-10-24_JGEngine 위키 다듬기 요약.md`. 확인 필요 1~5는 엔진 총괄이 고침. 6(`Tools/GameFrameWorks` 폴더 이름)은 코드 폴더라 따로 둠 |

## 일하는 법

- 보고함 글 다듬기: 엔진 총괄이 초안 경로를 주면 같은 폴더에 `<원래 이름>_다듬음.txt` 로 쓰고 경로를 알린다. 쉬운 말 · 결론 먼저 · 짧은 문장 · 목록과 표를 쓴다. 뜻 · 숫자 · 코드 이름 · 파일 경로 · 선택지 순서는 그대로 둔다. 그림이나 전 · 후 예제 코드가 빠졌으면 다듬지 않고 "빠짐: …"으로 되돌린다. 보고함에 직접 올리지 않는다.
- 위키 문장 다듬기: 고치기 전에 엔진 총괄에 한 줄 알린다. 문장만 다듬고 절은 더하거나 빼지 않는다. 머리말 · 그림 줄 · 코드 상자 · 표 이름 칸은 손대지 않는다. 사실이 틀려 보이면 고치지 않고 "확인 필요: 문서 · 줄 · 까닭"으로 알린다. 전 · 후 기록은 `03. 대시보드/data/polish/JGEngine/` 에 남긴다.
- 다듬기 전에 원본을 떠 두고, 다듬은 뒤 보호 부분(머리말 · 절 제목 · 그림 줄 · 코드 상자 · 표 이름 칸 · 링크 · 인라인 코드 · 숫자 · 영문 이름)이 그대로인지 기계로 맞춰 본다.
- 일감: `node "C:/obsidian/ProjectJG/03. 대시보드/tools/ilgam.js" <명령> … --by "Document Writer"`. 받으면 `start`, TODO마다 `todo … start/done`, 끝나면 `status … "검토 중"`. 상태 줄 · 보고에 일감 번호를 넣는다.
- 맡은 일의 결과 파일: `Saved/Feedback/results/<요청 id>.writer.txt`.
- `Saved/Feedback/memo.md` 는 사용자 메모라 열지 않는다.

## 엔진 위키 자리 (Engine-21 뒤)

`C:/obsidian/ProjectJG/02. 위키/JGEngine/` 아래:

- `JGEngine.md`: 대표 문서. 옛 "JGEngine 전체 구조"와 "JGEngine 프로젝트 구성도"를 합쳤다.
- 00. Core · 01. Graphics · 03. GUI · 04. JGEditor · 05. Programs · JGEngine 팀 구성 및 운영
- `02. GameFrameWork/`: GameFrameWork 개요 · 00. Game UI ~ 08. GameHost 모듈
- polish 기록 파일 이름은 다듬을 때의 옛 이름 그대로 둔다.
