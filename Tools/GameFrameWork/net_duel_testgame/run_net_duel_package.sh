#!/usr/bin/env bash
# GP-S1 패키지 네트워크 확인: 쿠킹한 NetDuel 패키지 exe 두 개로 -host / -join (127.0.0.1) 결투 한 판.
# 패키지는 폴더째 옮겨도 되므로 <패키지>_B 사본을 만들어 두 번째 exe 를 띄운다(로그 Saved\Logs\NetDuel.log 가 섞이지 않게).
# 호스트는 -bind=127.0.0.1 로 루프백에만 연다(Windows 방화벽 확인 창이 뜨지 않는다). 방화벽 창이 보이면 아무것도 누르지 않고 멈춘다(종료 코드 3).
# 두 exe 는 crashwalk 로 감싸 제한 시간 뒤 WM_CLOSE 하고 종료 코드를 받는다. 로그는 패키지의 Saved\Logs 에서 가져온다.
# usage: run_net_duel_package.sh <패키지 폴더(Windows 경로, NetDuel.exe 가 있는 곳)> <crashwalk.exe> <출력 폴더> [포트=47811] [제한 초=60]
PKG="${1:?usage: run_net_duel_package.sh <package folder> <crashwalk.exe> <outdir> [port] [seconds]}"
CRASHWALK="${2:?crashwalk.exe path}"
OUT="${3:?output folder}"
PORT="${4:-47811}"
LIMIT="${5:-60}"
PKGB="${PKG}_B"
export MSYS_NO_PATHCONV=1

OUTU="$(cygpath -u "$OUT")"
PKGU="$(cygpath -u "$PKG")"
PKGBU="$(cygpath -u "$PKGB")"
mkdir -p "$OUTU"
rm -rf "$PKGBU" "$PKGU/Saved"
cp -r "$PKGU" "$PKGBU"

"$CRASHWALK" "$PKG\\NetDuel.exe -host=$PORT -bind=127.0.0.1 -name=Host" "$PKG" "$LIMIT" > "$OUTU/host_crashwalk.txt" 2>&1 &
HOST_PID=$!

# 클라는 한 번만 접속한다(거부되면 Closed). 호스트 로그에 포트가 열린 뒤에 띄운다.
WAITED=0
until grep -q "hosting on port $PORT" "$PKGU/Saved/Logs/NetDuel.log" 2>/dev/null; do
	sleep 0.5
	WAITED=$((WAITED + 1))
	if [ $WAITED -gt 120 ]; then
		echo "package-duel: host did not open port $PORT"
		wait $HOST_PID
		exit 2
	fi
done

# 방화벽 확인 창이 떠 있으면 멈춘다(사용자가 정한다 — PC 보안 설정은 바꾸지 않는다).
FIREWALL=$(powershell -NoProfile -Command "Get-Process | Where-Object { \$_.MainWindowTitle -match 'Windows 보안 경고|Windows Security Alert|방화벽|Firewall' } | ForEach-Object { \$_.ProcessName + ': ' + \$_.MainWindowTitle }")
if [ -n "$FIREWALL" ]; then
	echo "package-duel: a firewall window is open - stopped without touching it: $FIREWALL"
	exit 3
fi

"$CRASHWALK" "$PKGB\\NetDuel.exe -join=127.0.0.1:$PORT -name=Guest" "$PKGB" "$LIMIT" > "$OUTU/client_crashwalk.txt" 2>&1 &
CLIENT_PID=$!

wait $HOST_PID
HOST_EXIT=$?
wait $CLIENT_PID
CLIENT_EXIT=$?

cp "$PKGU/Saved/Logs/NetDuel.log" "$OUTU/host.log"
cp "$PKGBU/Saved/Logs/NetDuel.log" "$OUTU/client.log"

field()
{
	echo "$1" | sed -n "s/.* $2=\([0-9-]*\).*/\1/p"
}
summary()
{
	echo "== $1 (crashwalk exit $2)"
	grep -E "Package NetDuel|JGGameInstance: (hosting|joining|following)|PGameplay(Host|Client)Session: |PGameplaySession: closed|NetDuel entry actor|NetDuel: finished|live blocks" "$OUTU/$1" | sed 's/^\[:[0-9]*:\]//'
	echo "[error]/[critical]: $(grep -cE '\]\[(error|critical)\]' "$OUTU/$1")"
}
summary host.log $HOST_EXIT
grep -h "Process exited" "$OUTU/host_crashwalk.txt"
summary client.log $CLIENT_EXIT
grep -h "Process exited" "$OUTU/client_crashwalk.txt"

H_LINE=$(grep "NetDuel: finished" "$OUTU/host.log" | tail -1)
C_LINE=$(grep "NetDuel: finished" "$OUTU/client.log" | tail -1)
HS=$(field "$H_LINE" seq); HC=$(field "$H_LINE" checksum); HW=$(field "$H_LINE" winner)
CS=$(field "$C_LINE" seq); CC=$(field "$C_LINE" checksum); CW=$(field "$C_LINE" winner); CD=$(field "$C_LINE" desyncs)
H_CODE=$(grep -h "Process exited with code" "$OUTU/host_crashwalk.txt" | tail -1)
C_CODE=$(grep -h "Process exited with code" "$OUTU/client_crashwalk.txt" | tail -1)
H_ERR=$(grep -cE '\]\[(error|critical)\]' "$OUTU/host.log")
C_ERR=$(grep -cE '\]\[(error|critical)\]' "$OUTU/client.log")
# 세션 공유 데이터(D-38): 방장이 정한 방 정보를 참가자도 같은 값으로 가졌는가
HR=$(echo "$H_LINE" | sed -n "s/.* room=\([^ ]*\).*/\1/p"); CR=$(echo "$C_LINE" | sed -n "s/.* room=\([^ ]*\).*/\1/p")

if [ -n "$HS" ] && [ "$HS" = "$CS" ] && [ "$HC" = "$CC" ] && [ "$HW" = "$CW" ] && [ "$CD" = "0" ] \
	&& [ -n "$HR" ] && [ "$HR" != "-" ] && [ "$HR" = "$CR" ] \
	&& echo "$H_CODE" | grep -q "code 0 " && echo "$C_CODE" | grep -q "code 0 " && [ "$H_ERR" = "0" ] && [ "$C_ERR" = "0" ]; then
	echo "package-duel: OK (seq $HS, checksum $HC, winner team $HW, room $HR)"
	exit 0
fi
echo "package-duel: FAILED"
exit 1
