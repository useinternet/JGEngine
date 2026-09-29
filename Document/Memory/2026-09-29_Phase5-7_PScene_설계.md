# 2026-09-29 Phase 5-7 PScene · 카메라 설계 (설계만, 구현 전)

## 상태
- 사용자가 구조를 제안: 밖에서 `PScene`을 만들고, 카메라 · 메시 · 머터리얼 등 구성물의 생성 · 조회 · 삭제를 `PScene`을 거쳐 하며, GameFrameWorks는 `PScene`에 접근해 자기 구성물의 데이터를 고친다.
- 그 구조를 받아 설계 방안을 `Document/Graphics_PScene_설계방안_2026-09-29.md`에 썼다. **사용자 결정 대기.** 코드는 아직 바꾸지 않았다.

## 방안 요지
- 장면이 가지는 것: 카메라(`HSceneCamera`, 값), 메시 배치(`HSceneMesh` = `IMesh` + 서브메시 머터리얼 + 월드 행렬). 메시 · 머터리얼 · 텍스처 리소스는 참조만(`PSharedPtr`).
- 밖에서는 ID(`HSceneMeshID`, `HSceneCameraID`)로만 다룬다. 이유는 GC 강제 파괴 순서 문제와 5-18(약참조) 문제를 피하기 위해서다.
- 그리기는 `PSceneRenderer`(DevScene의 G버퍼 · 합성 코드를 옮긴다, 5-22). GFW는 데이터만 고친다.
- 결정 필요: 위 네 가지 판단, 이름(특히 기존 `EMaterialDomain::Scene` · `HSceneDrawArguments` → `Screen` 개명 여부).

## 다음에 할 일 (결정 뒤)
1. 단계 1: 새 파일 `Graphics/Classes/Scene.h/.cpp`, `SceneRenderer.h/.cpp` → PreBuild(JGBuildTool) → DevScene 교체 → 캡처 · 리드백 65,587px · PSO 2 · 오류 0 확인.
2. 단계 2: 5-26. 단계 3: GFW 2-1 · 2-2(GFW 트랙).
