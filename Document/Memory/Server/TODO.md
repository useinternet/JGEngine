# Server(네트워크 · 리슨 서버) TODO

작성 2026-09-30, 갱신 09-30 23:1x (Phase 5). `Files/Network_TODO.md`(09-29 작성 · 09-30 11:45 갱신)를 통합 · 최신화한 것. 항목 ID 는 그대로 쓴다(Phase-번호). 현황은 `현황.md`.
결정(09-29, 사용자): 방안 A(호스트 권한 명령 중계 + 결정론 복제) · 위치 `GameFrameWorks/Network/` · 전송 Winsock TCP(`INetTransport` 뒤) · 이름 `PGameplaySession` · `INetTransport` · `PNet*`.
규칙: 한 항목이 끝나면 완료 이력으로 옮기고 `현황.md` §4 에 한 줄. 커밋은 사용자가 한다. Phase 5 는 사용자 지시(09-30 22:1x "페이즈 5 시작해줘")로 착수했다.

---

## 다음에 할 일

| 순서 | ID | 항목 | 착수 조건 | 완료 기준 |
|---|---|---|---|---|
| 1 | 5-7 | **Phase 5 커밋** (사용자) — 아래 "커밋 체크리스트 (5-7)" | GFW Phase 2 · C-3 · GUI 1-2 변경과 한 커밋(같은 파일 · 헤더). 커밋 전 `tasklist` 에 다른 세션 cl · link 없음 | 커밋 뒤 메인 트리에서 `gmtest` 156 · `net.test all` 101 · 2 프로세스 kernel · world OK |
| 2 | 5-5 | 최소 로비 — **사용자 결정 대기**(범위 · 시점, `현황.md` §8 #2) | C-3 위젯 이관 완료(JGEditor 담당 세션) | 권고안: JGEditor 개발용 창 — 세션 상태 · 슬롯 표 · Host · Join · Leave · 호스트 "월드 다시 로드". 게임 프로젝트(NetDuel)로 두 런처에서 창으로 호스트 · 참가 · 새 판 시작 확인 |

---

## 미완료 항목

| ID | 항목 | 우선순위 | 상태 | 선행조건 | 완료 기준 | 비고 |
|---|---|---|---|---|---|---|
| 5-5 | 최소 로비 — 슬롯 목록 · (준비) · 시작 | 중 | 미착수 · **결정 대기** | C-3 이관 완료, 사용자 결정 | 위 표 | 엔진 API 는 있음(`GetSlots` · `OnSlotsChanged` · `SetSlotControllers` · `HostSession` · `JoinSession` · `LeaveSession` · `LoadWorld`). 위치는 사용자 결정(09-30 밤) "위젯은 JGEditor" 에 따른다. "준비" 는 프로토콜 추가가 필요하다 |
| 5-7 | Phase 5 커밋 | 상 | 사용자 | — | 위 표 | |

### 커밋 체크리스트 (5-7)

Phase 5 가 고친 파일(09-30 23:0x 메인 트리 반영). 같은 파일에 다른 트랙 미커밋 변경이 섞여 있으니 트랙별로 나누지 말고 함께 넣는다.

| 분류 | 파일 | 섞인 다른 트랙 변경 |
|---|---|---|
| 세션 · 메시지 · 검증 | `Source/Runtime/GameFrameWorks/Network/Session/GameplayHostSession.h/.cpp` · `GameplayClientSession.h/.cpp` · `Network/Messages/GameplayNetMessage.cpp` · `Network/GameplayNetSelfTest.h/.cpp` | 없음 |
| 게임 인스턴스 · 모듈 | `Source/Runtime/GameFrameWorks/Core/GameInstance.h/.cpp` · `Core/GameFrameWorksModule.cpp` | `GameFrameWorksModule.cpp` 에 C-3(위젯 등록 제거) · GFW Phase 2 |
| 액터 | `Source/Runtime/GameFrameWorks/Actors/GameMasterActor.h/.cpp` · `GameplayControllerActor.h/.cpp` | GFW Phase 2(보드 배치 · 피킹) · C-3(주석 "씬 뷰포트") |
| JGConsole | `Source/Programs/JGConsole/NetCommands.cpp` | 없음 |
| 템플릿 · 문서 | `Build/Templates/GameProject/Source/{PROJECT_NAME}/{PROJECT_NAME}EntryActor.cpp` · `Document/Memory/GameModule/Files/게임프로젝트_생성가이드.md` | 가이드에 GameModule 09-30 저녁 변경 |
| 도구 · 증적 (신규 미추적 포함) | `Document/Memory/tools/net_two_process/run_two_process.sh` · `Document/Memory/Server/Files/tools/net_duel_testgame/`(3) · `Document/Memory/Server/Files/2026-09-30_phase5_*.txt`(3) · Server 문서 | — |
| 바이너리 | `Bin/DevelopEngine/GameFrameWorks.dll/.exp/.lib` · `JGConsole.exe` · `JGEditor.dll/.exp/.lib` (23:0x 메인 트리 빌드) | 다른 트랙 재빌드와 같은 파일 |

---

## 게임 프로젝트 영역 (엔진 범위 외)

| 항목 | 엔진이 제공하는 것 | 게임이 하는 것 |
|---|---|---|
| 행동자 → 조작 주체 | `PGameMaster::SetTeamOfActorFunction` · `TeamOfActor` | 함수 하나 등록 (팀 = 조작 주체 번호) |
| 초기 상태 · 시작 | `JGGameInstance::IsAuthority` · `JGGameMasterActor::StartGame(seed)` · 템플릿 주석 | 엔트리 액터에서 권한일 때만 초기 상태 구성 + 시작 (규칙 등록은 모든 기계) |
| 슬롯 ↔ 조작 주체 배정 | `PGameplayHostSession::SetSlotControllers` · `OnSlotsChanged` (기본 슬롯 N = 조작 주체 N) | 로비 화면의 표현 · 배정 규칙 |
| 끊김 정책 | `OnPeerDisconnected(slot)` · `OnClosed(reason)` · `PGameMaster::SetAgent` · `JGGameInstance::LeaveSession` | 대기 / AI 대리 / 싱글로 이어 두기 중 선택 |
| 결정론 준수 | 체크섬 비교 · 덤프 · 재동기 | 규칙 코드가 시계 · 로컬 설정 · 부동소수를 쓰지 않음, 컴포넌트 필드 기본값 초기화 |
| 은닉 정보(비공개 손패 · 덱) | 없음 (방안 A 는 전 상태 공개) | 필요해지면 보류 표의 방안 C |

---

## 보류 (지금 구체적 문제 없음 — 문제가 생기면 꺼낸다)

| 항목 | 꺼내는 조건 |
|---|---|
| 다른 전송(Steam · UDP · 릴레이) — `INetTransport` 구현 클래스 추가 | 인터넷 너머 플레이(포트 개방 없이)가 필요해질 때 |
| 방안 C(비공개 영역만 호스트 보유) | 은닉 정보가 있는 규칙 · PvP 가 필요할 때. 전체 체크섬 검증이 깨지므로 큰 변경 |
| 체크섬 비용(증분 해시 · 주기 비교) | 엔티티 수천 · 명령당 체크섬이 프레임을 넘길 때(지금 301 엔티티 5–6.6 ms 디버그) |
| 멀티 중 되돌리기(명령 하나로 얹기) | 게임이 요구할 때. 지금은 네트워크 모드 로컬 `Undo` 거부만 |
| 클라 예측 · 롤백 | 턴제라 필요 없음. 실시간 요소가 생길 때 |
| IPv6 | 필요할 때(`PNetTcpTransport` 는 IPv4) |
| `Network/` 별도 모듈 분리 | 소켓을 쓰는 곳이 GameFrameWorks 밖에도 생길 때 |
| 세션 상태에 `Lobby` · `Traveling` 분리(구조설계 §8) | 구현은 `Ready` · `OnTravelRequested` + 보관 메시지로 충분했다. 월드 이동 중 상태 질의가 필요해질 때 |
| PCH 맨 앞 `winsock2.h`(증적 `Files/2026-09-29_winsock_include_probe.txt`) | `WIN32_LEAN_AND_MEAN` 도입 등 PCH 를 어차피 바꿀 때 — 코드는 이미 `_WINSOCKAPI_` 분기로 대비 |
| 명령 스트림 직렬화 규약 문서화(GFW 백로그) | 외부 도구가 프로토콜을 읽어야 할 때. 지금은 `GameplayNetMessage.h` 주석이 규약 |
| 로비 "준비" 표시(클라 → 호스트 메시지 + 슬롯 필드) | 게임이 전원 준비 뒤 시작을 요구할 때. 지금은 호스트가 시작 시점을 정한다 |
| `-join` 자동 재시도 | 호스트보다 먼저 띄우는 사용 방식이 필요해질 때. 지금은 한 번 접속하고 거부되면 Closed(게임이 `OnClosed` 로 처리) |
| 세션 알림을 게임 인스턴스가 다시 내보내기(세션 교체에도 구독 유지) | 게임이 `OnLocalCommandRejected` 등을 구독하면서 Host · Join · Leave 로 세션을 바꿀 때. 지금은 세션 객체에 직접 구독 |

---

## 완료 이력

| ID | 항목 | 완료일 | 검증 근거 |
|---|---|---|---|
| 0-1 | C7 — `Load` 뒤 `Finalize`(`ImportDocument` · `Replay` 경유) | 09-29 | `Files/2026-09-29_net_phase0-4_results.txt` gmtest 10 절 R7 (12 vs 11) · GFW Phase 1 구현으로 통일(09-30) |
| 0-2 | C1 — 죽은 ID 쓰기 가드(`Add<T>` → `T*`, `SetBoardPosition` · `MoveToZone`) | 09-29 | gmtest 9 절 8 검사 · GFW 구현으로 통일(09-30) |
| 0-3 | `PGameMaster::ExportDocument` · `ImportDocument` | 09-29 | gmtest 10 절 왕복 체크섬 동일 |
| 0-4 | `PGameMaster::RulesFingerprint()` (FNV-1a, 등록 순서 무관) | 09-29 | gmtest 10 절 |
| 0-5 | `PGameplayAgentRunner::Choose` 분리 · `TeamOfActor` · `SetUndoEnabled` | 09-29 | gmtest 11 절 |
| 0-6 | Phase 0 검증 | 09-29 | `gmtest` 96/96, R1 · R2 · R3 · R7 `[NOT REPRODUCED]`, 런처 30 초 종료 0 |
| 1-1 | Winsock 포함 — PCH 미수정, `NetTcpTransport.cpp` 에서 Winsock 1.1 | 09-29 | 빌드 0 오류 · `Files/2026-09-29_winsock_include_probe.txt` |
| 1-2 | `INetTransport` · `HNetPeerId` · `HNetEvent` · `ENetDisconnectReason` | 09-29 | `net.test transport` |
| 1-3 | `PNetLoopbackTransport` · `PNetLoopbackHub`(`SetDelayPolls` · `Break`) | 09-29 | `net.test transport` 루프백 절 |
| 1-4 | `PNetTcpTransport`(비블로킹 · 길이 프레이밍 · `TCP_NODELAY` · 정상 종료 · `SetListenAddress`) | 09-29 | `net.test transport` TCP 절(32 B · 1 MB · 닫기 · 거부) |
| 1-5 | Phase 1 검증(전송 · 코덱) | 09-29 | 결과 파일 transport / codec 절 |
| 2-1 · 2-1b | 세션 골격 · `HGameplayPlayerSlot` · 메시지 12종 · 코덱 | 09-29 | `net.test session` |
| 2-2 | 핸드셰이크(프로토콜 버전 · 스키마 · 토큰, 지문은 `Welcome` · `StartGame`) | 09-29 | 프로토콜 101 ≠ 1 거부 · 만석 거부 · 규칙 다름 → StartGame 에서 종료 |
| 2-3 | `StartGame{시드, 지문, 초기 상태}` | 09-29 | 세 클라 같은 `Start(seed)` |
| 2-4 | 명령 중계(클라 한 번에 하나 · 로컬 `Validate` 선거절) | 09-29 | 1,001 명령 불일치 0, 클라 자체 명령 224 / 149 / 226 |
| 2-5 | 체크섬 비교 · 덤프 · 재동기 | 09-29 | recovery 절 seq 616 |
| 2-6 | 호스트 세션 AI 구동(한 Tick 최대 64) | 09-29 | 호스트 AI 202 명령 |
| 2-7 | 네트워크 모드 로컬 `Undo` 거부 | 09-29 | gmtest 11 절 · `PGameplaySession::Undo` Standalone 만 |
| 2-8 | Phase 2 검증 · 체크섬 비용 측정 | 09-29 | 5 엔티티 0.26 ms · 301 엔티티 5.1–6.6 ms(디버그) |
| 3-1 | 진행 중 입장(`Welcome{bStarted}` → `Document`) | 09-29 | seq 500 입장 → 일치 · 자기 명령 21/100 |
| 3-2 | 재접속(같은 토큰 = 같은 슬롯, 옛 연결 인계) | 09-29 | seq 742 재접속 → 슬롯 2 · 일치 |
| 3-3 | 재동기(`ResyncRequest` → `Document`, 중간 승인 폐기) | 09-29 | 변조 검출 1회 · 덤프 2개 · 복구 |
| 3-4 | 끊김 알림(`OnPeerDisconnected` · `OnClosed`, 핑 1 s · 타임아웃 10 s) | 09-29 | 연결 유실 → 클라 Closed "connection lost" · 호스트 AI 대리 |
| 3-5 | Phase 3 검증 | 09-29 | seq 1000 전원 일치 |
| 4-1 | `JGConsole.exe net.host` / `net.join` | 09-29 | 인자 `-bind -port -clients -commands -seed` / `-address -port -players -agent-seed -name` |
| 4-2 | 2 프로세스 검증(`run_two_process.sh`) | 09-29 | 셋 다 seq 1000 · 체크섬 동일 · 불일치 0 · 18 초. 따로 컴파일한 두 빌드도 같은 값 |
| M-1 | GFW Phase 1 병합 후 재검증 | 09-30 00:18 | `gmtest` 111/111 · `net.test all` 66/66 · 2 프로세스 체크섬 `16006860866352003711` |
| M-2 | `net.*` 를 `NetCommands.cpp` `HAutoConsoleCommand` 로 이동 · `Main.cpp` 임시 분기 삭제 · 숫자 인자 예외 없는 파서 | 09-30 00:31 | `net.host -port=abc` 종료 1 · abort 없음, help 에 세 명령, 3종 검사 재통과 |
| C-1 (0-7 · 1-6 · 2-9 · 3-6 · 4-3) | Phase 0–4 커밋 (사용자) | 09-30 17:30 | `4c8ac73` "문서 정리" — `Network/` 17 파일 + `NetCommands.cpp` · GameMaster 변경 · `Core/ConsoleCommand/` · `Main.cpp` · Bin 산출물 포함, 작업 트리 clean (09-30 저녁 git 확인) |
| N-2 | 문서 이동 뒤 코드 주석 경로 갱신 | 09-30 (확인) | `NetTcpTransport.cpp:5` 가 이미 `Document/Memory/Server/Files/2026-09-29_winsock_include_probe.txt` 를 가리킴 |
| V-1 | 커밋된 코드 기준 재검증 (`gmtest` · `net.test all` · 2 프로세스) | 09-30 22:2x | Phase 5 착수 전 스냅숏(메인 트리 22:20) 기준선: 빌드 오류 0 · `gmtest` 156/156 · `net.test all` 66/66 · 2 프로세스 OK(`16006860866352003711`) — `Files/2026-09-30_phase5_results.txt` 첫 절 |
| 5-1 | `JGGameInstance` 세션 소유(`GetSession` · `IsAuthority` · `HostSession` · `JoinSession` · `LeaveSession`), 틱 순서 세션 → 이동 → 월드, `LoadWorld` 가 엔트리 전에 `NotifyTravel`, 클라 이동 따라가기, 종료 시 닫기(최대 2 초), 인스턴스 교체 시 세션 넘김 | 09-30 23:0x (미커밋) | world 절: 로비 입장 · Travel → StartGame · 두 번째 월드 이동 · Welcome 의 월드로 진행 중 입장 · LeaveSession 이어받기. 2 프로세스 world 모드 |
| 5-2 | `JGGameMasterActor` BeginPlay 붙기 · EndPlay 떼기 · `StartGame` · `GetSession` · `executeNow` → `SubmitLocal` · 결과 `Sent`. 호스트 세션 `adoptStartedGameMaster` | 09-30 23:0x (미커밋) | world 절(호스트 `Executed` · 클라 `Sent` · 언로드 시 떼기 · 클라 `StartGame` false), `gmtest` 156(게임 인스턴스 절이 세션 경유로 통과) |
| 5-2b | 엔트리 규약: 템플릿 `EntryActor.cpp` 주석 예시 · 생성 가이드 4절 한 줄 · 3절 멀티플레이 실행 방법 | 09-30 23:0x (미커밋) | NetDuel 생성 시 새 템플릿이 들어감, NetDuel 엔트리가 규약대로 동작 |
| 5-3 | 컨트롤러 `IsLocallyControlled` · `GetInputActor` · `BeginCommand` → bool · `ResolveChoice` "not your choice" | 09-30 23:0x (미커밋) | world 절 컨트롤러 검사 5개 |
| 5-4 | 실행 인자 `-host[=포트]` · `-bind=` · `-join=주소[:포트]` · `-name=` (게임 인스턴스가 프로세스 명령줄에서 읽음) | 09-30 23:0x (미커밋) | NetDuel 런처 2개: `hosting on port 47791 as Host` · `joining 127.0.0.1:47791 as Guest` → slot 1 |
| 5-6 | 검증: `net.test world`(35) · `net.host/join -world`(2 프로세스 world 모드) · NetDuel 런처 2개 결투 완주 | 09-30 23:13 | `Files/2026-09-30_phase5_results.txt` — 런처 결투 `net-duel: OK (seq 13, checksum 9198514108868742568, winner team 0)`, 양쪽 종료 0 · 오류 로그 0. 메인 트리 반영 뒤 `gmtest` 156 · 101/101 · 2 프로세스 kernel · world OK · 엔진 런처 30 초 종료 0 |
| N-1 | `net.test` 를 GFW 모듈(`GameplayNetSelfTest.cpp`)의 `HAutoConsoleCommand` 로 이동. `NetCommands.cpp` 는 `net.host` · `net.join` | 09-30 23:0x (미커밋) | JGConsole 도움말에 세 명령, `net.test all` 101/101 |
| P5-a | 월드 이름 NAME_NONE 왕복 버그 — `Welcome` · `Travel` 의 이름을 없음 = 빈 문자열로 주고받음 | 09-30 22:4x | 로비 입장 뒤 Travel 이 1번만(수정 전 2번: 빈 이름 월드로 한 번 더) — world 절 `travels 1 / 1` |

---

## 제외 항목

| 항목 | 이유 |
|---|---|
| 핸드셰이크 "빌드 ID" 비교 | 기계마다 따로 빌드하면 빌드 시각이 달라 늘 거부 → 프로토콜 버전 + 규칙 지문으로 대체 |
| 규칙 지문을 `Hello` 에 | 로비에서는 호스트에 GameMaster 가 없을 수 있다 → `Welcome` · `StartGame` 에서 클라가 비교 |
| 전송 계층의 핑 · 타임아웃 | 시간을 세션이 안다 → 세션으로 이동 |
| 공유 PCH 에 `winsock2.h` 선행 | Winsock 1.1 로 충분. 전체 재빌드 · 세션 충돌 회피 |
| 동시 입력을 위한 커널 확장(결정 4) | 호스트가 도착 순으로 줄 세우므로 커널 입력 모델과 네트워크 구조는 독립(구조설계 §11) |
| 멀티 중 되돌리기 방식 결정(결정 5) | 사용자: 구현에 영향 없음 → 결정 항목에서 제외. 가드(2-7)만 |
| `Network/` 별도 모듈 · 네트워크 스레드 | 소켓 사용처가 하나, 턴제 트래픽은 프레임당 폴링으로 충분 |
