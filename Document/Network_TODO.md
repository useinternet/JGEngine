# Network 할 일 목록 (순차 진행용)

작성 2026-09-29. 근거는 `Document/리슨서버_설계방안_2026-09-29.md`(결정 반영본), 구조는 `Document/리슨서버_구조설계_2026-09-29.md`(세션 2종 · 불변식 7 · 상태 기계 · 흐름). 인계 기록은 `Document/Memory/2026-09-29_리슨서버_설계.md`.
위에서부터 순서대로 진행한다. 한 항목이 끝나면 `[x]`, 한 단계가 끝나면 인계 기록에 진행 상황을 한 줄 추가한다.
`결정 필요` 표시는 착수 전 사용자 확인. 커밋은 사용자가 직접 한다.

**결정 (2026-09-29, 사용자):** 방안 A(호스트 권한 명령 중계 + 결정론 복제) · 위치 `GameFrameWorks/Network/` · 전송 Winsock TCP(`INetTransport` 뒤) · 이름 `Network/` · `PGameplaySession` · `INetTransport` · `PNet*`.

**겹치는 트랙:**
- Phase 0 의 0-1 · 0-2 는 GameFrameWorks 리뷰(`GameFrameWorks_설계코드리뷰_2026-09-28.md`) C7 · C1 과 같은 수정이다. 그쪽에서 먼저 했으면 확인만 한다.
- JGConsole 명령은 DevConsole_TODO 0-3 문법(`영역.동작`)을 따른다. DevConsole_TODO 1-4(레지스트리 전환)가 끝났으면 명령마다 .cpp 하나로, 아니면 `Main.cpp` 분기로 넣는다.
- 착수 시점에 GameFrameWorks 가 아직 커밋 전이면(GFW TODO 0-7) 이 트랙의 변경과 diff 가 섞인다.

---

## Phase 0. 커널 준비 (GameMaster)

- [ ] **0-1. C7 — `Load` 뒤 `Finalize`** — `GameMaster/GameMaster.cpp:275`. 리뷰 재현 R7 이 `[NOT REPRODUCED]`.
- [ ] **0-2. C1 — 죽은 ID 쓰기 가드** — `State/GameplayState.h` `Add<T>` · `GameplayBoardState.cpp` · `GameplayZone.cpp` · 내장 효과 `SetBoardPosition` · `MoveToZone`. 리뷰 재현 R1–R3 이 `[NOT REPRODUCED]`.
- [ ] **0-3. `PGameMaster::ExportDocument` · `ImportDocument`** — 파일 없는 텍스트 저장/로드(`PGameplaySerializer::ToJsonText` · `FromJsonText`). Import 는 `Finalize` 포함.
- [ ] **0-4. `PGameMaster::RulesFingerprint()`** — 등록 종류 이름(핸들러 · 효과 · 트리거 · 수정자) · 컴포넌트 테이블 이름 · 영역 · 수치 단계의 정렬 목록 해시.
- [ ] **0-5. 에이전트 러너 분리** — `PGameplayAgentRunner` 에 Submit 없는 "명령 고르기" 함수. 기존 `Step` 은 그 위에서 Submit(`Agents/GameplayAgent.cpp:164, 183`).
- [ ] **0-6. 검증** — `JGConsole.exe gmtest` 전부 통과 · 종료 코드 0 · 로그 오류 0. 문서 Export → Import 후 체크섬 동일. 등록 하나를 바꾸면 지문이 바뀐다.
- [ ] **0-7. 커밋** — 사용자.

## Phase 1. 전송

- [ ] **1-1. PCH Winsock 순서** — `Source/PCH/PCH.h` 맨 앞(`d3d12.h` 보다 먼저)에 `winsock2.h` · `ws2tcpip.h` + `#pragma comment(lib, "ws2_32.lib")`. 공유 PCH 라 전체 재빌드. 다른 세션이 MSBuild 중이면 격리 워크트리에서 먼저 확인. 증거 `Memory/2026-09-29_winsock_include_probe.txt`.
- [ ] **1-2. `INetTransport` · `HNetPeerId` · `HNetEvent` · `ENetDisconnectReason`** — `GameFrameWorks/Network/`. `Host(port)` · `Connect(addr)` · `Send(peer, bytes)` · `Poll(out events)` · `Close(peer)`.
- [ ] **1-3. `PNetLoopbackTransport`** — 한 프로세스 안 호스트 · 클라 쌍. 지연 · 끊김 주입.
- [ ] **1-4. `PNetTcpTransport`** — Winsock 비블로킹, `uint32` 길이 접두 프레이밍, 최대 크기 검사, `TCP_NODELAY`, 핑 · 타임아웃, `WSAStartup` / `WSACleanup` 수명.
- [ ] **1-5. 검증** — `JGConsole.exe net.test transport`: 루프백 에코, 1 MB 메시지 왕복, 끊김 검출. JGConsole 두 개로 TCP 에코. `gmtest` 회귀.
- [ ] **1-6. 커밋** — 사용자.

## Phase 2. 세션 — 명령 중계

- [ ] **2-1. 세션 골격** — `PGameplaySession`(기반: 붙이기 · 떼기, 붙기 전 게임 메시지 보관, `SubmitLocal` · `Tick` · `IsAuthority` · `IsLocallyControlled`, 알림) · `PGameplayHostSession`(Standalone = 전송 없음 · ListenServer) · `PGameplayClientSession` · `HGameplayPlayerSlot`. 구조설계 §5.
- [ ] **2-1b. 메시지** — `Hello` · `Welcome` · `Refuse` · `CommandRequest` · `CommandAccepted` · `CommandRejected` · `ResyncRequest` · `Document` · `StartGame` · `SlotChanged` · `Travel` · `Ping`. 봉투 `{Type, Payload}`, 페이로드 JSON(`IJsonable`), 문서만 zlib. 순번 = 상태의 `Sequence`.
- [ ] **2-2. 핸드셰이크** — `Hello{빌드 ID, 스키마 버전, 규칙 지문, 이름, 재접속 토큰}` → `Welcome{슬롯}` / `Refuse{사유}`.
- [ ] **2-3. 시작** — `PGameplaySession::StartGame()`: 호스트가 `StartGame{시드, 초기 상태}` 를 보내고 전원이 같은 `Start(seed)`. Standalone 은 바로 `Start`.
- [ ] **2-4. 명령 중계** — `SubmitLocal`, 호스트 도착 순 명령 줄, 소유 검사(행동자 → 조작 주체 함수. `ResolveChoice` 는 `Chooser`), `CommandAccepted` 방송 · `CommandRejected` 회신.
- [ ] **2-5. 체크섬 비교 · 덤프** — 승인마다 비교, 어긋나면 양쪽 상태 JSON 덤프 + `ResyncRequest`.
- [ ] **2-6. 호스트 세션이 AI 구동** — 싱글(Standalone) 포함. 0-5 의 고르기 함수로 에이전트 명령을 같은 줄에 세운다.
- [ ] **2-7. 네트워크 모드 로컬 `Undo` 거부**
- [ ] **2-8. 검증** — `net.test session`: 한 프로세스 호스트 1 + 클라 3(루프백), 슬롯마다 무작위 에이전트로 1,000 명령 → 모든 순번 체크섬 동일. 남의 행동자 명령 · 지문이 다른 클라 거부. 명령당 체크섬 비용 측정값 기록.
- [ ] **2-9. 커밋** — 사용자.

## Phase 3. 입장 · 재접속 · 재동기

- [ ] **3-1. 진행 중 입장** — `Welcome` 에 문서(zlib)를 싣고, 이후 승인부터 적용.
- [ ] **3-2. 재접속** — 재접속 토큰으로 같은 슬롯 복구 + 문서.
- [ ] **3-3. 재동기** — `ResyncRequest` → `Document{순번}`. 받은 순번 이하의 승인은 버린다.
- [ ] **3-4. 끊김 알림** — 게임 콜백(대기 또는 `SetAgent` 대리는 게임이 선택). 호스트 종료 = 세션 종료.
- [ ] **3-5. 검증** — 500 명령 시점 입장 → 이후 체크섬 동일. 상태를 고의로 바꾼 클라 → 검출 · 덤프 · 복구. 끊김 → 재접속 후 체크섬 동일.
- [ ] **3-6. 커밋** — 사용자.

## Phase 4. 2 프로세스 결정론

- [ ] **4-1. `JGConsole.exe net.host` / `net.join`** — 헤드리스 호스트 · 클라, 무작위 에이전트. JGConsole 은 스케줄러를 돌리지 않으므로 명령 루프에서 세션 폴링을 직접 부른다.
- [ ] **4-2. 검증** — 프로세스 3개(호스트 1 · 클라 2) 1,000 명령 체크섬 동일.
- [ ] **4-3. 커밋** — 사용자.

## Phase 5. 월드 연결

- [ ] **5-1. `JGGameInstance` 가 세션 소유** — 틱 순서 세션 → 월드. 월드 변경을 `Travel` 로 알림. 종료 시 세션 · 전송 정리.
- [ ] **5-2. `JGGameMasterActor` 경유** — BeginPlay 에 세션에 붙기 · EndPlay 에 떼기. `executeNow` 가 세션으로(`Actors/GameMasterActor.cpp:405`). 결과에 `Sent`, 거절 알림. 원격 · 승인 명령은 입력 정책을 거치지 않는다.
- [ ] **5-2b. 엔트리 액터 규약** — 규칙 등록은 모든 기계, 초기 상태 구성 + `StartGame` 은 `IsAuthority()` 일 때만. 게임 모듈 가이드에 한 줄.
- [ ] **5-3. `JGGameplayControllerActor` 소유 검사** — `IsLocallyControlled`.
- [ ] **5-4. 런처 명령줄** — `-host [port]` · `-join <주소>` · `-name`(`JGLauncher/Main.cpp:6` 은 인자를 받지 않는다).
- [ ] **5-5. 최소 로비** — ImGui: 슬롯 목록 · 준비 · 시작.
- [ ] **5-6. 검증** — 런처 2개(호스트 · 클라)로 한 전투 완주, 양쪽 종료 코드 0 · 로그 오류 0.
- [ ] **5-7. 커밋** — 사용자.
