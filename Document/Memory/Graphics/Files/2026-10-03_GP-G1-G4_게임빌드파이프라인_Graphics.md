# GP-G1 ~ G4 — 게임 빌드 파이프라인의 Graphics 몫 (D-23)

- 작업: 2026-10-02 22:00 ~ 23:10(구현 · 검증 · 메인 반영), 2026-10-03 09:50 ~(G4 뒤 정리 · 회귀). 미커밋.
- 근거: 사용자 결정 D-23(10-02 밤, `진행현황.md` §3) — 작업 지시는 엔진 총괄 세션. 작업 정의는 `DevOps/Files/게임빌드파이프라인_설계_2026-10-02.md` 작업 표 GP-G1 ~ G4.
- 한 줄: 스왑체인 크기 변경을 GUI 백엔드에서 Graphics 로 옮기고(최소화 포함), `PSceneRenderer::Resize` 를 만들고, 최종 텍스처 제출에 크기 가드 · VSync 를 넣고, 셰이더를 `Content/Shaders` 로 옮겼다. 네 항목 모두 완료 기준을 채웠다.

## 1. 결과

| ID | 한 일 | 완료 기준(설계 표) | 결과 |
|---|---|---|---|
| GP-G1 | 창 `OnResize` 구독을 `DX12GUIBackend` → `HJGGraphicsModule` 로. 0 x 0(최소화)이면 버퍼는 그대로 두고 Present 만 건너뜀, 복원 때 같은 크기면 버퍼도 그대로. `ResizeBuffers` 실패를 로그로 남기고 옛 크기 유지 | GUI 없이 창 크기 변경 3회 · 최소화/복원에서 D3D 오류 0, 에디터 크기 변경 그대로 | 충족. GUI 없이(Graphics · Asset 만) 1264x761 → 1584x861 → 984x661, 최소화 · 복원, D3D12 0, 종료 0. 에디터(메인)도 같은 순서로 정상, 캡처 확인 |
| GP-G2 | `bool PSceneRenderer::Resize(uint32, uint32)` — 렌더 대상(G버퍼 4 · 깊이 · 출력 · 디스플레이)과 합성 · 디스플레이 머터리얼을 새 크기로 다시 만든다. 같은 크기면 바로 true, 0 이면 false(이전 그대로) | 크기를 바꾼 뒤 디스플레이 텍스처 크기 = 요청값, 리드백 정상 | 충족. 1920x1080 → 1280x720 → 1920x1080: 텍스처 · 렌더러 크기 = 요청값, 리드백 1280x720 중앙 112 / 0.163(sRGB 인코딩과 맞음), 되돌린 뒤 116 / 0.174 = 원래 값. 새 PSO 없음(캐시 재사용) |
| GP-G3 | `SubmitFinalTexture` 가드(크기 · 포맷이 백 버퍼와 다르면 그 프레임 복사 건너뜀 + Info 로그를 어긋난 구간마다 한 번), `PJGGraphicsAPI::SetVSyncEnabled/IsVSyncEnabled`(기본 꺼짐), Alt+Enter 막힘 확인 | 60Hz 모니터에서 약 60fps, 크기 변경 중 D3D 메시지 0 | 충족. VSync 켬 60.5fps(델 4K 60Hz). 간격 2 시험 31.6fps → 스왑체인이 간격을 지킨다. 어긋난 제출 · 맞는 제출 · 크기 변경 · 디버거 실행 모두 D3D12 · DXGI 0. Alt+Enter 뒤 창 사각형 그대로(`DXGI_MWA_NO_ALT_ENTER` 는 원래 있었음) |
| GP-G4 | `git mv Source/Shader Content/Shaders`(5파일), `HFileHelper::EngineShaderDirectory()` = 모든 모드에서 `EngineContentDirectory()/Shaders`(폴더 자동 생성 없음), 주석 2곳 경로 | 에디터 · 런처 · JGConsole 에서 새 경로, PSO · 리드백 그대로, `Source/Shader` 참조 0 | 충족(10-03 DevOps 실제 쿠킹까지 — §3-10). 런처(에디터) "Start Collect Shader in (../../Content/Shaders)" 템플릿 4개, PSO 3, 리드백 PNG 2장이 이동 전과 바이트 동일, D3D12 0. JGConsole `cookcheck.selftest` 의 엔진 Content(쿠킹 기본 제외 뒤)가 0 → 5파일(셰이더), 오류 · 경고 0. 코드의 `Source/Shader` 참조 0 — 마지막 하나였던 DevOps `CookCommand.cpp` 임시 복사 코드를 DevOps 가 10-03 에 지움(이제 `Content/Shaders` 가 없으면 쿠킹 실패) |

함께 고친 것: Graphics 모듈 종료 때 `WindowCallBacks` 가 이미 지워져 있으면(런처 `GCoreSystem::Destroy` 는 창 콜백을 먼저 지운 뒤 모듈을 내린다) 구독을 빼다가 널 접근으로 죽던 것 → 널 검사(Run A 첫 실행에서 발견, 종료 코드 0xC0000005 → 0).

## 2. 바꾼 파일 (메인 트리, 미커밋)

| 파일 | 내용 |
|---|---|
| `Source/Runtime/Graphics/JGGraphicsAPI.h` | `OnWindowResized`(protected 순수 가상), `SetVSyncEnabled` · `IsVSyncEnabled`(public 순수 가상), `SubmitFinalTexture` 설명 |
| `Source/Runtime/Graphics/DirectX12/DirectX12API.h/.cpp` | 위 세 함수 구현(프레임 버퍼로 전달). `draw2d.hlsl` 주석 경로 |
| `Source/Runtime/Graphics/DirectX12/DX12FrameBuffer.h/.cpp` | `_bMinimized` · `_bVSyncEnabled` · `_bSubmitMismatchLogged`, `Update` 가드, `Present`(최소화면 건너뜀, `Present(VSync ? 1 : 0, 0)`), `Resize`(0 x 0 처리 · HRESULT 확인 · 이름 · Trace 로그), `SetVSyncEnabled` · `IsVSyncEnabled` · `IsMinimized` |
| `Source/Runtime/Graphics/JGGraphics.h/.cpp` | `_windowResizeHandle`, `OnWindowResize`, 시작 때 `OnResize.AddRaw`, 종료 때 널 검사 뒤 `Remove` |
| `Source/Runtime/Graphics/Classes/SceneRenderer.h/.cpp` | `_name`, `Resize` |
| `Source/Runtime/Graphics/JGGraphicsDefine.h` | `draw2d.hlsl` 주석 경로 |
| `Source/Runtime/GUI/Backends/DX12GUIBackend.h/.cpp` | `OnResize` 구독 · 함수 삭제(GUI 세션 동의) |
| `Source/Runtime/Core/FileIO/FileHelper.cpp` | `EngineShaderDirectory()` 블록 하나만(같은 파일의 DevOps D1 변경은 안 건드림) |
| `Source/Shader/*` → `Content/Shaders/*` | git mv 5파일(`R`) |
| `Bin/DevelopEngine/*` | 10-02 23:09 메인 빌드(오류 0, 경고 LNK4098 1) |

ABI 주의: `PJGGraphicsAPI` 가운데에 가상 함수가 들어가 vtable 이 밀렸고 `PSceneRenderer` 크기가 바뀌었다. 엔진 솔루션 밖에서 옛 헤더로 빌드한 모듈이 이 타입을 쓰면 잘못된 함수를 부른다. 10-02 확인 결과 게임 프로젝트 코드에서 `GetGraphicsAPI()` · `PSceneRenderer` · `PJGGraphicsAPI` 사용 0(모두 GFW 경유). worktree 는 메인을 합칠 때 전체 다시 빌드.

## 3. 검증

임시 검증 코드는 격리 worktree(`Temp/wt/gp_graphics`, HEAD `0bd6356`)에만 넣었다. 그 diff 는 `tools/gp_g123_test_snippet.diff.txt`. 창 조작은 `tools/gp_window_test.ps1`(크기 변경 3회 1280x800 · 1600x900 · 1000x700 → 최소화 · 복원 → `-AltEnter` → `-CapturePath` → WM_CLOSE · 종료 코드, `-AttachPid` 로 이미 뜬 프로세스에 붙음).

| # | 실행 | 결과 | 증적 |
|---|---|---|---|
| 1 | Run A — `JG_GPTEST_NOGUI=1`(JGEditor 가 Graphics · Asset 만 연결) | 스왑체인이 클라이언트 크기를 따라감, 최소화 "Present paused" · 복원 "restored (984x661)", D3D12 0, 종료 0, live 0. 첫 실행은 종료 크래시 → 위 종료 널 검사로 해결 | `2026-10-02_GP_runA_nogui_jg_log.txt` |
| 2 | Run B — 에디터 + DevScene + `JG_GPTEST=1` + 창 시험 `-AltEnter` | 프레임 40 Resize(1280,720) true · 텍스처 1280x720 / 50 리드백 112 · 0.163, 같은 크기 true · (0,720) false + 경고 / 55 어긋난 제출 → Info 한 번 / 60 맞는 제출 / 90 해제 / 95 되돌림 / 105 리드백 116 · 0.174. 창 크기 · 최소화 · Alt+Enter 정상, D3D12 0, 종료 0 | `2026-10-02_GP_runB_editor_jg_log.txt`, `2026-10-02_GP_runB_capture.png` |
| 3 | Run C — GUI 없이 VSync 끔 3초 → 켬 3초 → 끔 3초 | 55.0 / **60.5** / 66.9 fps | `2026-10-02_GP_runC_vsync_jg_log.txt` |
| 4 | Run C' — 같은 시험, VSync 켬 = 간격 2(임시, 바로 되돌림) | 69.4 / **31.6** / 56.6 fps → 간격을 지킨다 | `2026-10-02_GP_runC_sync2_jg_log.txt` |
| 5 | Run D — 디버거(`tools/dbgevents`)로 Run B 를 붙여서 | 실행 중 DXGI · D3D12 메시지 0. 종료 때 live 보고(Object 12 · Producer 2)는 10-01 기준과 같음 | `2026-10-02_GP_runD_dbgevents.txt` |
| 6 | 메인 반영 뒤(G1~G3) 에디터 창 시험 | 정상, 캡처 | `2026-10-02_GP_main_editor_jg_log.txt`, `2026-10-02_GP_main_editor_capture.png`, `build_2026-10-02_gp_g123_main.log` |
| 7 | 메인 G4 뒤 에디터 | "Start Collect Shader in (../../Content/Shaders)", 템플릿 4개, 오류 0, D3D12 0 | `2026-10-02_GP_G4_main_editor_jg_log.txt`, `build_2026-10-02_gp_g4_main.log` |
| 8 | 10-03 DevScene 회귀(G4 뒤, 작업 폴더 `Temp/GPTest` — §5-6) | PSO 3, 리드백 0.174 · 65,587 px · 255, PNG 2장이 이동 전(Run B) 과 바이트 동일, D3D12 0, 오류 0, 종료 0 | `2026-10-03_GP_G4_devscene_jg_log.txt`, `2026-10-03_GP_G4_devscene_capture.png` |
| 9 | 10-03 JGConsole `cookcheck.selftest` | 26/26. `[engine_content] 5 file(s), 0 asset(s), 0 error(s), 0 warning(s)`(10-02 Data 검증 때 0 files) | `2026-10-03_GP_G4_cookcheck_jg_log.txt` |
| 10 | 쿡(`-runCook`) 실제 실행 — DevOps M2(10-03 10:1x 받음) | Graphics 세션에서는 못 했다(10-03 오전: 메인에 묶인 ReleaseGame 빌드 · GameHost 가 아직 없었음) → DevOps 가 GameHost 메인 반영 뒤 `cook.run` GameUIDemo 86초 exit 0 — 엔진 Content 5(`Content/Shaders`) · 게임 3, 콘텐츠 검사 오류 0, 스모크 300프레임 exit 0. 같은 때 `CookCommand.cpp` 임시 복사 코드 삭제(셰이더 폴더가 없으면 쿠킹 실패) | `DevOps/Files/2026-10-03_GP-D3-D6_구현_검증.txt` 35행 |

## 4. 쓰는 법 (소비 세션에 보낸 내용)

- `PSceneRenderer::Resize(w, h)`: 메인 스레드, `Render` 밖. 같은 크기면 바로 true(매 프레임 비교해도 싸다), 0 이면 false. 실제로 바뀔 때마다 셰이더 2개를 다시 컴파일한다(PSO 는 캐시) → 창을 끄는 동안은 디바운스. 텍스처 객체가 바뀌므로 `GetDisplayTexture()` · 텍스처 ID 는 매 프레임 다시 받는다. Update 단계에서 불러도 안전하다 — 옛 텍스처는 그 프레임 제출 뒤(다음 갱신의 GC) 상태 추적에서 빠지고 GPU 리소스는 미룬 해제, ImGui 는 그릴 때 디스크립터를 복사한다.
- `SetVSyncEnabled(bool)`: 기본 꺼짐(에디터 그대로), 다음 Present 부터. GameHost 가 `JGPackage.json` Window.VSync / `-novsync` 로 쓴다(DevOps).
- `SubmitFinalTexture`: 크기 · 포맷(R8G8B8A8_Unorm, 클라이언트 크기)이 백 버퍼와 같을 때만 복사. 창 크기를 바꾼 직후 한두 프레임은 건너뛰고 Info 로그를 한 번 남긴다(오류 아님).
- 코드로 창 크기를 바꾸면(`SetWindowPos`) WM_SIZE 가 동기로 와서 그 자리에서 스왑체인을 바꾼다(Flush + ResizeBuffers). 창 메시지는 보통 `UpdateWindow` 단계(GraphicsEnd 뒤)에서 처리되니 프레임 밖이다. 코드로 바꿀 때는 Update 단계나 프레임 밖에서, GraphicsEnd 안에서는 부르지 않는다.
- 최소화 중에도 프레임 루프는 돈다. Present 가 없어 VSync 를 켜도 제한 없이 돈다(이전에도 같음). 쉬게 하려면 호스트 루프에서 클라이언트 0 x 0(또는 `IsIconic`)을 보고 재운다 — DevOps 에 알림.

## 5. 발견 · 함정

1. **종료 순서:** 런처 `GCoreSystem::Destroy` 는 `WindowCallBacks` 를 먼저 지우고 모듈을 내린다. 모듈 `ShutdownModule` 에서 창 콜백을 만지면 널 검사.
2. **이 PC 의 VSync 끔 fps:** 지금 디스플레이(델 4K 60Hz, 내장 240Hz 패널 꺼짐)에서는 GUI 없는 빈 프레임도 VSync 끔이 55~69fps 로 묶인다(원인 미확인). 09-29 ~ 10-01 에 240Hz 내장 패널에서 잰 에디터는 73~129fps. 켬/끔 차이는 간격 2 시험(31.6fps)으로 확인했다. 에디터(DevScene 열림)는 끔에서 약 50fps.
3. **ImGui 보조 창 경고:** 떠 있는(도킹 안 된) ImGui 창이 줄어든 메인 창 밖으로 나가면 ImGui 가 별도 OS 창(보조 뷰포트)을 만들고, 그 첫 프레임에 `imgui_impl_dx12.cpp:1055` `CommandQueue->Wait(fence, 0)` 이 D3D12 경고 [1424]("waiting for a fence value of zero")를 낸다. 5-16(09-18)에서 확인한 무해 경고와 같은 출처. 도킹된 배치로 같은 시험을 돌리면 0. GUI 백엔드(서드파티) 쪽 일.
4. **프레임 밖 GPU 기록(GFW 10-03 발견):** DevConsole 입력은 `Submit` 으로 쌓였다가 `GConsoleCommandGlobalSystem::Update` — 스케줄러 프레임(End() 제출) **뒤** — 에서 실행된다. 그 안에서 렌더러를 만들어 그리고 놓으면, 기록은 다음 프레임 리스트에 들어가는데 다음 갱신 첫머리의 메모리 GC 가 텍스처를 파괴해 상태 추적에서 뺀다 → 제출 때 보류 배리어가 대상을 못 찾아 빠짐 → D3D12 [538](COMMON 상태에서 Clear) · [527](이전 상태 불일치). 프레임 밖 `RequestCommandList` 는 `Begin()` 펜스 대기 전에 그 인덱스 리스트를 Reset 하는 위험도 있다. 규칙: GPU 기록은 프레임 안(GraphicsBegin ~ GraphicsEnd 사이 단계)에서만. 고치는 방안은 `TODO.md` 5-33.
5. **셰이더 위치:** 10-02 23:09 전 바이너리는 `Source/Shader` 를 보므로 셰이더 템플릿을 못 찾는다("Shader Template(...) Not Found" → 기본 머터리얼 · 씬 뷰포트 머터리얼 실패). 이동 직후 다른 세션이 메인 Bin 런처를 띄워(23:07:53) 실행이 무효가 되고 링크가 LNK1168 로 잠깐 막혔다 — 공유 경로를 옮길 때는 옮기기 직전에 메인 Bin 실행 여부를 확인하고, 옮긴 뒤 바로 빌드 · 공지.
6. **DevScene 검증을 imgui.ini 수정 없이:** 엔진 루트는 실행 파일 위치(`../../`) 기준이고 `imgui.ini` · `jg_log.txt` · 리드백 PNG 는 작업 폴더 기준이다. `Temp/GPTest` 같은 같은 깊이의 폴더에 `[JGWidget][Open]` `JGDevFeature=1` 이 든 `imgui.ini`(도킹 정보가 있는 것 — 없으면 §5-3 경고)를 두고 `-WorkDir` 로 띄우면 메인 Bin 의 사용자 배치 · 로그를 건드리지 않는다.

## 6. 다음 에이전트 참고

- GP-G 다음 Graphics 일은 없다(D-23 Graphics 몫 끝, 쿡 확인 · 임시 복사 코드 삭제는 DevOps 가 10-03 에 끝냄). 남은 것: 커밋(사용자, `TODO.md` 체크리스트 — DevOps `CookCommand.cpp` 와 같은 커밋), 5-33(프레임 밖 GPU 기록 방어 — 결정 D-31).
- 재검증: 위 표 2 · 3 을 다시 하려면 `tools/gp_g123_test_snippet.diff.txt` 를 worktree 에 적용(`JG_GPTEST=1`, `JG_GPTEST_VSYNC=1`, `JG_GPTEST_NOGUI=1`) → `tools/gp_window_test.ps1`. 메인에는 넣지 않는다.
- 배율 블릿(제출 텍스처 크기가 달라도 늘려 그리기)은 설계 보류 GP-H9.
