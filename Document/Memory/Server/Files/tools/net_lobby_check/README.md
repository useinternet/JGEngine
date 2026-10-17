# 세션 로비 창 검증 (Server 5-5)

JGEditor 개발용 창 `Windows/Session Lobby`(`Source/Editor/JGEditor/Widgets/SessionLobby.*`)를 실제 런처 두 개에서 창으로만 조작해 확인하는 장치다. 게임은 `../net_duel_testgame`(NetDuel)을 쓴다.

| 파일 | 내용 |
|---|---|
| `run_net_lobby.sh` | 게임 Bin 을 `<Bin>B` 로 복사(로그 · `imgui.ini` 분리) → 두 Bin 에 `imgui.ini` 를 넣고 `JG_UITEST=1` 로 런처 두 개를 인자 없이(싱글) crashwalk 아래 띄움 → 드라이버 실행 → 두 로그 요약(세션 줄 · `NetDuel: finished` · 오류 수 · 종료 코드) |
| `net_lobby_ui_driver.ps1` | 두 창을 찾아 PostMessage 로만 클릭: A "Host on this machine only" + Host → B Join → A Reload World → B Leave. 단계마다 PrintWindow 캡처, 끝에 WM_CLOSE. `-Probe` 는 시작 캡처만 남기고 닫는다(좌표 읽기용) |
| `imgui.ini` | 창 배치: Scene Viewport (10,30) 880x560 · Session Lobby (900,30) 800x560 · DevConsole (10,600) · Gameplay DevView (900,600). `[JGWidget][Open]` 으로 로비 창을 시작할 때 연다. 멀티 뷰포트가 켜져 있어 메인 창 클라이언트(약 1711x1029) 밖으로 나가는 창은 별도 OS 창이 된다 — 그 창은 메인 창 캡처 · 클릭에 잡히지 않으므로 전부 안쪽에 둔다 |

## 전제 — 검증 빌드 전용 ImGui 훅

실제 커서가 런처 밖에 있으면 ImGui Win32 백엔드가 보낸 클릭을 버린다. **스크래치 워크트리에서만** `Document/Memory/GameFrameWorks/Files/tools/datatable_uitest_hook.patch.txt` 를 적용한다(주석 줄을 빼고 `git apply`). `JG_UITEST=1` 이면 ImGui 가 보낸 마우스 메시지만 쓴다. 메인 트리에 넣지 않는다.

## 재현

```
git worktree add --detach <scratch>\wt HEAD  →  훅 패치 적용  →  PreBuild 2회  →  JGEngine.sln 빌드
<wt>\Build\BatchFiles\CreateGameProject.bat <dir>\NetDuel NetDuel
copy ..\net_duel_testgame\NetDuelEntryActor.cpp → <dir>\NetDuel\Source\NetDuel\
<dir>\NetDuel\GenerateProjectFiles.bat  →  MSBuild <dir>\NetDuel\NetDuel.sln -m -p:Configuration=DevelopEngine -p:Platform=x64
bash run_net_lobby.sh <dir>\NetDuel\Bin\DevelopEngine <엔진>\Document\Memory\tools\crashwalk\crashwalk.exe <출력 폴더> probe
  → <출력>\lobby_A0_start.png 에서 버튼 좌표(창 캡처 픽셀)를 읽는다
bash run_net_lobby.sh <같은 Bin> <crashwalk.exe> <출력 폴더> run -HostX .. -HostY .. -JoinX .. -JoinY .. -LeaveX .. -LeaveY .. -ReloadX .. -ReloadY .. -LocalX .. -LocalY ..
```

버튼 줄은 상태와 상관없이 같은 자리에 있어(쓸 수 없는 버튼은 흐리게) 한 번 읽은 좌표를 끝까지 쓴다.

기대 결과: A 로그 `JGGameInstance` 없이 싱글로 시작 → `PGameplayHostSession: listen server on port 47771` · `adopted a game started outside the session` · `slot 1 joined (Player)` · `sent document to slot 1` → Reload 뒤 `world loaded (NetDuel)` · `NetDuel: finished mode=ListenServer slot=0 …`. B 로그 `PGameplayClientSession: joined as slot 1 (game in progress)` · `document applied` → `JGGameInstance: following the host to world NetDuel` · `game started` → `NetDuel: finished mode=Client slot=1 …` (A 와 같은 seq · checksum) → Leave 뒤 `adopted a game started outside the session`. 양쪽 `Process exited with code 0`, `[error]`/`[critical]` 0.
