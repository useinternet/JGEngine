# Graphics 이어받기 메모

갱신: 2026-10-24 19:15 (Graphics 세션)

## 지금

| 항목 | 상태 |
|---|---|
| 맡은 일 | ① Engine-51 3D 렌더링 계획 · 코드 설계(Q-005 답 18:52 "조명 => 머터리얼 => 스켈레탈, 애니메이션 … 계획 세우고 어떻게 코드를 작성할건지 보고해"). ② Engine-45 01. Graphics 재검토(Q-004) — 끝남, 엔진 총괄에 넘김(검토 중) |
| 단계 | 둘 다 끝남 — Engine-51 다듬은 계획 · 그림 3장, Engine-45 수정안 표를 엔진 총괄에 넘겼다(일감 검토 중) |
| 다음 할 것 | 사용자가 Engine-51 계획을 확인하면("이 계획대로 조명부터 구현 시작") 엔진 총괄이 여는 1단계 일감으로 구현. 구현 차례: 첫날 PScene 빛 API → 빛 상수 · 합성 셰이더 → 법선 행렬 → 시험 → 문서(9절). "계획을 고쳐 다시 보고"면 고칠 점만 고쳐 다시 올린다 |
| 기다리는 상대 | 없음(사용자 답은 엔진 총괄이 전함) |
| 상태 줄 | `Saved/Feedback/status/graphics.txt` = "쉬는 중 — 맡은 일 없음" |
| 잠금 · 빌드 · 시험 | 없음 |
| 미커밋 파일 | `Document/기록/Graphics/Engine-51_*`(계획 .md · 그림 3장 .svg · .png), `Engine-45_01Graphics_재검토_2026-10-24.md`, 이 메모. 보고함 그림 사본 `Saved/UIShots/Engine-51_Plan_*.png`(Saved 는 커밋하지 않음) |

## Engine-51 요약

- 기록: `Document/기록/Graphics/Engine-51_3D렌더링계획_2026-10-24.md`, 그림 `Engine-51_1_Light` · `2_Material` · `3_Skeletal`(scratchpad `e27/e51_*.js` 로 그림).
- 1단계 조명 3.5~4일 · 2단계 머터리얼 6~7.5일 · 3단계 스켈레탈 · 애니메이션 10~14일 · 시험 에셋 0.5~1일. 합계 21~28 작업일(문서 갱신 포함, CLAUDE.md 9절).
- 설계 핵심: `PScene` 빛 배치(HSceneLight) + 빛 상수 루트 CBV b3 · 법선 행렬 / 표준 Lit(`CreateStandardLitMaterial`) · 셰이더 캐시 · sRGB 포맷 · 양면 · JGMaterial 에셋 / `HSkinnedVertexData` · `PSkeletalMesh` · 뼈 상수 b4 · `JG_SKINNED` 변형 · JGSkeletalMesh · JGAnimationClip · JGSkeletalMeshComponent.
- 기본값: 빛이 없으면 지금 고정 빛, 빛 최대 16 · 뼈 최대 256.

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
