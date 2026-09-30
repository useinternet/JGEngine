# Network 할 일 목록 (순차 진행용)

작성 2026-09-29. 근거는 `Document/리슨서버_설계방안_2026-09-29.md`(결정 반영본), 구조는 `Document/리슨서버_구조설계_2026-09-29.md`(세션 2종 · 불변식 7 · 상태 기계 · 흐름). 인계 기록은 `Document/Memory/2026-09-29_리슨서버_설계.md`.
위에서부터 순서대로 진행한다. 한 항목이 끝나면 `[x]`, 한 단계가 끝나면 인계 기록에 진행 상황을 한 줄 추가한다.
`결정 필요` 표시는 착수 전 사용자 확인. 커밋은 사용자가 직접 한다.

**결정 (2026-09-29, 사용자):** 방안 A(호스트 권한 명령 중계 + 결정론 복제) · 위치 `GameFrameWorks/Network/` · 전송 Winsock TCP(`INetTransport` 뒤) · 이름 `Network/` · `PGameplaySession` · `INetTransport` · `PNet*`.

**진행 (2026-09-29):** Phase 0–4 구현 · 검증 완료(격리 워크트리에서 작성 → 메인 트리로 반영 → 메인 트리에서 재검증: `gmtest` 96/96, `net.test all` 66/66, 2 프로세스 OK). 결과 원문 `Document/Memory/2026-09-29_net_phase0-4_results.txt`. 남은 것: 각 단계 커밋(사용자), Phase 5.
**GFW Phase 1 병합 완료 (2026-09-30 00:15, GameFrameWorks 세션):** 겹친 4개 파일(GameMaster.cpp · GameplayRuleEngine.cpp · GameplayState.h · GameMasterSelfTest.cpp)은 이 트랙의 변경을 보존해 3-way 병합됐다. 달라진 것: C1 · C7 은 그쪽 구현으로 통일(`Add<T>` 는 `T*` · 경고 없음, 상태 계층 `HGameplayState::SetBoardPosition` · `MoveToZone` 이 죽은 ID 를 거부하고 내장 효과도 이것을 씀), 트리거 `React(HGameplayTriggerContext&)`(`PNetThornsTrigger` 시그니처도 바뀜), 트리거 반응 · 페이즈 전이 시점이 바뀌어 체크섬 **값**이 바뀜. **병합 뒤 이 세션이 직접 재검증(00:18): `gmtest` 111/111(GameMaster 92 · 월드 19), `net.test all` 66/66, 2 프로세스 OK — seq 1000 · 체크섬 `16006860866352003711` 이 셋 다 같고 불일치 0.** 아래 항목의 수치 중 체크섬 값과 `gmtest` 개수는 병합 전 값이다.

**겹치는 트랙:**
- Phase 0 의 0-1 · 0-2 는 GameFrameWorks 리뷰(`GameFrameWorks_설계코드리뷰_2026-09-28.md`) C7 · C1 과 같은 수정이다. 그쪽에서 먼저 했으면 확인만 한다.
- JGConsole 명령은 DevConsole_TODO 0-3 문법(`영역.동작`)을 따른다. DevConsole_TODO 1-4(레지스트리 전환)가 끝났으면 명령마다 .cpp 하나로, 아니면 `Main.cpp` 분기로 넣는다.
  → 2026-09-30 옮김: `net.test` · `net.host` · `net.join` 은 `NetCommands.cpp` 의 `HAutoConsoleCommand` 이고, `Main.cpp` 의 임시 분기는 지웠다. 숫자 인자는 `TryGetInt` · `std::from_chars` 로 읽어 잘못된 입력은 예외 없이 거부한다(아래 원래 메모).
  → 2026-09-29 DevConsole_TODO 1-4 완료. `Main.cpp` 는 레지스트리로 바뀌었고, `net.*` 는 `runNetCommand`(NetCommands.cpp)로 보내는 임시 분기로 남겼다(`Main.cpp` main). 세 명령을 NetCommands.cpp 에서 `HAutoConsoleCommand` 로 선언하면(`Source/Runtime/Core/ConsoleCommand/ConsoleCommandGlobalSystem.h` 상단 예시, 인자는 `HConsoleCommandArgs::TryGetString/TryGetInt`) `Main.cpp` 의 `runNetCommand` 선언과 분기를 지운다. `std::stoull` 은 잘못된 입력에 예외를 던진다(엔진에 catch 없음). 32비트 값은 `TryGetInt`, 64비트 시드는 `TryGetString` 뒤 `std::from_chars`(예외 없음)로 읽는다.
  → 2026-09-30(DevConsole 트랙): 사용자 결정으로 **모듈의 테스트 명령은 그 모듈에 선언**한다. `gmtest`는 `GameFrameWorks/Core/GameFrameWorksModule.cpp`로 옮겼다. JGConsole은 이제 시작할 때 GameFrameWorks를 연결한다(`Main.cpp` `CONSOLE_ENGINE_MODULES`), 그래서 `NetCommands.cpp` 핸들러 안의 `ConnectModule("GameFrameWorks")`는 없어도 된다(있어도 무해). `net.test`도 같은 방식으로 GameFrameWorks 안(예: `Network/` 쪽 .cpp)으로 옮기는 것을 권한다. `net.host`/`net.join`은 호스트·클라 루프를 계속 도는 프로세스용 명령이라, 에디터 콘솔에서 실행되면 에디터가 멈춘다. 그래서 JGConsole에 남긴다. 옮기는 시점은 Network 트랙이 정한다.
- 착수 시점에 GameFrameWorks 가 아직 커밋 전이면(GFW TODO 0-7) 이 트랙의 변경과 diff 가 섞인다.

---

## Phase 0. 커널 준비 (GameMaster)

- [x] **0-1. C7 — `Load` 뒤 `Finalize`** — 완료 2026-09-29. `Load` 는 `ImportDocument` 를 거치고 `ImportDocument` · `Replay` 가 `Finalize` 를 부른다. `gmtest` 10 절 "R7: same trigger order after ImportDocument" (같은 우선순위 트리거 둘, 순서에 따라 12 vs 11). GameFrameWorks_TODO 1-6 과 같은 수정.
- [x] **0-2. C1 — 죽은 ID 쓰기 가드** — 완료 2026-09-29. `HGameplayState::Add<T>` 는 죽은 ID 면 쓰지 않고 `nullptr`(반환형 `T&` → `T*`). 내장 효과 `SetBoardPosition` · `MoveToZone` 은 죽은 대상이면 이벤트 없이 건너뛴다. `gmtest` 9 절(R1–R3 시나리오 8 검사). GameFrameWorks_TODO 1-1 과 같은 수정 — 2026-09-30 병합에서 그쪽 구현(경고 없음, 상태 계층 `SetBoardPosition` · `MoveToZone` 가드)으로 통일됐다.
- [x] **0-3. `PGameMaster::ExportDocument` · `ImportDocument`** — 완료 2026-09-29. Import 는 `Finalize` 포함.
- [x] **0-4. `PGameMaster::RulesFingerprint()`** — 완료 2026-09-29. 핸들러 · 효과 · 트리거(우선순위 포함) · 수정자(값 종류 · 단계 포함) · 컴포넌트 테이블 · 수치 단계 · 보드 종류 · 스키마의 정렬 목록 FNV-1a. 영역은 상태에 들어 있어 넣지 않았다. 등록 순서가 달라도 같고 핸들러 하나를 더하면 달라지는 것을 검사.
- [x] **0-5. 에이전트 러너 분리** — 완료 2026-09-29. `PGameplayAgentRunner::Choose`(Submit 없음), `Step` = Choose + Submit. 그 밖에 `PGameMaster::TeamOfActor` · `SetUndoEnabled` 추가.
- [x] **0-6. 검증** — 완료 2026-09-29. `gmtest` 96/96(기존 75 + 새 21) · 종료 코드 0. 리뷰 재현 코드(HEAD 판)에서 R1 · R2 · R3 · R7 `[NOT REPRODUCED]`. 런처 30 초 종료 코드 0 · 로그 오류 0.
- [ ] **0-7. 커밋** — 사용자.

## Phase 1. 전송

- [x] **1-1. Winsock 포함** — 완료 2026-09-29, **계획 변경: PCH 를 고치지 않았다.** PCH 가 `Windows.h` 로 이미 들여오는 Winsock 1.1(`winsock.h`)에 필요한 함수(socket · select · ioctlsocket · TCP_NODELAY …)가 모두 있어 `NetTcpTransport.cpp` 에서 그대로 쓴다(`SD_SEND` 만 직접 정의, `ws2_32.lib` 는 그 파일의 pragma). 공유 PCH 변경 = 전 모듈 재빌드 · 다른 세션 빌드와 충돌을 피했다. `WIN32_LEAN_AND_MEAN` 이 생기면 같은 파일이 `winsock2.h` 로 바꿔 넣는다. IPv4 만.
- [x] **1-2. `INetTransport` · `HNetPeerId` · `HNetEvent` · `ENetDisconnectReason`** — 완료 2026-09-29. `Listen(port)` · `Connect(addr, port)` · `Send` · `Poll` · `Close`(보낸 뒤 닫기) · `Shutdown` · `HasPendingSends`.
- [x] **1-3. `PNetLoopbackTransport`** — 완료 2026-09-29. `PNetLoopbackHub` 가 포트로 잇는다. `SetDelayPolls` · `Break`(연결 유실).
- [x] **1-4. `PNetTcpTransport`** — 완료 2026-09-29. 비블로킹 + Poll, `uint32` 길이 접두 프레이밍(최대 64 MB), `TCP_NODELAY`, 정상 종료(다 보냄 → 송신 종료 → 상대가 닫을 때까지 읽어 버림. 읽지 않은 채 닫으면 RST 로 상대가 못 읽은 데이터를 잃는다), 전송 객체마다 `WSAStartup` / `WSACleanup`. `SetListenAddress` — 한 기계 테스트는 127.0.0.1 로 묶는다(모든 인터페이스로 열면 방화벽 확인 창). **핑 · 타임아웃은 세션으로 옮겼다**(시간을 세션이 안다).
- [x] **1-5. 검증** — 완료 2026-09-29. `net.test` 전송 절: 루프백(연결 · 1 MB · 지연 · 닫기 순서 · 유실 · 거부) · 같은 프로세스 TCP(연결 · 1 MB · 닫기 · 거부) · 코덱(필드 왕복 · 압축 문서 50,000 → 181 바이트 · 잘못된 종류 거부). 프로세스 간 TCP 는 Phase 4 가 같은 전송으로 검증한다.
- [ ] **1-6. 커밋** — 사용자.

## Phase 2. 세션 — 명령 중계

- [x] **2-1. 세션 골격** — 완료 2026-09-29. 슬롯 값 타입은 메시지에 실리므로 `Network/Messages/GameplayPlayerSlot.h` 에 뒀다. 슬롯 = 조작 주체 목록 · `bAllControllers`(Standalone 로컬) · 이름 · 접속 여부(+ 호스트 내부용 피어 · 토큰). **에이전트가 등록된 조작 주체는 AI**(호스트가 구동, 사람 명령 거부), 아니면 그 조작 주체를 가진 슬롯의 사람.
- [x] **2-1b. 메시지** — 완료 2026-09-29. 바이트 = `[종류 u8][플래그 u8][본문]`, 본문 JSON(메시지 구조체를 "M" 아래에), 문서만 zlib. 순번 = 상태의 `Sequence`.
- [x] **2-2. 핸드셰이크** — 완료 2026-09-29. `Hello{프로토콜 버전, 스키마, 이름, 토큰}`. "빌드 ID" 대신 프로토콜 버전을 쓴다(기계마다 따로 빌드하면 빌드 시각이 달라 늘 거부된다). **규칙 지문은 `Welcome` · `StartGame` 에 싣고 클라가 비교**(로비에서는 호스트에 GameMaster 가 없을 수 있다).
- [x] **2-3. 시작** — 완료 2026-09-29. `StartGame{시드, 지문, 초기 상태 JSON}` → 클라가 등록된 테이블을 가진 사본으로 읽고 같은 `Start(seed)`.
- [x] **2-4. 명령 중계** — 완료 2026-09-29. 클라는 한 번에 하나만 보낸다(앞선 명령이 복제본에 반영되기 전 상태로 다음 명령을 검증하지 않게. 나머지는 줄에 둔다). 클라는 로컬 `Validate` 로 틀린 명령을 왕복 없이 거절한다.
- [x] **2-5. 체크섬 비교 · 덤프** — 완료 2026-09-29. 클라가 승인마다 순번 · 체크섬을 비교, 어긋나면 `<DesyncDumpDirectory>/desync_slot<N>_seq<M>_local.json` 과 문서 수신 뒤 `_host.json` 을 쓰고 재동기.
- [x] **2-6. 호스트 세션이 AI 구동** — 완료 2026-09-29. 한 Tick 에 최대 64 명령.
- [x] **2-7. 네트워크 모드 로컬 `Undo` 거부** — 완료 2026-09-29. 세션이 붙으면 `PGameMaster::SetUndoEnabled(false)`, 떼면 복구. `PGameplaySession::Undo` 는 Standalone 만.
- [x] **2-8. 검증** — 완료 2026-09-29. `net.test` 세션 절: 호스트 1 + 클라 3, 1,001 명령 3.7 초(디버그) → 세 클라 모두 불일치 0 · 마지막 체크섬 동일. 클라가 직접 보낸 명령 224 · 149 · 226, 호스트 AI 202, 네트워크로 푼 선택 대기 112. 가짜 클라의 남의 행동자 명령 거절("not your actor"), 프로토콜 다른 클라 거부, 자리 없음 거부, 규칙이 다른 클라는 StartGame 에서 스스로 종료. **체크섬 비용: 엔티티 5개 0.26 ms, 301개 5.1–6.6 ms(디버그)** — 명령마다 전원이 한 번씩 낸다.
- [ ] **2-9. 커밋** — 사용자.

## Phase 3. 입장 · 재접속 · 재동기

- [x] **3-1. 진행 중 입장** — 완료 2026-09-29. `Welcome{bStarted}` 뒤에 `Document`(zlib), 그다음 승인부터 적용.
- [x] **3-2. 재접속** — 완료 2026-09-29. `PGameplayClientSession::Reconnect` · 같은 토큰이면 같은 슬롯. 옛 연결이 살아 있다고 알던 중이면 호스트가 그 연결을 닫고 이어받는다.
- [x] **3-3. 재동기** — 완료 2026-09-29. `ResyncRequest` → `Document`. 재동기 중 온 승인은 버리고(문서에 들어 있다), 문서의 `Sequence` 이하 승인도 버린다.
- [x] **3-4. 끊김 알림** — 완료 2026-09-29. `OnPeerDisconnected(slot)` · `OnClosed(reason)`. 핑 1 초 · 타임아웃 10 초(설정). 대리는 게임이 `SetAgent` 로(테스트가 흉내 냄).
- [x] **3-5. 검증** — 완료 2026-09-29. `net.test` 복구 절: 순번 500 에 입장 → 문서로 합류 · 자기 명령 전송 · 이후 일치. 고의로 바꾼 복제본 → 순번 616 에서 1 회 검출 · 덤프 2 개 · 복구. 연결 유실 → 호스트 AI 가 그 자리를 이어받아 진행 → 순번 742 에 재접속 → 같은 슬롯 · 일치. 순번 1000 에서 전원 일치.
- [ ] **3-6. 커밋** — 사용자.

## Phase 4. 2 프로세스 결정론

- [x] **4-1. `JGConsole.exe net.host` / `net.join`** — 완료 2026-09-29. `Source/Programs/JGConsole/NetCommands.cpp` 의 `HAutoConsoleCommand` 세 개(2026-09-30 명령 레지스트리로 옮김, `Main.cpp` 는 손대지 않음). 인자 `-bind= -port= -clients= -commands= -seed=` / `-address= -port= -players= -agent-seed= -name=`. 클라는 호스트가 아직 없으면 30 초까지 다시 접속한다.
- [x] **4-2. 검증** — 완료 2026-09-29. `Document/Memory/tools/net_two_process/run_two_process.sh <Bin/DevelopEngine>`: 호스트 1 + 클라 2 를 별도 프로세스 · TCP 로 1,000 명령 → 셋 다 seq 1000 · 체크섬 동일, 클라 불일치 0(클라가 보낸 명령 206 · 234). 18 초.
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
