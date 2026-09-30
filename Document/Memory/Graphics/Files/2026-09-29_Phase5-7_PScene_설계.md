# 2026-09-29 Phase 5-7 PScene · 카메라 (+ 5-22, 5-26) — 단계 1 · 2 구현 완료

## 상태
- 설계 방안(`Document/Graphics_PScene_설계방안_2026-09-29.md`)을 사용자가 승인("그렇게 진행해줘"). 단계 1(5-7 + 5-22)과 단계 2(5-26 (1)~(3))를 구현 · 검증했다. 커밋은 사용자가 직접.
- 남은 것: 단계 3 GFW 2-1 · 2-2(GFW 트랙: `PWorld::GetScene`, 카메라 · 스태틱 메시 컴포넌트), 5-26 (4) 런타임 빌드에서 CPU 사본 해제, 5-25 리드백 덤프 정리.

## 구조 (코드 기준)
| 타입 | 위치 | 하는 일 |
|---|---|---|
| `PScene` | `Graphics/Classes/Scene.h/.cpp` | 메시 배치 · 카메라를 ID → 값으로 든다. `CreateMesh/DestroyMesh/FindMesh/SetMeshWorldMatrix/SetMeshMaterial`, `CreateCamera/DestroyCamera/FindCamera/SetCamera`, `GetMeshes`. 메인 스레드가 아니면 에러 로그 후 거부 |
| `HSceneMesh` | 같은 곳 | `IMesh`(에셋의 렌더 메시) + 서브메시별 머터리얼(비면 메시 것) + 월드 행렬. 리소스는 참조만(강참조) |
| `HSceneCamera` | 같은 곳 | 뷰 행렬 · 위치 · FovY · Near/Far. `SetLookAt`, `SetWorldMatrix`(카메라 → 월드), `GetProjMatrix(aspect)` |
| `HSceneMeshID` / `HSceneCameraID` | 같은 곳 | 0 = 무효. 밖(DevScene, GFW 컴포넌트)은 ID만 든다 |
| `PSceneRenderer` | `Graphics/Classes/SceneRenderer.h/.cpp` | `Initialize(name, w, h)`, `Render(scene, cameraID)`: 지오메트리 패스(G버퍼 4장 `ESceneGBuffer` + 깊이) → 합성 패스(Screen 도메인) → `GetOutputTexture()`. 카메라가 없으면 출력만 지운다 |

## 바뀐 파일
| 파일 | 내용 |
|---|---|
| `Classes/Scene.h/.cpp`, `Classes/SceneRenderer.h/.cpp` | 위 표. 새 파일이라 PreBuild가 필요했다 |
| `Classes/Camera.h` | 삭제(빈 스텁) |
| `Material.h`, `DX12Material.cpp`, `ShaderLibrary.h/.cpp`, `Source/Shader/graphics_shader_template.hlsl` | `EMaterialDomain::Scene` → `Screen`, `MATERIAL_DOMAIN_SCREEN`, `PS_SCREEN_*` |
| `JGGraphicsDefine.h`, `JGGraphicsCommand.h`, `DX12GraphicsCommand.h/.cpp` | `HSceneDrawArguments` → `HScreenDrawArguments`. `HDrawArguments::MaterialOverrides`(배치별 머터리얼) |
| `Devkit/DevScene.h/.cpp` | 장면 · 렌더러 · 카메라 ID · 메시 배치 ID만 든다. 패스 · G버퍼 코드는 렌더러로 옮김 |
| `Classes/VertexBuffer.h`, `IndexBuffer.h`, `DX12VertexBuffer.h/.cpp`, `DX12IndexBuffer.h/.cpp` | 5-26 (1): `_shadowData` · `GetDatas/GetData/SetData` 삭제. `SetDatas`가 스테이징 관리자에 바로 요청(요청 시점에 memcpy) |
| `Classes/StaticMesh.h/.cpp` | 5-26 (2): `HStaticSubMesh::Vertices/Indices`(CPU 사본), 렌더 메시는 `HRenderSubMesh`(버퍼 · 머터리얼만), `SetData`는 서브메시를 move. (3): `JGPROPERTY() HBBox _bounds`를 `SetData`에서 구해 저장, `CalculateBounds` → `GetBounds`. `_bounds` 없는 예전 에셋은 `OnLoadAsset_Thread`에서 정점으로 구하고 Trace "no saved bounds" |

## 검증
| 항목 | 결과 |
|---|---|
| 빌드 | 16/16 오류 0 (`build_2026-09-29_5-7.log`, `_5-26.log`, `_5-26_bounds.log`, `_5-26_final_devkit.log`) |
| 60초 실행 3회(단계 1, 5-26 (1)(2), 최종) | error/critical 0, D3D12 0, PSO 2, 종료 코드 0, live blocks 0 |
| 화면 · 리드백 | 경계 상자 min(-90.3, -0.0, -17.2) max(90.3, 180.9, 14.9), 서브메시 2 · 정점 28,464 · 인덱스 147,336, 씬 중앙 0.174, 알베도 65,587px. 리드백 PNG 2장이 5-7 이전과 바이트 동일. 캡처 `2026-09-29_5-7_step1_capture.png`, `2026-09-29_5-26_capture.png` |
| 경계 상자 저장 (3) | 테스트 FBX(쿼드) 임포트 → 저장 파일에 `"_bounds": [-100, 0, -0, 100, 100, -0]`. 같은 실행에서 디스크로 다시 로드 · 다음 실행에서 시작 로드, 두 경로 모두 저장된 값 == 정점으로 다시 구한 값 → PASS. "no saved bounds"는 실행마다 1줄(예전 `Sample.jgasset`) |

## 다음 에이전트 참고
- GFW 2-1 · 2-2: 컴포넌트는 ID만 든다. `OnBeginPlay`에서 `CreateMesh/CreateCamera`, `OnTick`에서 `SetMeshWorldMatrix/SetCamera`, `OnEndPlay`에서 `Destroy*`. `PScene`은 메인 스레드 전용.
- `Content/TempAsset/Sample.jgasset`(36MB, git 추적)은 `_bounds` 없는 예전 형식이라 로드마다 정점으로 구한다. 다시 저장하면 이 경로가 사라지지만 36MB 파일 전체가 바뀌므로 하지 않았다.
- 최종 실행 바이너리에는 다른 세션(Memory 4-1)의 임시 계측 `[4-1 repro]`가 들어 있어 경고 3줄(해제된 카운터에 약참조 IsValid)이 찍혔다. 이 변경은 약참조를 늘리지 않는다(`HRenderSubMesh::Material`은 전에 복사되던 `HStaticSubMesh::Material`과 같다).
- 도구: `Document/Memory/tools/boundstest_snippet.cpp.txt`(경계 상자 저장 검증), `tools/fbx_embedded_texture/`(테스트 FBX 생성기).
