# 리슨 서버 런처 검증용 게임 (NetDuel)

Server Phase 5-6 검증 장치다. 게임 콘텐츠가 아니다. 엔진 밖 게임 프로젝트에서 실제 런처 두 개(호스트 · 클라)를 띄워, 실행 인자(5-4) → 게임 인스턴스 세션(5-1) → GameMasterActor(5-2) → 컨트롤러(5-3) → 엔트리 규약(5-2b) 경로로 한 판을 끝까지 두는지 본다.

| 파일 | 내용 |
|---|---|
| `NetDuelEntryActor.cpp` | 2인 결투. 팀마다 유닛 2(체력 6), 공격 = 난수 피해 1~3, 체력 0 이면 파괴, 한 팀이 전멸하면 `FinishGame`. 엔트리 규약대로 규칙 등록은 모든 기계, 초기 상태 + `StartGame` 은 `IsAuthority()` 일 때만. 스크립트 액터가 로컬 플레이어 대신 0.1 초마다 컨트롤러로 무작위 합법 명령을 넣고, 끝나면 `NetDuel: finished mode=… slot=… seq=… checksum=… winner=… submitted=… desyncs=…` 를 한 번 남긴다 |
| `run_net_duel.sh` | 호스트 `JGLauncher.exe -host=47791 -bind=127.0.0.1 -name=Host` 를 crashwalk 로 띄우고, 로그에 `hosting on port` 가 보이면 클라 `-join=127.0.0.1:47791 -name=Guest` 를 띄운다. 제한 시간(기본 60 초) 뒤 둘 다 WM_CLOSE. 두 `finished` 줄의 순번 · 체크섬 · 승자가 같고 종료 코드 0 · `[error]`/`[critical]` 0 이면 OK |

## 재현

```
<엔진>\Build\BatchFiles\CreateGameProject.bat <dir>\NetDuel NetDuel
copy NetDuelEntryActor.cpp → <dir>\NetDuel\Source\NetDuel\
<dir>\NetDuel\GenerateProjectFiles.bat
MSBuild <dir>\NetDuel\NetDuel.sln -m -p:Configuration=DevelopEngine -p:Platform=x64      (처음 약 5분, 엔진 포함)
bash run_net_duel.sh <dir>\NetDuel\Bin\DevelopEngine <엔진>\Document\Memory\tools\crashwalk\crashwalk.exe <출력 폴더> 60
```

기대 결과: `net-duel: OK (seq N, checksum C, winner team W)`. 클라 로그에 `JGGameInstance: joining 127.0.0.1:47791 as Guest` → `PGameplayClientSession: joined as slot 1 (game in progress)` → `document applied` (호스트가 이미 시작했으므로 진행 중 입장), 호스트 로그에 `slot 1 joined (Guest)` 가 보인다.

두 런처가 같은 `Bin` 을 쓰므로 `jg_log.txt` · `imgui.ini` 는 서로 덮어쓴다. 증거는 crashwalk 표준 출력(`host.txt` · `client.txt`)이다.
