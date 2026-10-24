#!/usr/bin/env bash
# 대기방 기능 묶음(D-38) Session Lobby 창 확인: 프로젝트 없이 띄운 엔진 에디터 두 개(A, B)를 로비 창으로만 조작한다.
#   월드가 없어 세션이 판 시작 전(Ready) 그대로다 — 세션 공유 데이터 · 준비 표시 · 시작 전 나간 자리 비우기를 본다.
#   순서는 net_lobby_room_driver.ps1 머리말. B 는 엔진 Bin 복사본(<Bin>B)에서 띄운다(jg_log.txt · imgui.ini 가 섞이지 않게).
# 창 입력은 PostMessage 로만 보낸다 — 검증 빌드에 워크트리 전용 ImGui 훅이 있어야 한다(README).
# usage: run_net_lobby_room.sh <엔진 Bin/DevelopEngine (Windows 경로)> <crashwalk.exe> <출력 폴더(Windows 경로)> [probe|run] [드라이버 좌표 인자...]
BIN="${1:?usage: run_net_lobby_room.sh <engine Bin/DevelopEngine> <crashwalk.exe> <outdir> [probe|run] [-HostX n -HostY n ...]}"
CRASHWALK="${2:?crashwalk.exe path}"
OUT="${3:?output folder}"
MODE="${4:-run}"
shift 4
HERE="$(cd "$(dirname "$0")" && pwd)"
BINB="${BIN}B"
LIMIT=180
export MSYS_NO_PATHCONV=1

mkdir -p "$(cygpath -u "$OUT")"
rm -rf "$(cygpath -u "$BINB")"
cp -r "$(cygpath -u "$BIN")" "$(cygpath -u "$BINB")"
cp "$HERE/imgui_room.ini" "$(cygpath -u "$BIN")/imgui.ini"
cp "$HERE/imgui_room.ini" "$(cygpath -u "$BINB")/imgui.ini"

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
powershell -NoProfile -ExecutionPolicy Bypass -File "$(cygpath -w "$HERE/net_lobby_room_driver.ps1")" \
	-ExeA "$BIN\\JGLauncher.exe" -ExeB "$BINB\\JGLauncher.exe" -OutPrefix "$OUT\\room" $PROBE "$@" > "$(cygpath -u "$OUT")/driver.txt" 2>&1
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
	grep -E "PGameplay(Host|Client)Session: (listen|slot|joined|released|closed)|PGameplaySession: closed|SetSessionData|Process exited" "$f" | sed 's/^\[:[0-9]*:\]//'
	echo "errors: $(grep -cE '\]\[(error|critical)\]' "$f")"
}
summary A.txt $A_EXIT
summary B.txt $B_EXIT
cat driver.txt
