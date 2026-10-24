# Graphics 이어받기 메모

갱신: 2026-10-24 19:44 (Graphics 세션)

## 지금

| 항목 | 상태 |
|---|---|
| 맡은 일 | 없음. 마지막 일 Engine-56 3D 1단계 조명 Graphics 몫(Q-010 답 19:24 "이 계획대로 조명부터 구현 시작" + "빛 시스템은 언리얼 참고해서 만들어줘") — 끝남, 엔진 총괄에 넘김(검토 중) |
| Engine-56 자리 | 작업 폴더 `Temp/wt/light-gfx`(main 9fa12e2 에서, 분리 HEAD). 코드 패치 `Temp/Light/Graphics.patch`(md5 e28092561901d854638cf6323b6bf9d8) · 문서 패치 `Temp/Light/Graphics_docs.patch`(그림 06 · 07 · 08). 검증 기록 `Document/기록/Graphics/Engine-56_조명_검증_2026-10-24.md`, 위키 반영 글 `Engine-56_위키반영글_2026-10-24.md`, 증적 `Temp/Light/Graphics_verify/`, 그림 확인본 `Temp/Light/wiki_check/Graphics/` |
| 시험 | graphics.selftest 22/22 · gfw.selftest 9/9 그룹 1,120 검사 0 실패 · 에디터 오류 0 |
| 다음 할 것 | 엔진 총괄이 패치 적용 · 커밋 · 위키 반영. GameFrameWork 빛 컴포넌트(Engine-57)는 이 패치 위에서 빌드(필드 이름 LightColor 알림). Graphics API Reference(Engine-39)는 Core 꼴 기다림 — 장면 빛 항목 넣기를 진행 기록에 남김. 2단계 머터리얼은 1단계 보고 뒤 |
| 기다리는 상대 | 없음 |
| 상태 줄 | `Saved/Feedback/status/graphics.txt` = "쉬는 중 — 맡은 일 없음" |
| 잠금 · 빌드 · 시험 | 없음(메인 트리는 쓰지 않음, 작업 폴더에서만 빌드) |
| 미커밋 파일 | 패치 두 개(엔진 총괄이 적용), `Document/기록/Graphics/Engine-56_*` 2개, 이 메모 |

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
