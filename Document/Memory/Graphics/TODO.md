# Graphics TODO

- 갱신 2026-10-01 14:00 (5-14 출력 색 공간 (a) 8비트 sRGB 완료 · 미커밋. 5-30은 `736ddbe`로 커밋됨). 옛 TODO `Files/Graphics_TODO.md`(2026-09-16 작성, 09-29 23:49 마지막 갱신)를 코드 기준으로 통합했다. 항목 번호(Phase N-M)는 그대로 쓴다(다른 트랙 문서가 참조).
- 규칙: 항목이 끝나면 "완료 이력"으로 옮기고 근거(캡처 · 로그 · 커밋)를 적는다. 임시 검증 코드는 `git diff`로 제거를 확인한다. 커밋은 사용자가 한다. 현재 상황과 함정은 `현황.md`.
- 원칙: 지금 구체적 문제가 있는 것만 미완료 표에 둔다. "나중에 필요할지도"는 보류 표(재검토 조건 명시).

## 다음에 할 일 (사용자 2026-09-30: 5-30 · 5-14 외 나머지는 "지금 안 함". 아래는 다음에 착수할 때의 순서)
| 순서 | ID | 할 일 | 착수 조건 | 완료 기준 |
|---|---|---|---|---|
| 1 | 커밋 | 5-14 변경 커밋(사용자) | 없음 | 아래 "커밋 체크리스트 (5-14)" |
| 2 | 5-29 | FBX 머터리얼 임포트 + sRGB 텍스처 포맷 + 밉 생성 | 5-2 완료(됨). sRGB 슬롯 규칙은 D-3(10-01) 결과대로: 3D 알베도 = `_SRGB`(샘플링 때 선형으로 풀림), 노멀 · 러프니스 = 선형, 2D · UI 이미지 = UNORM 그대로 | 테스트 FBX(`Files/tools/fbx_embedded_texture/`) 임포트 → 메시 머터리얼이 텍스처 에셋을 참조 → 씬 뷰와 알베도 리드백에 텍스처 색이 보임(단색 255 아님), 밉 체인이 올라가고 리드백 · 저장이 밉을 처리, 오류 · D3D12 메시지 0, PSO 수 고정 |
| 3 | 5-31 | 임포트 진입점 콘솔 명령 `asset.import <fbx> -out=<Content 경로>` | 콘솔 명령 시스템 완료(2026-09-30). 5-29와 순서 무관 | 런처 DevConsole에서 명령 실행 → `Content` 아래 `.jgasset` 생성, `AssetPath` 채워짐(5-20 메시 경로 확인), 다음 실행 시작 로드에서 경고 없이 로드. 메시 · 텍스처 플래그 선택 규칙 문서화 |
| 4 | 5-32 | DevScene 창(`JGDevFeature`)을 여는 메뉴 항목 | 없음 | 메뉴에서 열고 닫을 수 있음. 지금은 imgui.ini `[JGWidget][Open]`에 `JGDevFeature=1`을 넣어야만 열린다(검증 절차가 이걸 쓴다) |
| 5 | 5-25 | DevScene 리드백 덤프 → `devscene.readback` 명령 | 자동 검증 루프의 PNG 확보 방법 결정(런처 실행 인자로 명령 실행 / 환경 변수 유지) | 보통 실행에서 `Bin/DevelopEngine`에 PNG가 생기지 않음, 명령 실행 시 `-path` 위치에 저장, `현황.md` §6 검증 절차 갱신 |

## 미완료 항목
| ID | 항목 | 우선순위 | 상태 | 선행조건 | 완료 기준 | 비고 |
|---|---|---|---|---|---|---|
| 5-29 | FBX 머터리얼 임포트 — `FBXAssetImporter.cpp:171` `ReadMaterial` 주석 상태라 5-2로 저장한 텍스처를 참조하는 곳이 없다. assimp 머터리얼의 텍스처 경로(임베디드도 원본 경로 문자열)를 `makeTextureAssetName` 규칙으로 텍스처 에셋에 연결. 외부 파일 텍스처는 범위 밖. 함께: (1) `ETextureFormat`에 `R8G8B8A8_Unorm_Srgb`(DXGI `_UNORM_SRGB`, `JGENUMMETA`, 헤더 툴 재실행) 추가, 디퓨즈 = sRGB · 노멀/러프니스 = 선형 (2) 밉 생성(임포트 때 CPU 또는 로드 뒤 GPU)과 업로드 · 리드백 · 저장의 밉 처리 | 높음 | 미착수 | 5-2(완료) | Top 2 참조 | 색 공간은 5-14(10-01 완료)로 정해짐: 출력 sRGB, 3D 선형 계산 |
| 5-31 | 임포트 진입점 — `JGFBXAssetImporter`를 부르는 코드가 엔진에 없다(5-2 검증은 DevScene 임시 스니펫). `HAutoConsoleCommand` 전역 변수로 선언(`Core/ConsoleCommand/ConsoleCommandGlobalSystem.h` 상단 예시), 문법 `-name=value`(공백 경로는 따옴표). 저장 위치 규칙: `Content` 아래여야 `AssetPath`가 채워진다 | 중 | 미착수 | 콘솔 명령 시스템(완료) | Top 3 참조 | 선언 위치는 Graphics 모듈 .cpp(임포터가 Graphics에 있음) |
| 5-32 | DevScene 창(`JGDevFeature`)을 여는 메뉴가 없다 — `736ddbe`의 JGEditor는 `JGDevFeature`를 시작 때 열지 않고 메뉴도 없어 imgui.ini 저장 상태로만 다시 열린다(커밋된 imgui.ini는 `[JGWidget][Open]`이 비어 있음). 기본 실행에서 DevScene 리드백 검증이 돌지 않는다 | 중 | 미착수 | 없음 | `Windows/...` 메뉴로 열고 닫음, 검증 절차(현황 §6)에서 imgui.ini 수정 단계를 뺀다 | 메뉴 등록 위치는 Devkit 모듈(DevConsole · DevStatistics처럼 `AddMainMenuItem`). 지금 우회: `JGDevFeature=1` 줄 추가 후 실행, 끝나면 되돌림 |
| 5-25 | DevScene 리드백 덤프 정리 — 메시 로드 30프레임 뒤 `Bin/DevelopEngine/DevScene_Readback_Albedo_Async.png` · `DevScene_Readback_Scene_Immediate.png`를 매 실행 기록(두 파일은 `fcace8f`(09-28)부터 git 추적 파일이라 내용이 바뀌면 매 실행 diff가 생긴다). `stb_image_write` 구현이 `DevScene.cpp`에 있음. 명령으로 바꾸면 DevConsole `Submit`이 프레임 밖이라 Immediate 경고가 안 난다 | 중 | 미착수 | PNG 확보 방법 결정(런처 실행 인자 경로는 DevConsole 트랙 범위 밖) | Top 5 참조 | 자동 검증 루프(§6)가 이 PNG를 비교에 쓴다 |
| 5-19 | JGHeaderTool: `JGENUMMETA()` 없는 열거자 이름 누락 — `HeaderTool.cpp:444, 840`이 메타 없는 줄을 건너뛰어 `GetEnumNameByValue()`가 NAME_NONE. 우회: 이름을 쓰는 열거형은 모든 항목에 `JGENUMMETA()`(`JGGraphicsDefine.h:52` 주석). 09-21에 탭 제거 · `};` 종료 두 건은 고침 | 낮음 | 부분 완료 | 없음 | 메타 없는 열거자도 이름 목록에 들어감. `Temp/CodeGen` 재생성 뒤 `ETextureFilterMode` 등의 `JGENUMMETA()` 제거해도 샘플러 이름 정상, 전체 빌드 · 60초 실행 오류 0 | 소유는 Programs/JGHeaderTool. 클래스 분석 모드도 같은 "다음 토큰까지 읽기" 구조라 비슷한 덮어쓰기 가능성 |
| 5-20 | `Content/TempAsset/Sample.jgasset` `AssetPath` "(null)" — 로드마다 `[warning][Asset] NOT Support Asset Path`. `HAssetPath::ReadJson`이 빈 값/"(null)"을 조용히 미설정으로 다루게 하고, 재임포트 시 메시 경로가 채워지는지 확인(텍스처는 5-2에서 확인됨, 메시 `WriteMesh`는 같은 방식이라 채워질 것 — 미확인) | 낮음 | 미착수 | 5-31(재임포트 경로) 또는 `Sample.jgasset` 재저장(D-10, 보류) | 시작 로드 경고 0, 재임포트 메시의 `AssetPath` 채워짐 | 09-30 5-30 검증에서 이름 = 에셋 경로로 다시 저장하면 `AssetPath`가 채워지고 다시 로드할 때 경고가 없음을 확인(`Files/2026-09-30_Phase5-30_저장형식.md`). 재저장하면 `_bounds`(Trace "no saved bounds")도 함께 사라짐 |

## 보류 (구체적 문제 발생 시 재검토)
| ID | 항목 | 재검토 조건 | 비고 |
|---|---|---|---|
| 5-30 (2) | 저장 시 GPU 왕복 제거 — `JGTexture::WriteJson`이 픽셀을 `ReadbackTextureImmediate`로 GPU에서 다시 읽어 압축한다(임포트 저장 = CPU → GPU → CPU). CPU 사본(압축 바이트)을 들고 있다가 그대로 쓰면 없어진다 | 사용자 2026-09-30 "지금 안 함". 임포트를 많이 돌려 저장 시간이 문제 되거나, GPU 없이(도구 · 서버) 저장해야 할 때 | 1024² 저장 430.9ms(리드백 + zlib 포함, 09-30 측정). 텍스처마다 CPU 사본을 들면 런타임 메모리가 늘어나므로 에디터 빌드 한정 여부를 함께 정한다(5-26 (4)와 같은 문제) |
| D-10 | `Content/TempAsset/Sample.jgasset` 새 형식으로 재저장 — 36MB → 2.9MB, 시작 로드 1.2s → 0.28s, 경고 2줄(`AssetPath` "(null)", `no saved bounds`) 제거 | 사용자 2026-09-30 "지금 안 함". 시작 로드 시간이나 36MB 파일이 문제 될 때 | 절차: `Files/tools/format_test_snippet.cpp.txt`로 실행 → `<출력 폴더>/Sample_New.jgasset`을 덮어쓰기 → 60초 실행 · 리드백 PNG 바이트 동일 확인. 옛 형식은 계속 읽힌다 |
| 5-26 (4) | 런타임 빌드에서 로드 → 업로드 요청 → CPU 사본(`HStaticSubMesh::Vertices/Indices`) 해제. 에디터 빌드는 유지 | 런타임 빌드(`DevelopGame`/`ConfirmGame`/`ReleaseGame`)에서 메시 CPU 사본이 메모리 문제로 측정될 때 | 승인된 단계 2 범위 밖이라 안 함. 스테이징 관리자가 요청 시점에 복사하므로 요청 직후 해제 가능. X Bot 기준 약 2MB |
| 5-21 | 머터리얼 재컴파일 API(`Recompile()` 또는 샘플러 배열 동적 인덱싱) — 샘플러 이름이 컴파일 시 박혀 `Compile()` 뒤 다른 필터/랩 모드 텍스처를 `SetTexture`하면 `_bNeedCompile`만 켜지고 갱신되지 않음 | 컴파일 뒤에 텍스처를 바꾸는 코드가 생길 때(지금은 모두 `Compile()` 전에 `SetTexture`) | 규칙 "텍스처는 Compile 전에"로 우회 중. `DX12Material.h:17` 주석 |
| 5-17 | `DirectX12API.cpp:444-470` `HDirectXAPI` 폴백의 함수 지역 static `HHashMap` 4개(09-17 규칙 위반). 폴백 경로가 한 번이라도 돌면 DLL 언로드에서 AV | 실행 로그에 `logFallbackOnce` 폴백 진입 로그가 나타날 때(지금 종료 경로는 폴백을 타지 않음, Memory 4-1 뒤 확인) | Memory_TODO 4-3과 같은 대상. Memory 트랙도 "지금은 안 함" 권장. 고칠 때는 `std::unordered_map` 또는 의도적 leak 힙 객체. 동적 디스크립터 힙 1024 vs 루트 테이블 범위 10240도 텍스처를 많이 쓰기 시작할 때 함께 |
| 5-18 | `HTaskHandle::IsCompelete()`가 `PTask` 파괴 뒤 영구 false(`Task.h:132` 원자 변수 raw 포인터). `GAssetDatabase`는 자체 `bCompleted`로 우회 | 새로 `HTaskHandle`로 완료를 감지하는 코드가 생길 때 | **소유 Memory_TODO 4-4.** 근본 원인(GC가 WeakCount 무시)은 Memory 4-1(2026-09-29)로 해결됨 |
| 5-28 | `GStringTable::removeOldStringInfos`(`StringTable.cpp:167`)가 큐에서 꺼낸 ID를 참조가 살아 있으면 다시 넣지 않아 그 문자열은 영구히 남음 | 이름 문자열 수 · `_stringInfoMap`(128B 풀 블록) 증가가 측정될 때 | **소유 Memory_TODO 7-1(문자열 테이블 수명 정책, 결정 필요).** |
| 5-11 | LNK4098(MSVCRT/MSVCRTD 충돌) — Debug 구성에 릴리스 CRT 서드파티 정적 라이브러리 혼재. 최근 빌드 로그마다 1건 | 서드파티 경계에서 힙 손상 · 크래시가 나거나 경고 0 기준선이 필요해질 때 | 동작 문제 없음. 서드파티 Debug 빌드 준비 또는 `/NODEFAULTLIB` 정리 |
| 5-10 후속 | 컴퓨트 PSO(`PComputePipelineState::Finalize`)가 아직 원본 바이트 해시 | 컴퓨트 셰이더를 만드는 코드가 생길 때(지금 0곳) | 그래픽스 PSO와 같은 `computeGraphicsPSOHash` 방식으로 |
| 5-23 후속 | `HTexturePixels::Data` · DevScene RGBA 변환 버퍼가 `std::vector`(옛 2MB 풀 한도 우회) | 통일이 필요한 이유가 생길 때 | 09-28 풀 재설계 뒤 `HList`로 되돌릴 수 있음. `JGTexture` 작업 버퍼는 5-2에서 되돌림 |
| 전송 관리자 후속 | COPY 전용 큐 | 정적 텍스처를 프레임과 무관하게 읽는 스트리밍이 필요할 때 | `PResourceStagingManager` 안의 백엔드만 바꾼다(큐 하나인 지금은 이득 없음) |
| 종료 크래시 P1 · P3 | P1 `PResourceStateTracker` 종료 시 추적 끄기, P3 `PDirectX12API`를 GC 큐에서 빼고 동기 파괴 | 종료 경로에서 `HDirectXAPI` 폴백이 다시 돌거나 종료 코드 ≠ 0이 재발할 때, 또는 종료 시 D3D12 live 보고를 0으로 만들어야 할 때 | P0(캐시 수명 + null 가드)과 모듈 역순 Shutdown으로 종료 코드 0 · live 0(메모리) 유지 중. 10-01 관찰: 디버거로 돌리면 D3D12가 "Process is terminating. Using simple reporting"으로 `Live Object : 12` · `Live Producer Refcount 2`를 찍는다(장치가 끝까지 놓이지 않음. 5-14 전 11) — P3로 장치를 동기 파괴하면 사라질 보고 |
| 모듈 RefCount | `DisconnectModule`에 참조 카운트 없음. 한 모듈을 두 곳에서 Connect하면 먼저 끊는 쪽이 죽인다 | leaf 모듈이 자기 `StartupModule`에서 `ConnectModule`을 부르기 시작하거나, 에디터와 Dev 툴이 한 프로세스에 같이 뜰 때 | 2026-09-17 사용자 결정으로 안 함. 지금은 호스트 하나(JGEditor)라 중복 연결 없음 |
| GFW 2-1 · 2-2 지원 | `PScene` ID API를 GFW 컴포넌트가 쓰기 시작할 때 생길 요구(활성 카메라 선택, 렌더러 소유 위치, `PSceneRenderer::Render` 호출 지점) | GFW 트랙이 2-1 · 2-2에 착수할 때 | Graphics 코드 변경은 요청이 오면. 설계 `Files/Graphics_PScene_설계방안_2026-09-29.md` |

## 완료 이력
| ID | 항목 | 완료일 | 검증 근거 |
|---|---|---|---|
| 0-1 | WIP 커밋으로 현재 상태 고정 | 2026-09-15/16 | 사용자 커밋 `bd98a67`, `04d9247` |
| 0-2 | 프로젝트 파일 재생성 후 빌드 시도(오류 10건 확보) | 2026-09-16 | `Files/build_2026-09-16_DevelopEngine_console.log` |
| 1-0 | `IConstantBuffer` 전방 선언 | 2026-09-16 | `Files/build_2026-09-16_graphics_step1-1.log`, 커밋 `334052b` |
| 1-1 | `PDX12GraphicsShader` 재설계(`IMemoryObject` + 도메인별 바이트코드) | 2026-09-16 | 〃 |
| 1-2 | `PDX12Material` 생성자 수정, 컴파일을 셰이더 객체로 이동, 컴파일러 버그 3건 | 2026-09-16 | `Files/build_2026-09-16_graphics_step1-2.log` |
| 1-3 | 상수 버퍼 다운캐스트 제거 | 2026-09-16 | `Files/build_2026-09-16_graphics_step1-3to5.log` |
| 1-4 | `BindShader` 호출 · 본문 완성, `Finalize` 순서 | 2026-09-16 | 〃 |
| 1-5 | 디스크립터 핸들 비교 `.ptr != 0` | 2026-09-16 | 〃, Graphics.dll 생성 |
| 1-6 | C4819 정리(`/utf-8`, CP949 소스 27개 → UTF-8) | 2026-09-16 | `Files/build_2026-09-16_graphics_step1-6.log`, 커밋 `c300fd3` |
| 1-7 | 전체 빌드 성공 확인 | 2026-09-16 | 16/16 오류 0, 런처 20초 실행 |
| 2-1 ~ 2-9 | 플레이스홀더 통일 · 템플릿 HLSL 수정 · 도메인 매크로 · `IsValid` · CB 256B · RT/DSV · `Draw(6)` · 기본 텍스처/머터리얼 · 회귀 확인(+ 템플릿 지연 로드, `EngineShaderDirectory`) | 2026-09-16 | `Files/build_2026-09-16_full_phase2.log`, 커밋 `57189c7` |
| 3-1 ~ 3-3 | Screen(구 Scene) 도메인 테스트 머터리얼 · 매 프레임 드로우 · 결과 확인 | 2026-09-16 | `Files/2026-09-16_first_draw_scene_domain.png`, `Files/build_2026-09-16_full_phase3.log` |
| 3-4 | 커밋 "Scene 도메인 첫 드로우" | 2026-09-17 | 사용자 커밋 `60ccb48` "드로우콜 정리"(09-17 00:15, Phase 3 마무리 포함. 옛 TODO는 미완료 표시였음) |
| 4-1 | `IMesh` 확정(에셋/렌더 메시 분리 `PStaticMesh`, POD `HVertexData`) | 2026-09-17 | `Files/2026-09-17_Phase4_Surface메시드로우.md` |
| 4-2 | `Draw(HDrawArguments)` 구현, 행 우선 규약 | 2026-09-17 | 〃 |
| 4-3 | 깊이 텍스처 · `ClearDepthTexture` | 2026-09-17 | 〃 |
| 4-4 | 임시 카메라(LookAt/Perspective, 경계 구 프레이밍) | 2026-09-17 | 〃 |
| 4-5 | Surface 출력 대상 = G버퍼 4장 + 합성 패스 채택 | 2026-09-17 | 〃 |
| 4-6 | 메시 에셋 로드(`GAssetDatabase` 버그 5건 수정 포함) | 2026-09-17 | 〃, 로그 `Success Load Asset` |
| 4-7 | 텍스처 프로퍼티 슬롯 할당, `SetTexture/GetTexture`, 식별자 경계 치환 | 2026-09-17 | 〃 |
| 4-8 | 결과 확인 | 2026-09-17 (음영 정정 09-21) | `Files/2026-09-17_phase4_capture.png`, `Files/build_2026-09-17_phase4.log`, `Files/2026-09-21_phase5_meta_capture.png`, 커밋 `4e12fed` |
| 종료 크래시 P0 | `HDirectXAPI` 캐시 수명 관리 + 래퍼 21개 null 가드, `GetGraphicsAPI()` 캐시 제거 | 2026-09-17 | `Files/2026-09-17_종료크래시_3단계적용.md`, ExitCode 0, `Memory Chunk Shutdown` 완주 |
| 모듈 수명 1~3 | Asset 로그 오타, 순회 중 변경 제거, 연결 순서 기록 · 역순 Shutdown, 데드락 수정, Shutdown 뒤 unregister | 2026-09-17 | `Files/2026-09-17_모듈수명_1-3적용.md`, 커밋 `cd1a1d6` |
| 5-1 | 리드백: `RequestTextureReadback`(비동기) · `ReadbackTextureImmediate`(동기), `HTexturePixels`, BytesPerPixel 메타 | 2026-09-18 (메타 09-21) | `Files/2026-09-18_phase5_readback_albedo_async.png` · `_scene_immediate.png`, `Files/2026-09-18_Phase5_전송관리자.md` |
| 5-6 | 정적 메시 VB/IB DEFAULT 힙(`GPULoad`), 텍스처 업로드도 스테이징 관리자 경로 | 2026-09-18 | `Files/2026-09-18_phase5_capture.png`, `Files/build_2026-09-18_phase5.log`, 커밋 `a71b70a` |
| 5-12 | 버퍼 재설정 순서 버그(VB/IB/SB) | 2026-09-18 | 〃 |
| 5-24 | `PString::Format` 해시 미갱신 | 2026-09-18 | 〃(스테이징 리소스 이름 "(null)" 소멸) |
| 5-13 | `JG_LOG` `%s`에 `PName` 빈 문자열(댕글링), `PName` 자기 대입 | 2026-09-17 | `Files/2026-09-17_Phase4_Surface메시드로우.md` |
| 5-15 | 간헐적 에셋 로드 실패(실제 원인 2건) | 2026-09-17 | 4-6과 함께 |
| 5-16 | 디버그 레이어 로그 드레인 동작 검증(ImGui 뷰포트 무해 경고 출처 확인) | 2026-09-18 | 로그 `D3D12 DebugLayer [1424] ... Wait` |
| 5-10 | PSO 캐시 해시 → `computeGraphicsPSOHash`(사용자 보고 크래시 원인) | 2026-09-21 | `Files/2026-09-21_psofix_capture.png`, `Files/build_2026-09-21_psofix.log`, 60초 PSO 2개, 커밋 `033e988` |
| 5-19 (일부) | JGHeaderTool 메타 키 탭 제거, 열거형 분석 `};` 종료(입력 레이아웃 오프셋 0 버그 해소) | 2026-09-21 | `Files/2026-09-21_phase5_meta_readback_scene.png`(중앙 0.828 → 0.174), `Files/build_2026-09-21_meta.log` |
| 5-2 | FBX 임베디드 텍스처 임포트(stb_image 디코딩, 이름 규칙, `R8G8B8A8_Unorm` 밉 1, 2MB 제한 제거, 헤더 `.generation.h`, `JGTexture::GetType`) | 2026-09-29 | `Files/2026-09-29_5-2_import_capture.png`, `Files/2026-09-29_Phase5-2_FBX텍스처임포트.md`, `Files/build_2026-09-29_5-2.log` · `_5-2_devkit.log`, `Files/tools/fbx_embedded_texture/`, 커밋 `d457b92` |
| 5-3 | 로그 포맷 `{0}` → `%s`/`%d` 10곳, `Json.h` `static_assert` | 2026-09-29 | `Files/2026-09-29_Phase5-3_5-4_5-5.md`, 60초 로그 `{N}` 0줄 |
| 5-4 | `CSTarget` `cs_6_0` → `cs_5_1`(FXC 유지) | 2026-09-29 | `Files/tools/shader_target_probe/` 출력, `Files/build_2026-09-29_5-3_5-4.log` |
| 5-5 | 프레임 파이프라이닝(FramesInFlight 2, `HFrameContext`, `DeferRelease`, GUI SRV 프레임별 구간, 종료 정리) | 2026-09-29 | `Files/2026-09-29_5-5_capture.png`, `Files/build_2026-09-29_5-5.log` · `_5-5_final.log`, GPU 부하 13.7 → 9.9ms, `Files/tools/frame_timing_snippet.cpp.txt`, 커밋 `1308fd5` |
| 5-7 | `PScene` · `HSceneCamera` · `HSceneMesh` · `PSceneRenderer`, `Camera.h` 삭제, `Scene` → `Screen` 개명 | 2026-09-29 | `Files/2026-09-29_5-7_step1_capture.png`, `Files/build_2026-09-29_5-7.log`, 리드백 PNG 바이트 동일, `Files/2026-09-29_Phase5-7_PScene_설계.md`, 커밋 `1308fd5` |
| 5-22 | DevScene 합성 패스 · G버퍼 · 카메라를 `PSceneRenderer`/`PScene`으로 이관 | 2026-09-29 | 〃 |
| 5-26 (1)~(3) | 버퍼 `_shadowData` 제거, CPU 사본은 `HStaticSubMesh`, `HRenderSubMesh`, `_bounds` 임포트 때 저장(`GetBounds`) | 2026-09-29 | `Files/2026-09-29_5-26_capture.png` · `5-26a_capture.png`, `Files/build_2026-09-29_5-26.log` · `_5-26_bounds.log` · `_5-26_final_devkit.log`, `Files/tools/boundstest_snippet.cpp.txt`. (1)(2)는 `1308fd5`, (3)은 `4c8ac73`(09-30 사용자 커밋) |
| 5-23 | 메모리 풀 블록 한도(2MB `out_of_range`) | 2026-09-28 (Memory Phase 2) | `Document/Memory/Memory/Files/Memory_TODO.md` 2-1, `Document/Memory/Memory/Files/2026-09-28_memtest_phase2_results.txt` |
| 5-27 | 풀 고갈이 조용히 nullptr 반환 | 2026-09-28 (Memory Phase 2) | 〃(성장형 클래스, 실패는 Critical + `abort()`) |
| 설계 규칙 09-16 | 스마트 포인터 타입은 `IMemoryObject` 뿌리 하나(오프셋 0) | 2026-09-16 | `Files/2026-09-16_Graphics_현황분석.md` 진행 기록 19:20 |
| 설계 규칙 09-17 | 엔진 컨테이너를 static 저장소에 두지 않음 | 2026-09-17 | `Files/2026-09-17_Phase4_Surface메시드로우.md` 버그 4 |
| 설계 규칙 09-18 | 전송은 `PResourceStagingManager` 한 곳, 요청/실행 분리, 우선순위 버킷, `Request*`/`*Immediate` 이름 | 2026-09-18 | `Files/2026-09-18_Phase5_전송관리자.md` |
| 코딩 규칙 09-21 | 조건문 본문 한 줄 금지(Allman, 탭) | 2026-09-21 | 옛 TODO 항목 |
| 0 (회귀) | 새 PC(`C:\Develop\JGEngine`, RTX 4090 Laptop) 첫 실행 확인 | 2026-09-30 | 19:57 다른 세션의 런처 실행 로그(`Files/2026-09-30_newpc_launcher_jg_log.txt`: 오류 0 · D3D12 0 · PSO 2 · live 0, 리드백 수치 65,587 px · 255 / 0.174, PNG가 커밋된 것과 바이트 동일) + 21:1x 5-30 검증 실행(종료 코드 0). 화면 캡처만 공용 스크립트 옛 경로 문제로 못 함 |
| 5-30 | 에셋 저장 형식: 큰 배열을 base64 문자열로(사용자 결정 (a)안). `PJsonData::AddBinaryMember/GetBinaryData/IsString`, 텍스처 `Pixels`(zlib + base64), 메시 `VertexStride` + `Vertexes` · `Indexes`(base64), 옛 형식(숫자 배열) 읽기 유지. 1024² 노이즈 73.7MB → 5.6MB, X Bot 36.2MB → 2.9MB(로드 1,215 → 278ms). (2) 저장 GPU 왕복 · D-10 재저장은 보류 표로 | 2026-09-30 | `Files/2026-09-30_Phase5-30_저장형식.md`, `Files/2026-09-30_5-30_formattest_jg_log.txt`(텍스처 · 메시 왕복 PASS, 오류 0 · D3D12 0 · PSO 2 · live 0 · 종료 0, 리드백 PNG 바이트 동일), `Files/build_2026-09-30_5-30.log`, `Files/tools/format_test_snippet.cpp.txt`. 사용자 커밋 `736ddbe`(10-01 12:16) |
| 5-14 | 출력 색 공간 (a) 8비트 sRGB(사용자 결정 D-3, 10-01): 프레임버퍼 `R8G8B8A8_Unorm`, `PSceneRenderer` 디스플레이 패스(FP16 선형 → sRGB 인코딩 → `GetDisplayTexture()`), DevScene · SceneViewport는 디스플레이 텍스처를 보여 주고 게임 UI는 그 위에 그림, ImGui PSO 포맷 = 프레임버퍼 포맷, `HGUI::DisplayColor` · 메모리 창 `displayColor` 변환 제거, 메모리 그래프 면 알파 0.035 → 0.10. 톤매핑 곡선은 안 넣음(1.0 넘는 밝기 없음) | 2026-10-01 | `Files/2026-10-01_Phase5-14_출력색공간.md`, `Files/2026-10-01_5-14_60s_run.txt`(오류 0 · D3D12 0 · PSO 3 · live 0 · 종료 0), `Files/2026-10-01_5-14_capture.png`(창 배경 69 → 15, 메뉴 바 105 → 36, 장면 픽셀 그대로), 리드백 PNG 바이트 동일, `Files/build_2026-10-01_5-14.log`. 게임 UI(프로젝트 모드)는 게임 UI 세션이 e2e 20단계 · D3D12 0으로 확인(`GameFrameWorks/Files/2026-10-01_gameui_er_srgb514.png`). 미커밋 |

## 커밋 체크리스트 (5-14, 2026-10-01)
- 소스 9개: `Source/Runtime/Graphics/DirectX12/DirectX12API.cpp`, `Source/Runtime/Graphics/Classes/SceneRenderer.h` · `.cpp`, `Source/Runtime/Devkit/DevScene.cpp`, `Source/Editor/JGEditor/Widgets/SceneViewport.cpp`, `Source/Runtime/GUI/Backends/DX12GUIBackend.cpp`, `Source/Runtime/GUI/GUI.cpp` · `GUI.h`, `Source/Editor/DevStatistics/MemoryStatistics.cpp`. 새 파일 없음(PreBuild 불필요).
- 같은 파일에 다른 트랙의 미커밋 변경이 섞여 있다: `DX12GUIBackend.cpp`(한글 글꼴, GUI BL-3), `GUI.cpp/.h`(DataTable 그리드 등). 파일 단위로 나눠 커밋할 수 없다.
- `Bin/DevelopEngine/*`: 13:33 메인 트리 빌드(오류 0) 결과. 커밋 직전 최신 빌드인지 확인.
- 문서: `Document/Memory/Graphics/`(현황 · TODO · Files 새 파일: `2026-10-01_Phase5-14_출력색공간.md`, `2026-10-01_5-14_60s_run.txt`, `2026-10-01_5-14_capture.png`, `2026-10-01_5-14_capture_jg_log.txt`, `2026-10-01_5-14_60s_crashwalk.txt`(끝나지 않은 crashwalk 실행 기록), `2026-10-01_5-14_60s_jg_log.txt`, `build_2026-10-01_5-14.log`, `tools/dbgevents/` 진단 도구 소스 · 배치), 다른 트랙 문서의 색 규칙 정정(Memory · Etc · GameFrameWorks), `Document/Memory/진행현황.md`.
- `Bin/DevelopEngine/imgui.ini`는 검증 뒤 원래대로 되돌렸다.

## 커밋 체크리스트 (Graphics, 2026-09-30 21:30) — 완료: 사용자 커밋 `736ddbe`(10-01 12:16)에 5-30 소스 4개 포함
- 5-30 소스 4개: `Source/Runtime/Core/FileIO/Json.h` · `Json.cpp`, `Source/Runtime/Graphics/Classes/Texture.cpp` · `StaticMesh.cpp`. 새 파일 없음(PreBuild 불필요).
- `Bin/DevelopEngine/*`: Core 헤더가 바뀌어 거의 모든 DLL이 다시 링크된다. 21:15 메인 트리 빌드는 Core · Graphics까지 성공했지만 GameFrameWorks에서 실패했다(같은 시각 GFW 세션이 만들던 Phase 2 새 파일, PreBuild 전 — 5-30과 무관). 커밋 직전에 메인 트리 전체 빌드 성공을 확인한다(같은 트리의 GameModule R7 · R8, GUI, GFW 변경도 함께 들어간다).
- 문서: `Document/Memory/Graphics/`(현황 · TODO · Files 새 파일 5개: `2026-09-30_Phase5-30_저장형식.md`, `2026-09-30_5-30_formattest_jg_log.txt`, `2026-09-30_newpc_launcher_jg_log.txt`, `build_2026-09-30_5-30.log`, `tools/format_test_snippet.cpp.txt`), `Document/Memory/진행현황.md`, `Document/Graphics_진행보고_2026-09-30.html`.
- `Content/TempAsset/Sample.jgasset`은 바꾸지 않았다(D-10 보류).

## 제외 항목
| ID | 항목 | 이유 |
|---|---|---|
| 5-8 | `EndDraw()` 정리, `_renderPassConstantBuffer` 제거 또는 사용 | `EndDraw`는 렌더 타깃 → PIXEL_SHADER_RESOURCE 전이라는 역할이 Phase 3에서 확정됐고, 렌더 패스 데이터는 `SetRenderPassData`가 리스트의 업로드 할당자로 복사하므로 영구 CB가 필요 없다. 남은 것은 `DX12GraphicsCommand.h:24`의 미사용 선언 1줄 — 동작 문제 없음. 그 파일을 다음에 고칠 때 지운다 |
| 5-9 | 커밋 정리: WIP 커밋을 의미 단위로 나눌지 | 사용자가 직접 작업 단위로 커밋하고 있다(09-16 ~ 09-29 12회). 커밋 단위는 사용자 몫이라 TODO로 관리하지 않는다 |
| 2-2 잔여 | `Source/Shader/scene_shader.hlsl` 용도 결정 | 참조 0, `GShaderLibrary`가 템플릿으로 수집만 한다(로그 "Collect scene_shader"). 삭제만 남았고 문제를 일으키지 않는다. 셰이더 폴더를 다음에 고칠 때 지운다 |
