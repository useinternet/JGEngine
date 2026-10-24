# Graphics 이어받기 메모

갱신: 2026-10-24 18:46 (Graphics 세션)

## 지금

| 항목 | 상태 |
|---|---|
| 맡은 일 | Engine-27 렌더링 파이프라인 구조 · 3D 렌더링 기능 검토(엔진 총괄, 사용자 대시보드 18:17). 코드는 읽고 검토만 했고 고친 것은 없다 |
| 단계 | 끝남 — 다듬은 보고(`…_다듬음.md`) · 그림 2장을 엔진 총괄에 넘겼다. 일감은 "검토 중"(보고함 올리기 · 사용자 확인은 엔진 총괄) |
| 다음 할 것 | 사용자가 첫 단계(가 빛 · 나 머터리얼 · 다 스켈레탈)를 고르면 그 단계의 설계도부터(CLAUDE.md 7절) |
| 기다리는 상대 | 없음(사용자 결정은 엔진 총괄이 전함) |
| 상태 줄 | `Saved/Feedback/status/graphics.txt` = "쉬는 중 — 맡은 일 없음" |
| 잠금 · 빌드 · 시험 | 없음(메인 트리 잠금을 쥐지 않았다) |
| 미커밋 파일 | `Document/기록/Graphics/Engine-27_*` 6개(초안 · 다듬음 .md, 그림 2장 .svg · .png), 이 메모. 커밋은 엔진 총괄 몫 |

## Engine-27 결과 요약

- 기록: `Document/기록/Graphics/Engine-27_렌더링파이프라인_3D검토_2026-10-24.md`
- 그림: `Engine-27_그림1_지금구조` · `Engine-27_그림2_3D뒤구조` (.png · .svg, 같은 폴더)
  - 위키 그리개 `render.js` 를 불러 쓰는 스크립트로 그렸다(scratchpad `e27/run.js`). 그리개 파일은 고치지 않았다.
- 권고 첫 단계: "방향광 하나를 장면 데이터로"(1.5일). 렌더링 ①~⑥ 의 합계는 18~22 작업일이다.
- 사용자 덧붙임(Q-003 답 18:29: 애니메이션 · 3인칭 3D 테크 데모 예정)을 반영했다.
  - ⑦ 스켈레탈 · 애니메이션 11~15일, ⑧ 캐릭터(입력 · 3인칭 카메라 · 이동과 충돌) 6~9일. 데모까지 35~46 작업일.
  - 스켈레탈 시험 에셋 확보(GameFrameWork 0.5~1일)를 첫 단계와 나란히 권했다.
- GameFrameWork 에 FBX 노드 변환 · 머터리얼 번호 · sRGB 세 가지를 검토 중에 알렸다. GameFrameWork 가 Engine-26 기록 77-81줄에서 축 · 단위 문제를 확인했고, 그 일을 Engine-27 로 넘겼다.

## 담당

- `Source/Runtime/Graphics`, `Content/Shaders`. 렌더 자체 시험 `graphics.selftest`(`SelfTest/RenderSelfTest.*`, JGConsole `-graphics` · 에디터 콘솔).
- 위키 `C:/obsidian/ProjectJG/02. 위키/JGEngine/01. Graphics.md` 와 그림 원본 `Tools/Ops/wiki_svg/specs/Graphics/`.

## 아직 문서에 넣지 않은 사용자 피드백

- 없음.
