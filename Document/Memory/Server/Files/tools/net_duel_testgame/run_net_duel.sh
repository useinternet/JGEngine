#!/usr/bin/env bash
# 리슨 서버 Phase 5-6: 런처 2개(호스트 · 클라)로 결투 한 판을 끝까지 둔다.
# 호스트는 -host -bind=127.0.0.1, 클라는 호스트가 포트를 연 뒤 -join=127.0.0.1 로 띄운다. 둘 다 crashwalk 로 감싸
# 표준 출력(로그)을 따로 받고 제한 시간 뒤 WM_CLOSE 로 닫는다. 두 로그의 "NetDuel: finished" 줄(순번 · 체크섬 · 승자)이 같고
# 종료 코드 0, [error]/[critical] 0 이면 OK.
# usage: run_net_duel.sh <게임 Bin/DevelopEngine (Windows 경로)> <crashwalk.exe> [출력 폴더] [제한 초=60]
BIN="${1:?usage: run_net_duel.sh <game Bin/DevelopEngine> <crashwalk.exe> [outdir] [seconds]}"
CRASHWALK="${2:?crashwalk.exe path}"
OUT="${3:-$(pwd)/net_duel_out}"
LIMIT="${4:-60}"
PORT=47791
mkdir -p "$OUT"
export MSYS_NO_PATHCONV=1

LAUNCHER="$BIN\\JGLauncher.exe"
"$CRASHWALK" "$LAUNCHER -host=$PORT -bind=127.0.0.1 -name=Host" "$BIN" "$LIMIT" > "$OUT/host.txt" 2>&1 &
HOST_PID=$!

# 클라는 한 번만 접속한다 (거부되면 Closed). 호스트가 포트를 연 뒤에 띄운다.
WAITED=0
until grep -q "hosting on port $PORT" "$OUT/host.txt" 2>/dev/null; do
	sleep 0.5
	WAITED=$((WAITED + 1))
	if [ $WAITED -gt 120 ]; then
		echo "net-duel: host did not open port $PORT"
		wait $HOST_PID
		exit 2
	fi
done

"$CRASHWALK" "$LAUNCHER -join=127.0.0.1:$PORT -name=Guest" "$BIN" "$LIMIT" > "$OUT/client.txt" 2>&1 &
CLIENT_PID=$!

wait $HOST_PID
HOST_EXIT=$?
wait $CLIENT_PID
CLIENT_EXIT=$?

field()
{
	echo "$1" | sed -n "s/.* $2=\([0-9-]*\).*/\1/p"
}

H_LINE=$(grep "NetDuel: finished" "$OUT/host.txt" | tail -1 | sed 's/.*NetDuel: /NetDuel: /')
C_LINE=$(grep "NetDuel: finished" "$OUT/client.txt" | tail -1 | sed 's/.*NetDuel: /NetDuel: /')
H_ERRORS=$(grep -cE "\]\[(error|critical)\]" "$OUT/host.txt")
C_ERRORS=$(grep -cE "\]\[(error|critical)\]" "$OUT/client.txt")
H_CODE=$(grep -oE "Process exited with code [0-9-]+" "$OUT/host.txt" | tail -1)
C_CODE=$(grep -oE "Process exited with code [0-9-]+" "$OUT/client.txt" | tail -1)
echo "host:   $H_LINE | $H_CODE | errors $H_ERRORS (crashwalk exit $HOST_EXIT)"
echo "client: $C_LINE | $C_CODE | errors $C_ERRORS (crashwalk exit $CLIENT_EXIT)"

HS=$(field "$H_LINE" seq); HC=$(field "$H_LINE" checksum); HW=$(field "$H_LINE" winner)
CS=$(field "$C_LINE" seq); CC=$(field "$C_LINE" checksum); CW=$(field "$C_LINE" winner); CD=$(field "$C_LINE" desyncs)

if [ -n "$HS" ] && [ "$HS" = "$CS" ] && [ "$HC" = "$CC" ] && [ "$HW" = "$CW" ] && [ "$CD" = "0" ] \
	&& [ "$H_CODE" = "Process exited with code 0" ] && [ "$C_CODE" = "Process exited with code 0" ] \
	&& [ "$H_ERRORS" = "0" ] && [ "$C_ERRORS" = "0" ]; then
	echo "net-duel: OK (seq $HS, checksum $HC, winner team $HW)"
	exit 0
fi
echo "net-duel: FAILED"
exit 1
