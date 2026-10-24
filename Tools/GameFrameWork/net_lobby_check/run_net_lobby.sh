#!/usr/bin/env bash
# Server 5-5 세션 로비 창 검증: 같은 게임 프로젝트(NetDuel)의 런처 두 개를 인자 없이(싱글) 띄우고 로비 창으로만
#   A Host(127.0.0.1) → B Join → A Reload World(지금 참가자로 새 판) → B Leave 를 한다.
# 런처 B 는 Bin 복사본(<Bin>B)에서 띄운다(jg_log.txt · imgui.ini 가 섞이지 않게). 둘 다 crashwalk 로 감싸 표준 출력(로그)과 종료 코드를 받는다.
# 창 입력은 net_lobby_ui_driver.ps1 이 PostMessage 로만 보낸다 — 검증 빌드에 워크트리 전용 ImGui 훅이 있어야 한다(README).
# usage: run_net_lobby.sh <게임 Bin/DevelopEngine (Windows 경로)> <crashwalk.exe> <출력 폴더(Windows 경로)> [probe|run] [드라이버 좌표 인자...]
BIN="${1:?usage: run_net_lobby.sh <game Bin/DevelopEngine> <crashwalk.exe> <outdir> [probe|run] [-HostX n -HostY n ...]}"
CRASHWALK="${2:?crashwalk.exe path}"
OUT="${3:?output folder}"
MODE="${4:-run}"
shift 4
HERE="$(cd "$(dirname "$0")" && pwd)"
BINB="${BIN}B"
LIMIT=240
export MSYS_NO_PATHCONV=1

mkdir -p "$(cygpath -u "$OUT")"
rm -rf "$(cygpath -u "$BINB")"
cp -r "$(cygpath -u "$BIN")" "$(cygpath -u "$BINB")"
cp "$HERE/imgui.ini" "$(cygpath -u "$BIN")/imgui.ini"
cp "$HERE/imgui.ini" "$(cygpath -u "$BINB")/imgui.ini"

# ImGui 훅: 보낸 마우스 메시지만 쓴다(사용자 커서를 움직이지 않는다)
export JG_UITEST=1
"$CRASHWALK" "$BIN\\JGLauncher.exe" "$BIN" $LIMIT > "$(cygpath -u "$OUT")/A.txt" 2>&1 &
PA=$!
"$CRASHWALK" "$BINB\\JGLauncher.exe" "$BINB" $LIMIT > "$(cygpath -u "$OUT")/B.txt" 2>&1 &
PB=$!

PROBE=""
if [ "$MODE" = "probe" ]; then
	PROBE="-Probe"
fi
powershell -NoProfile -ExecutionPolicy Bypass -File "$(cygpath -w "$HERE/net_lobby_ui_driver.ps1")" \
	-ExeA "$BIN\\JGLauncher.exe" -ExeB "$BINB\\JGLauncher.exe" -OutPrefix "$OUT\\lobby" $PROBE "$@" > "$(cygpath -u "$OUT")/driver.txt" 2>&1
echo "driver exit $?"

wait $PA
A_EXIT=$?
wait $PB
B_EXIT=$?

cd "$(cygpath -u "$OUT")" || exit 2
summary()
{
	local f="$1"
	echo "== $f (crashwalk exit $2)"
	grep -E "JGGameInstance: (hosting|joining|following)|PGameplay(Host|Client)Session: (listen|slot|joined|document|game started|adopted|closed)|PGameplaySession: closed|NetDuel: finished|world loaded|Process exited" "$f" | sed 's/^\[:[0-9]*:\]//'
	echo "errors: $(grep -cE '\]\[(error|critical)\]' "$f")"
}
summary A.txt $A_EXIT
summary B.txt $B_EXIT
cat driver.txt
