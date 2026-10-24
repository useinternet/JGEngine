# 01. Graphics 문서 재검토 — Engine-45

기록: 2026-10-24 19:08, Graphics 세션. 상위 일감: 구조도 재검토(Engine-36). 사용자 Q-004 답: "JGEngine.md 하위 문서까지 싹다 검토".
대상: `C:/obsidian/ProjectJG/02. 위키/JGEngine/01. Graphics.md`(172줄 · 10절 · 그림 11장). 문서는 고치지 않았다.
기준 코드: 본 폴더 main `c6dd99d` 위. Graphics 코드는 2단계 커밋 `51fb1da` 뒤 바뀌지 않았다.

## 결론

| 본 것 | 결과 |
|---|---|
| 코드와 맞는지 | 클래스 · 흐름 · 이름은 맞다. 출처 경로 42개 · 그림 11장 · 링크 3개(`GameFrameWork 개요` · `05. 에셋` · `03. GUI`)가 모두 있다. 의존(Core 만) · 사용 모듈(GameFrameWork · JGEditor · GameHost · GUI) · 최소화 · DX12 내부를 쓰는 바깥 모듈(GUI 백엔드 하나)도 맞다. 어긋난 곳은 작은 것 둘이다(수정안 1 · 2) |
| 게임 프로젝트 눈높이 | 게임 코드가 Graphics 에서 쓰는 이름은 사실상 `IRawTexture`(게임 UI 이미지) · `HVertex`(절차 생성 메시) 둘이다(Engine-35). 문서에는 이 둘을 게임이 어디서 얻고 쓰는지가 없다. 대신 DX12 백엔드 · 리소스 전송 같은 엔진 내부가 2 · 3절로 앞에 나온다(수정안 3 · 4 · 5) |
| API Reference 로 이어지는지 | Reference 로 가는 줄이 없다. 5 · 7절은 제목 하나에 클래스 둘을 묶었다. 코드 주석에만 있는 사용 규칙(텍스처는 컴파일 전에 넣기 · Resize 는 크기 변경이 멈춘 뒤 한 번 · 헤드리스에서 `GetGraphicsAPI()` 금지)이 갈 자리가 없다(수정안 6 · 7) |
| 설명 · 절차(하우투)가 섞였는지 | 단계별 절차는 없다. 9절 끝의 실행 방법 한 줄만 절차에 가깝다(수정안 8) |

## 수정안

| 번호 | 어디를 | 어떻게 | 왜 |
|---|---|---|---|
| 1 | 1-1절 "GPU 리소스 인터페이스는 모두 `IJGGraphicsObject`를 상속한다" | "API 가 만드는 GPU 리소스 인터페이스(텍스처 · 버퍼 · 머터리얼 · 메시 · 프레임 버퍼)는 모두 …"로 좁힌다 | 셰이더 인터페이스(`IRawShader` · `IRawGraphicsShader`)는 `IMemoryObject` 를 바로 상속한다(`Classes/Shader.h:7 · 25`). 셰이더는 머터리얼 안에서만 만들어지므로, 문장을 좁히는 것으로 충분하다 |
| 2 | 5-2절 렌더 패스 첫 문단 | "합성 패스의 조명은 셰이더 코드에 박힌 고정 방향 빛 하나다(장면에 빛 데이터가 없다)"를 한 줄 더한다. 빛을 장면 데이터로 바꾸는 일(Engine-51 1단계)이 끝나면 그 문장을 고친다 | 지금 문서만 읽으면 빛을 어디서 놓는지 찾게 된다. 코드는 `SceneRenderer.cpp:23-25` 에 빛 방향이 박혀 있다. 조명은 구조에 해당하는 사실이라 사소한 세부가 아니다 |
| 3 | 0절 뒤 새 짧은 절 "게임 코드가 Graphics 와 만나는 곳" | 세 줄로 적는다. ① 3D 는 Graphics 를 직접 쓰지 않고 GameFrameWork 컴포넌트(`JGStaticMeshComponent` · `JGCameraComponent`)를 쓴다 — `PScene` · `PSceneRenderer` 는 GameFrameWork 가 감싼다. ② 게임 UI 이미지는 `PGameUIManager::LoadTexture` 가 돌려주는 `IRawTexture` 를 `PGameUIImage::SetTexture` 에 넘긴다. 크기는 `GetTextureInfo()` 로 본다. ③ 절차 생성 메시는 `HVertex` 목록을 `JGStaticMesh` 에 넣는다. 줄마다 GameFrameWork 문서 · API Reference 링크를 달고, 아래 예제를 붙인다 | 게임 쪽 코드가 Graphics 에서 쓰는 이름은 `IRawTexture`(ProjectAH 11개 파일 138회) · `HVertex`(검증 게임 GfwView 3회) 둘뿐이다(Engine-35 1 · 5-2절). 지금 문서는 이 둘이 게임에 어떻게 닿는지 말하지 않는다. 구조도 재검토(Engine-36) 수정안 1 · 03. GUI 재검토(Engine-47) 수정안 2 와 같은 까닭 |
| 4 | 2 · 3절(DX12 백엔드 · 리소스 전송) | 절 머리에 "엔진 내부 — 게임 코드는 쓰지 않는다" 한 줄을 넣는다. 4~7절(머터리얼 · 장면 · 그리기 명령 · 메시와 텍스처) 뒤로 옮기는 것도 검토한다 | 게임 쪽은 DX12 계층을 include 하지 않는다(Engine-35: 공개하지 않는 쪽). 지금 순서면 게임 독자가 커맨드 큐 · 스테이징부터 읽는다. 03. GUI 재검토(Engine-47) 수정안 4 와 같은 판단 |
| 5 | 7절 첫 문단 · 7-1절 | `IRawTexture` 를 만드는 길을 둘로 적는다. 엔진 모듈은 `PJGGraphicsAPI::CreateRawTexture`, 게임은 GameFrameWork(`PGameUIManager::LoadTexture` · 텍스처 에셋 `JGTexture`)로 얻는다. 출처에 `GameFrameWork/UI/GameUIManager.h` 를 더한다 | 지금은 "API 의 `CreateRawTexture` 로 생성한다"만 있다. 게임 코드는 이 함수를 부르지 않는다(Engine-35 표: 게임이 쓰는 멤버는 `GetTextureInfo` · `IsValid` · `Reset` 정도). `GameUIManager.h:211` `LoadTexture` 가 실제 입구다 |
| 6 | 클래스가 나오는 절마다 | "→ API Reference: <클래스>" 링크 한 줄을 단다. Reference 후보를 둘로 나눈다. **게임용**: `IRawTexture` · `HTextureInfo` · `ETextureFormat` · `HVertex`. **엔진 모듈용**: `PJGGraphicsAPI` · `IJGGraphicsCommand` · `IRawMaterial` · `PScene` · `PSceneRenderer` · `PStaticMesh`. 5 · 7절처럼 둘을 묶은 제목은 Reference 에서 클래스별로 나뉜다 | 구조도 → 모듈 문서 → Reference 로 바로 가기(Engine-36 수정안 2). 게임용 후보는 Engine-35 의 쓰임 수에서 골랐다. 엔진 모듈용은 GameFrameWork · GUI · JGEditor 가 실제로 부르는 것이다 |
| 7 | (Reference 가 생기면) 코드 주석에만 있는 사용 규칙 세 가지 | 각 클래스의 "주의"로 옮긴다. ① `IRawMaterial::SetTexture` 는 `Compile` 전에 — 샘플러(필터 · 랩)가 코드에 박혀, 다른 샘플러의 텍스처로 바꾸면 다시 컴파일한다(`DX12Material.cpp:180-191`). ② `PSceneRenderer::Resize` 는 크기 변경이 멈춘 뒤 한 번 — 부를 때마다 셰이더 두 개를 다시 컴파일한다(`SceneRenderer.h:43-48`). ③ `GetGraphicsAPI()` 는 Graphics 가 없으면 멈춘다(`JGGraphics.cpp:139-144`). 헤드리스일 수 있는 곳은 `FindModule<HJGGraphicsModule>()` 로 먼저 본다(`GameUIRenderer.cpp:30-35`) | 셋 다 쓰는 쪽이 모르면 잘못 쓰기 쉬운 규칙인데, 문서 어디에도 없다. 위키 본문(설명)에 넣으면 사소한 세부라서, Reference 의 "주의" 자리가 맞다 |
| 8 | 9절 마지막 문장 "콘솔 명령 `graphics.selftest`로 실행하고, …" | 설명에는 "콘솔 명령 `graphics.selftest` 로 실행한다"만 남긴다. 실행 방법(`-exit`, `JGConsole -graphics`, 결과 PNG 가 `Temp/GraphicsSelfTest/` 에 남음)은 하우투 · Reference 로 옮긴다 | 새 규칙: 위키 문서는 한 종류(설명)만. 지금 섞인 것은 이 한 줄뿐이라 급하지 않다 |
| 9 | 문서 맨 아래 | "변경 이력" 절(날짜 · 바뀐 것 · 까닭)을 둔다. 첫 줄은 2026-10-24 모듈 정리(에셋을 GameFrameWork 로 · 렌더 자체 시험) | 새 문서 유지 규칙(Engine-36 수정안 6) |
| 10 | (Engine-51 단계마다) 4 · 5 · 7절과 그림 05 · 06 · 07 · 09 | 조명(빛 상수 · 빛 컴포넌트) · 머터리얼(표준 Lit · sRGB · 셰이더 캐시 · 양면) · 스켈레탈(`PSkeletalMesh` · 스키닝)이 들어가면 그 단계 끝에 함께 고친다 | 3D 렌더링 계획(Engine-51)이 이 절들을 바꾼다. 미리 고치지 않고 코드가 바뀔 때 맞춘다 |

수정안 3 의 예제. 둘 다 엔진 저장소의 검증 게임에서 줄여 옮겼다.

```cpp
// 게임 UI 이미지 — Tools/GameFrameWork/gameui_demo_testgame/GameUIDemoHUD.cpp:90-95
PString cardPath;
HFileHelper::CombinePath(HFileHelper::EngineContentDirectory(), PString(cardFiles[i]), &cardPath);
PSharedPtr<PGameUIImage> card = AddChild<PGameUIImage>(PName(PString::Format("Card%d", i)));
card->SetTexture(ui->LoadTexture(cardPath));      // LoadTexture 가 IRawTexture 를 돌려준다
```

```cpp
// 절차 생성 메시 — Tools/GameFrameWork/gfw_phase2_testgame/GfwViewEntryActor.cpp:34-53
HList<HVertex> vertices;
HVertex vertex;
vertex.Position = corners[i];
vertex.Texcoord = uvs[i];
vertex.Normal   = HVector3(0.0f, 1.0f, 0.0f);
vertices.push_back(vertex);
PSharedPtr<JGStaticMesh> mesh = Allocate<JGStaticMesh>();
mesh->SetData(names, vertexLists, indexLists);    // 컴포넌트는 JGStaticMeshComponent::SetStaticMesh 로 받는다
```

## 확인한 것(코드)

| 문서 주장 | 근거 |
|---|---|
| Graphics 는 Core 에만 의존, 사용 모듈 넷 | `Graphics.module.json` `ModuleDependencies: [ "Core" ]`. `"Graphics"` 를 의존에 둔 모듈: GUI · GameFrameWork · GameHost · JGEditor |
| 세 계층(장면 → API → DX12) · 모듈이 API 생성 · 프레임 단계 예약 | `JGGraphics.cpp:23-54`(StartupModule · ScheduleByFrame GraphicsBegin/End) |
| GPU 리소스 인터페이스가 `IJGGraphicsObject` 상속 | `Texture.h:8` · `VertexBuffer.h:14` · `IndexBuffer.h:14` · `ConstantBuffer.h:5` · `StructuredBuffer.h:5` · `Material.h:142` · `Mesh.h:10` · `FrameBuffer.h:32`. 예외 `Shader.h:7 · 25`(수정안 1) |
| 프레임 인덱스별 커맨드 리스트 · 같은 인덱스 완료만 대기 · 미룬 해제 | `CommandQueue.h:23-25`(FramesInFlight 2), `DirectX12API.cpp:348`(DestroyCommittedResource) |
| 업로드 · 리드백 요청은 어느 스레드에서나, 리드백 콜백은 다음 BeginFrame | `ResourceStagingManager.h:13-16 · 46 · 67-79` |
| 머터리얼 템플릿 치환 · PSO 캐시 · 기본 텍스처 · 기본 머터리얼 | `DX12Material.cpp:418-506`, `DirectX12API.cpp:138-181 · 380-388` |
| 장면 · 렌더러 · 세 패스 · 디스플레이에서 한 번만 sRGB | `Scene.h:56-82`, `SceneRenderer.cpp:85-106 · 28-35` |
| 그리기 세 종류 · 2D 는 내장 셰이더 | `DX12GraphicsCommand.cpp:139-347`, `Content/Shaders/draw2d.hlsl` |
| 렌더 메시 · 서브메시 · 기본 머터리얼 대체 | `StaticMesh.h:13-51`, `Mesh.h:18-19` |
| 최소화 중 Present 만 멈춤 · VSync | `DX12FrameBuffer.cpp:118-148`, `JGGraphicsAPI.h:54-57` |
| GUI 백엔드만 바깥에서 DX12 내부를 씀 · OnResize 직접 구독 | `Source/` 전체에서 DirectX12 헤더를 include 하는 Graphics 밖 파일은 `GUI/Backends/DX12GUIBackend.h/.cpp` 뿐이다. `JGGraphics.cpp:50-51` |
| 렌더 자체 시험 | `SelfTest/RenderSelfTest.cpp`(판정 16가지), `JGGraphics.cpp:111-128` |
| 출처 42개 · 그림 11장 · 링크 3개 | 모두 있다(경로 존재 검사) |
