# Graphics 모듈 할 일 목록 (순차 진행용)

작성 2026-09-16. 근거는 `Document/GraphicsStatus_2026-09-16.html`과 `Document/Memory/2026-09-16_Graphics_현황분석.md`.
위에서부터 순서대로 진행한다. 앞 단계가 끝나야 다음 단계의 결과를 확인할 수 있게 배열했다.
한 항목이 끝나면 `[x]`로 바꾸고, 한 단계(Phase)가 끝나면 `Document/Memory/`에 진행 상황을 한 줄 추가한다.
파일:행 표기는 2026-09-16 소스 기준이므로 편집 후에는 어긋날 수 있다.

---

## Phase 0. 준비 · 안전장치

- [x] **0-1. WIP 커밋으로 현재 상태 고정** — 완료 (사용자가 직접 커밋: `bd98a67` 2026-09-15 "다시한번 해보자." 스테이징 77개 파일, `04d9247` 2026-09-16 "현황 파악" CLAUDE.md·Document/)
  스테이징된 77개 파일(Graphics 21, Devkit 신규, GUI ImPlot, Core 메모리, Shader 1)과 `CLAUDE.md` 2개, `Document/`를 커밋한다. 7개월치 작업이 워킹 트리에만 있다.
  완료 조건: `git status`에 의도하지 않은 변경이 남지 않음.

- [x] **0-2. 프로젝트 파일 재생성 후 현재 상태 그대로 빌드 시도** — 완료 (2026-09-16 16:00). JGHeaderTool·JGBuildTool로 코드젠·프로젝트 파일 갱신 후 DevelopEngine x64 전체 빌드.
  결과: Graphics 오류 10건(예측 4건 + Material.h 전방 선언 누락 1건과 그 파생 4건), 경고 6건(C4819 ×4, C4244 ×2). Core·Asset·Game·GameFrameWorks·프로그램 4종은 빌드 성공, Graphics 실패로 GUI·Devkit·DevConsole·DevStatistics·JGDev_Graphics·JGEditor·AI는 건너뜀. Devkit 자체 오류 여부는 Graphics 복구 후 확인. 로그: `Document/Memory/build_2026-09-16_DevelopEngine_console.log`
  `GenerateProjectFiles.bat` → `Temp/ProjectFiles/JGEngine.sln`을 DevelopEngine 구성으로 빌드해 실제 오류 목록을 받는다. 예측한 컴파일 오류 4건 외에 다른 오류가 있는지 확인한다. Devkit도 2025-05-07(DevScene.cpp), 2025-11-01(DevFeature.h, DevSettings.h) 수정분이 마지막 빌드(2025-04-08) 이후라 함께 확인한다.
  완료 조건: 오류 목록 확보. 이 문서의 Phase 1 항목과 대조.

---

## Phase 1. 빌드 복구 (컴파일 오류 10건 · 실제 빌드로 확인)

- [x] **1-0. `IConstantBuffer` 전방 선언 추가** — `Classes/Material.h:7` — 완료 2026-09-16 16:20. Graphics 빌드에서 Material.h 169행·DX12Material.h 103행 오류 4건 소멸 확인.
  `IRawMaterial::GetConstantBuffer()`가 `PWeakPtr<IConstantBuffer>`를 반환하는데 Material.h에 `class IConstantBuffer;`가 없다. DX12Material.h가 Material.h를 먼저 포함해 C2065/C2923/C2955(169행)와 DX12Material.h:103 C2555(공변 반환 불일치)가 연쇄 발생. 전방 선언 한 줄로 4건 해소.

- [x] **1-1. `PDX12GraphicsShader` 재설계** — `DirectX12/Classes/DX12Shader.h:111-145`, `DX12Shader.cpp:163-236` — 완료 2026-09-16 16:20. `IMemoryObject` + `IRawGraphicsShader` 다중 상속, `_byteCodes` 맵과 `StoreCompiledByteCodes()`/`SetByteCode()`/`GetByteCodes()`/`GetByteCode()`/`HasByteCode()`/`IsValid()`/`Reset()` 추가. 보조 변경: `Classes/Shader.h`의 `IRawShader`에 가상 소멸자와 `IsValid()` 순수 가상 추가(`Cast<>`의 dynamic_cast를 위해 다형 타입 필요). Graphics 빌드에서 DX12Shader.cpp 컴파일 통과, 남은 오류 6건은 1-2~1-5 항목.
  `IMemoryObject` 상속 추가(현재 `IRawGraphicsShader`만 상속해 `Allocate<>` 내부 assert에 걸림, `Core/Memory/Memory.h:567`).
  컴파일 결과 바이트코드 `HHashMap<EShaderDomain, HList<uint8>>`를 보관하는 멤버와 Getter 추가. 컴파일러 멤버는 유지.
  완료 조건: `Allocate<PDX12GraphicsShader>()`가 가능하고 바이트코드를 꺼낼 수 있음.

- [x] **1-2. `PDX12Material` 생성자와 컴파일 결과 저장 수정** — 완료 2026-09-16 16:45. 설계 변경 포함: `GetCompiler()`(생 포인터 반환)와 `StoreCompiledByteCodes()`를 제거하고 컴파일을 셰이더 객체의 연산으로 이동. `IRawGraphicsShader::Compile(const HGraphicsShaderCompileArguments&, PString*)` 순수 가상 추가(`Classes/Shader.h`), `PDX12GraphicsShader::Compile()`이 지역 컴파일러로 컴파일 후 `_byteCodes`만 남김. 머터리얼의 `_shaderBtDatas`·`HDX12CompileConfig`·컴파일러 전방 선언 제거, `compileShader()`는 `_graphicsShader->Compile(args, &error)` 한 줄로 정리. 함께 고친 것: `PDX12ShaderCompiler::compile()`의 매크로 문자열 수명(임시 PString 포인터 댕글링)과 `{nullptr,nullptr}` 종료 항목, `error`가 null일 때 실패를 true로 반환하던 버그, `errorData` null 역참조. Graphics 빌드 결과 오류 5건 잔존(DX12GraphicsCommand.cpp 74, 91, 132, 137, 143행) = 1-3~1-5.
  `_graphicsShader = Allocate<PDX12GraphicsShaderCompiler>()` → `Allocate<PDX12GraphicsShader>()`.
  `compileShader()`에서 VS/PS 바이트코드를 `_graphicsShader`에도 저장(또는 `_shaderBtDatas`를 셰이더 객체로 옮기고 머터리얼은 참조만).
  완료 조건: `GetShader().Pin()`으로 바이트코드 접근 가능.

- [ ] **1-3. 상수 버퍼 다운캐스트 수정** — `DirectX12/DX12GraphicsCommand.cpp:132`
  `Pin()` 결과는 `PSharedPtr<IConstantBuffer>`. `Cast<PDX12ConstantBuffer>(...)`로 바꾸거나, `BindConstantBuffer`가 `IConstantBuffer`를 받으므로 캐스트 없이 그대로 넘긴다.

- [ ] **1-4. `BindShader` 호출과 본문 완성** — `DirectX12/DX12GraphicsCommand.cpp:137`, `:245-251`
  137행을 `BindShader(dx12Material->GetShader().Pin())` 형태로 완성.
  `BindShader()` 본문에서 `_graphicsPSO->BindShader(바이트코드)` 호출. 입력 레이아웃과 토폴로지는 Phase 2에서 도메인별로 처리.

- [ ] **1-5. 디스크립터 핸들 비교 수정** — `DirectX12/DX12GraphicsCommand.cpp:74, 91`
  `JG_CHECK(rtvHandle != 0)` → `JG_CHECK(rtvHandle.ptr != 0)`. dsvHandle도 동일.

- [ ] **1-6. 소스 인코딩 경고(C4819) 정리** — `Source/Programs/JGBuildTool/Template/BuildTemplate.lua`
  DX12GraphicsCommand.cpp/.h, DX12Material.cpp/.h가 BOM 없는 UTF-8이라 컴파일러가 CP949로 읽는다. 템플릿의 세 Config 함수에 `buildoptions { "/utf-8" }`를 추가하고 JGBuildTool로 jgengine.lua를 재생성한다(jgengine.lua는 생성물이므로 직접 고치지 않음). 대안은 네 파일을 UTF-8 BOM으로 저장.

- [ ] **1-7. 빌드 성공 확인**
  완료 조건: DevelopEngine 구성 빌드 error 0, `Bin/DevelopEngine/Graphics.dll` 갱신. 실행해서 기존처럼 DevScene 빨간 렌더 타깃이 뜨는지 확인(회귀 없음).

---

## Phase 2. 첫 드로우 전 필수 수정 (Scene 도메인 기준)

- [ ] **2-1. 플레이스홀더 상수 통일** — `Classes/ShaderLibrary.cpp:4-6`, `Classes/ShaderLibrary.h:8-10`
  `__PS_SURFACE_Content_SCRIPT__` → `__PS_SURFACE_CONTENTS_SCRIPT__`, `__PS_CONSTANT_BUFFER_Content_SCRIPT__` → `__PS_CONSTANT_BUFFER_CONTENTS_SCRIPT__`. Scene용 `__PS_SCENE_CONTENTS_SCRIPT__` 상수 추가.
  `PDX12Material::generateShaderCode()`(`DX12Material.cpp:348`)에서 도메인에 따라 Surface/Scene 플레이스홀더를 선택해 치환.
  완료 조건: 생성된 `_fullShaderCode`에 `__PS_` 문자열이 남지 않음(로그로 덤프해 확인).

- [ ] **2-2. 템플릿 HLSL 오류 수정** — `Source/Shader/graphics_shader_template.hlsl`
  164행 `float Depth = SV_TARGET3;` → `float Depth : SV_TARGET3;`. 51행의 cbuffer 밖 중복 플레이스홀더 제거.
  (선택) `scene_shader.hlsl`은 템플릿 53~113행과 중복이므로 삭제 또는 용도 결정.

- [ ] **2-3. 머터리얼 도메인 저장과 매크로 전달** — `DX12Material.h/.cpp` (매크로 배열 종료 항목·문자열 수명은 1-2에서 처리 완료, 남은 것은 도메인 저장과 `compileArgs.Macros`에 `MATERIAL_DOMAIN_SCENE=1` 전달)
  `HRawMaterialConstructArguments::Domain`을 `PDX12Material::Initialize()`에서 멤버로 저장(현재 무시됨).
  `compileShader()`에서 Domain이 Scene이면 매크로 `MATERIAL_DOMAIN_SCENE=1` 전달.
  `PDX12ShaderCompiler::compile()`의 `D3D_SHADER_MACRO` 배열 끝에 `{nullptr, nullptr}` 종료 항목 추가(현재 없음).
  완료 조건: Scene 도메인 머터리얼이 `#if MATERIAL_DOMAIN_SCENE` 분기로 컴파일됨.

- [x] **2-4. `IsValid()` 의미 수정** — 1-2에서 함께 처리 완료 2026-09-16. `_materialConstantBuffer.IsValid() && _materialConstantBuffer->IsValid()`로 변경. 셰이더 컴파일 여부는 `GetShader()->IsValid()`로 분리. `SetName()` 조건은 중복이지만 무해하여 유지.
  `_shaderBtDatas.empty()` 반환(반전)을 `_materialConstantBuffer.IsValid() && _materialConstantBuffer->IsValid()` 등 리소스 유효성 기준으로 바꾼다.
  `Compile()`이 `_bNeedCompile`로 재컴파일 가능한지 확인. `SetName()`의 조건도 함께 점검.

- [ ] **2-5. 상수 버퍼 크기 처리** — `Classes/Material.h:66-132`, `DX12Material.cpp:316-346`
  프로퍼티 총 크기를 16의 배수로 올림(정의기 마무리 패딩 또는 `AlignUp`). `updateMaterialConstantData()`의 `IsAligned` assert(330행)가 float 하나로도 걸리는 문제 해결.
  프로퍼티가 0개일 때 크기 0 버퍼를 만들지 않도록 최소 크기(256바이트) 보장. 현재는 `CD3DX12_RESOURCE_DESC::Buffer(0)` 생성 실패 후 null `Map`으로 크래시한다.
  `_materialConstantPropertyList`가 재컴파일마다 누적되는 것도 `clear()`로 정리.

- [ ] **2-6. 렌더 타깃 포맷 개수와 DSV 처리** — `DX12GraphicsCommand.cpp:49-100`, `DirectX12/Classes/PipelineState.cpp:25-49`
  `SetRenderTarget`이 빈 슬롯까지 8개 포맷을 넣어 `BindRenderTarget`의 `cnt >= 8` 오류 로그가 매번 찍힌다. 유효한 RT 수만큼만 채운다.
  깊이 텍스처가 없으면 미초기화 `dsvHandle` 주소 대신 `nullptr`을 넘긴다(58, 99행).

- [ ] **2-7. 드로우 호출 정리** — `DX12GraphicsCommand.cpp:110-147`
  `Draw(4)` → `Draw(6)`(템플릿 `gTexCoords` 6개).
  `cmdList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST)` 호출 추가. Scene 도메인은 `SV_VertexID`만 쓰므로 입력 레이아웃 없음.
  `JG_CHECK(_graphicsPSO->Finalize())`(122행)를 `bool ok = Finalize(); JG_CHECK(ok);`로 분리(Release에서 assert가 사라짐).

- [ ] **2-8. 기본 텍스처 · 기본 머터리얼 생성** — `JGGraphicsAPI.h:34-39`, `DirectX12/DirectX12API.cpp:16-71`
  `_defaultTexture`, `_defaultMaterial`이 `PJGGraphicsAPI`의 private 멤버라 파생 클래스에서 대입할 수 없다. `protected`로 바꾼다.
  `PDirectX12API::Initialize()` 끝에서 1×1 흰색 텍스처(`InitializeByMemory`)와 프로퍼티 없는 Surface 머터리얼을 생성·컴파일해 대입.
  완료 조건: `GetDefaultMaterial()`, `GetDefaultTexture()`가 null이 아님.

- [ ] **2-9. 빌드 · 실행 회귀 확인**
  완료 조건: 빌드 error 0, 실행 시 DevScene 표시 정상, D3D12 디버그 레이어 오류 없음.

---

## Phase 3. 첫 드로우 · Scene 도메인 풀스크린

- [ ] **3-1. 테스트 머터리얼 생성** — `Source/Runtime/Devkit/DevScene.cpp`
  `HRawMaterialConstructArguments{ Name, Domain = Scene, 빈 정의기 }` → `GetGraphicsAPI().CreateRawMaterial()` → `Compile({ ShaderCode: "_output.final = float4(1.0, 0.5, 0.0, 1.0);" })`.
  완료 조건: `Compile()`이 true 반환, 실패 시 로그의 FXC 오류 메시지로 템플릿 수정.

- [ ] **3-2. 매 프레임 드로우 시퀀스** — `DevScene.cpp`
  `OnUpdate` 등 매 프레임 지점에서: `ClearTexture(SceneTexture)` → `BeginDraw()` → `SetRenderTarget({ RenderTextures[0] = SceneTexture, Viewports, ScissorRects })` → `SetRenderPassData(단위 행렬)` → `Draw(HSceneDrawArguments{ material })` → `EndDraw()`.
  `GetGraphicsCommand()`가 호출마다 새 `PDX12GraphicsCommand`를 만들므로(`DirectX12API.cpp:108-112`) DevScene에서 한 번 받아 보관하거나 API 쪽에서 캐싱.

- [ ] **3-3. 결과 확인**
  완료 조건: DevScene 이미지가 클리어 컬러(빨강)가 아닌 머터리얼 출력색(주황)으로 보임. 디버그 레이어 경고 없음. 막히면 PIX 캡처.

- [ ] **3-4. 커밋** — "Scene 도메인 첫 드로우"

---

## Phase 4. Surface 도메인 메시 드로우

- [ ] **4-1. `IMesh` 인터페이스 확정** — `Classes/Mesh.h`, `Classes/StaticMesh.h`
  `JGStaticMesh`가 `IMesh`를 구현하도록 하고(현재 미구현이라 `HDrawArguments::Mesh`에 담을 수 없음) 서브메시 수, 서브메시별 VB/IB/머터리얼 접근 API를 `IMesh`에 정의.

- [ ] **4-2. `Draw(HDrawArguments)` 구현** — `DX12GraphicsCommand.cpp:110-117`, `:208-225`
  서브메시 순회: `BindVertexBuffer`(스트라이드 0 → `vertexSize`, 222행), `BindIndexBuffer`, 머터리얼 텍스처/CB 바인드, `BindShader`, `_graphicsPSO->BindInputLayout(HVertex::GetInputLayout())`, `SetPrimitiveTopology`, `cmdList->DrawIndexed(indexCount)`.

- [ ] **4-3. 깊이 텍스처와 클리어 API** — `DevScene.cpp`, `JGGraphicsCommand.h`
  `D24_Unorm_S8_Uint` + `Allow_DepthStencil` 텍스처를 만들어 `HRenderTarget.DepthTexture`에 설정. `IJGGraphicsCommand`에 깊이 클리어 API가 없으므로 `ClearDepthTexture` 추가(`PGraphicsCommandList::ClearDepthTexture`는 이미 있음).

- [ ] **4-4. 임시 카메라** — `DevScene.cpp`
  `PCamera`가 빈 클래스라 DevScene에서 LookAt/Perspective로 View/Proj를 계산해 `HRenderPassCBData` 전체(역행렬, 해상도, Near/Far, EyePosition)를 채운다.

- [ ] **4-5. Surface 출력 대상 결정**
  템플릿 Surface 경로는 MRT 4장(Albedo, Normal_Metallic, Specular_Roughness, Depth)을 출력한다. 선택지: (a) G버퍼 4장을 만들고 Scene 도메인 머터리얼로 Albedo를 SceneTexture에 합성(디퍼드 초석), (b) 테스트용 매크로로 SV_TARGET0만 출력. (a)를 권장하되 첫 확인은 (b)로 빨리 가도 된다.

- [ ] **4-6. 메시 에셋 로드**
  `Content/TempAsset/Sample.jgasset`(X Bot, 36MB JSON)을 AssetDatabase로 로드하거나 `Content/RawResources/X Bot.fbx`를 `JGFBXAssetImporter`로 재임포트. 로드 API 확인 필요.

- [ ] **4-7. 텍스처 프로퍼티 슬롯 할당** — `DX12Material.cpp:383-404`, `:141-153`, `:232-242`
  `@TODO` 구현: Texture/TextureCube 프로퍼티마다 `_materialTextures`에 기본 텍스처를 넣고 NameMap 등록, CB의 `int` 필드에 인덱스 기록. `SetTexture`/`GetTexture` 구현.
  동적 디스크립터 힙 크기(`PDynamicDescriptionAllocator` 기본 1024)가 루트 시그니처 테이블 범위(10240)보다 작다. 텍스처를 실제로 쓰기 시작하면 힙 크기를 맞춘다.

- [ ] **4-8. 결과 확인 · 커밋**
  완료 조건: X Bot 메시가 DevScene에 깊이 테스트된 상태로 보임.

---

## Phase 5. 첫 드로우 이후 정리 · 기반 개선

- [ ] **5-1.** `PDX12Texture::AccessPixels` readback 버퍼 구현(`DX12Texture.cpp:42-53`, DEFAULT 힙 `Map` 불가). `JGTexture::WriteJson` 픽셀 저장 복구.
- [ ] **5-2.** FBX 텍스처 쓰기: 포맷을 `R8G8B8A8_Unorm`으로(`Importers/FBXAssetImporter.cpp:449`), `WriteTexture` 호출 활성화(140행).
- [ ] **5-3.** 로그 포맷 통일: `{0}` 스타일 → `%s`(`DirectX12/Classes/ResourceStateTracker.cpp:197, 203, 214, 246, 249` 등).
- [ ] **5-4.** 셰이더 컴파일러: FXC 유지 시 `CSTarget`을 `cs_5_1`로(`DirectX12Define.h:77`), 또는 DXC 전환(`DX12Shader.cpp:100-161` 주석 코드 참고).
- [ ] **5-5.** 프레임 파이프라이닝: `CommandQueue::Begin()`의 매 프레임 펜스 대기 제거, 프레임별 커맨드 할당자/펜스 값 도입.
- [ ] **5-6.** 정적 메시 VB/IB를 DEFAULT 힙으로 업로드(`CommandList::CopyBuffer` 활용).
- [ ] **5-7.** `PCamera`, `PScene` 설계와 구현(`Classes/Camera.h`, `Classes/Scene.h` 현재 스텁).
- [ ] **5-8.** `EndDraw()` 정리, `_renderPassConstantBuffer` 제거 또는 사용.
- [ ] **5-9.** 커밋 정리: WIP 커밋을 의미 단위로 나눌지 결정.
