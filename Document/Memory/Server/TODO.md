# Server(네트워크 · 리슨 서버) TODO

작성 2026-09-30. `Files/Network_TODO.md`(09-29 작성 · 09-30 11:45 갱신)를 통합 · 최신화한 것. 항목 ID 는 그대로 쓴다(Phase-번호). 현황은 `현황.md`.
결정(09-29, 사용자): 방안 A(호스트 권한 명령 중계 + 결정론 복제) · 위치 `GameFrameWorks/Network/` · 전송 Winsock TCP(`INetTransport` 뒤) · 이름 `PGameplaySession` · `INetTransport` · `PNet*`.
규칙: 한 항목이 끝나면 완료 이력으로 옮기고 `현황.md` §4 에 한 줄. 커밋은 사용자가 한다. 사용자 지시 범위는 "Phase 4 까지" — Phase 5 는 착수 지시를 받은 뒤 시작한다.

---

## 다음에 할 일 Top 5

| 순서 | ID | 항목 | 착수 조건 | 완료 기준 |
|---|---|---|---|---|
| 1 | V-1 | 현 Bin(09-30 11:56 재빌드) 에서 3종 검사 재실행 | 없음 (읽기 전용 검사) | `gmtest` 111/111 · `net.test all` 66/66 · `run_two_process.sh` OK, 결과를 `Files/2026-09-29_net_phase0-4_results.txt` 끝에 추가 |
| 2 | C-1 | Phase 0–4 커밋 (0-7 · 1-6 · 2-9 · 3-6 · 4-3) — 사용자 | V-1 통과 | GameFrameWorks 모듈 전체 + `Core/ConsoleCommand/` + `JGConsole/Main.cpp` + `NetCommands.cpp` + `Bin` 산출물이 한 커밋에. HEAD 에서 클린 빌드 0 오류 |
| 3 | 5-1 | `JGGameInstance` 가 세션 소유 · 틱 순서 세션 → 월드 | 사용자 Phase 5 착수 지시. GFW 트랙과 `Core/GameInstance.*` 편집 조율 | Standalone 세션이 기본 생성되고 `Tick` 맨 앞에서 폴링. `LoadWorld` 가 호스트면 `NotifyTravel`. 종료 시 세션 · 전송 정리. `gmtest` 월드 절 유지 |
| 4 | 5-2 | `JGGameMasterActor` 경유 — 붙기/떼기 · `Submit` 경로 | 5-1 | `executeNow` 가 `_gameMaster->Submit` 대신 세션 `SubmitLocal`. 결과에 `Sent` 추가, 거절은 `OnLocalCommandRejected`. 원격 · 승인 명령은 입력 정책을 거치지 않음. `gmtest` · `net.test all` 유지 |
| 5 | 5-2b · 5-3 | 엔트리 규약 · 컨트롤러 소유 검사 | 5-2 | 규칙 등록은 전원, 초기 상태 + `StartGame` 은 `IsAuthority()` 만(가이드 한 줄). 컨트롤러는 `IsLocallyControlled` 인 행동자만 초안 · 선택 응답 |

---

## 미완료 항목

| ID | 항목 | 우선순위 | 상태 | 선행조건 | 완료 기준 | 비고 |
|---|---|---|---|---|---|---|
| V-1 | 현 Bin 재검증 (`gmtest` · `net.test all` · 2 프로세스) | 상 | 미착수 | — | 셋 통과, 결과 파일에 기록 | 마지막 기록은 09-30 00:31. 그 뒤 다른 트랙이 Bin 재빌드 |
| C-1 (0-7 · 1-6 · 2-9 · 3-6 · 4-3) | Phase 0–4 커밋 | 상 | 미완 — 사용자 | V-1 | `Network/` · `NetCommands.cpp` 추적, GameMaster 변경 · `Main.cpp` · `Core/ConsoleCommand/` 함께 | 세 트랙(GFW Phase 1 · DevConsole · Network) 변경이 섞여 있어 한 번에 |
| 5-1 | `JGGameInstance` 세션 소유 · 틱 순서 · `Travel` 알림 · 종료 정리 | 상 | 미착수 | 사용자 착수 지시, GFW 조율 | Top 5 #3 | `Core/GameInstance.h` 에 세션 없음(코드 확인) |
| 5-2 | `JGGameMasterActor` 붙기/떼기 · `executeNow` → 세션 · `Sent` · 거절 알림 | 상 | 미착수 | 5-1 | Top 5 #4 | `GameMasterActor.cpp` `executeNow` 가 `Submit` 직접 호출(코드 확인). GFW 2-4 시퀀서 확장과 같은 파일 |
| 5-2b | 엔트리 액터 규약 문서화 | 중 | 미착수 | 5-2 | `Document/Memory/GameModule/Files/게임프로젝트_생성가이드.md` 에 한 줄 + 템플릿 엔트리 액터가 `IsAuthority()` 분기 | 게임 코드가 하는 일은 이것과 "행동자 → 조작 주체 함수" 둘뿐 |
| 5-3 | `JGGameplayControllerActor` 소유 검사 | 중 | 미착수 | 5-2 | `IsLocallyControlled` 아닌 행동자는 `BeginCommand` · `ResolveChoice` 거절 | GFW 2-5 피킹이 Controller 에 붙는다 — 조율 |
| 5-4 | 실행 인자 `-host [port]` · `-join <주소>` · `-name` | 중 | 미착수 · **결정 대기** | 게임 실행 호스트 결정(게임 프로젝트 트랙) | 인자로 ListenServer / Client 세션 생성, 없으면 Standalone | `JGLauncher/Main.cpp` 는 인자 없는 `main()` · JGEditor 호스트 |
| 5-5 | 최소 로비 — 슬롯 목록 · 준비 · 시작 | 중 | 미착수 · **결정 대기** | 위치 결정(엔진 개발용 뷰 vs 게임), 5-1 | 호스트가 슬롯 ↔ 조작 주체 배정 · `StartGame`, 클라는 슬롯 표 표시 | 엔진 API 는 있음(`GetSlots` · `SetSlotControllers` · `OnSlotsChanged` · `StartGame`) |
| 5-6 | 검증 — 프로세스 2개(호스트 · 클라)로 한 전투 완주 | 상 | 미착수 | 5-1 ~ 5-5 | 양쪽 종료 코드 0 · 로그 오류 0 · 마지막 체크섬 동일 | 헤드리스 부분은 `net.test` 로, 월드 경유는 실행 파일 2개로 |
| 5-7 | 커밋 | — | 사용자 | 5-6 | — | |
| N-1 | `net.test` 를 GameFrameWorks 모듈 안(`Network/` 쪽 .cpp)의 `HAutoConsoleCommand` 로 이동 | 하 | 미착수 | — | JGConsole 도움말에 `net.test` 가 GameFrameWorks 연결 뒤 뜨고 66/66 유지. `net.host` · `net.join` 은 JGConsole 에 남김 | 사용자 결정(DevConsole 09-30) "모듈 테스트 명령은 그 모듈에". 시점은 이 트랙이 정함 — Phase 5 착수 때 함께 |
| N-2 | 문서 이동 뒤 코드 주석 경로 갱신 | 하 | 미착수 | 문서 이동 완료 | `NetTcpTransport.cpp:5` 의 `Files/2026-09-29_winsock_include_probe.txt` → 새 경로 | 이동은 사용자가 수행 |

---

## 게임 프로젝트 영역 (엔진 범위 외)

| 항목 | 엔진이 제공하는 것 | 게임이 하는 것 |
|---|---|---|
| 행동자 → 조작 주체 | `PGameMaster::SetTeamOfActorFunction` · `TeamOfActor` | 함수 하나 등록 (팀 = 조작 주체 번호) |
| 초기 상태 · 시작 | `PGameplaySession::IsAuthority` · `StartGame(seed)` | 엔트리 액터에서 권한일 때만 초기 상태 구성 + 시작 |
| 슬롯 ↔ 조작 주체 배정 | `PGameplayHostSession::SetSlotControllers` · `OnSlotsChanged` | 로비 화면의 표현 · 배정 규칙 |
| 끊김 정책 | `OnPeerDisconnected(slot)` · `OnClosed(reason)` · `PGameMaster::SetAgent` | 대기 / AI 대리 중 선택 |
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
