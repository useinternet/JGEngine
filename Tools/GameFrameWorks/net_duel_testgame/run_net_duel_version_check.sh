#!/usr/bin/env bash
# 대기방 기능 묶음(D-38) 프로토콜 버전 확인: 프로토콜이 다른 두 NetDuel 패키지(옛 = 버전 1, 새 = 버전 2)를 서로 붙인다.
#   두 방향(새 호스트 + 옛 참가자, 옛 호스트 + 새 참가자) 모두 참가자가 입장 때 "refused: protocol a != b" 로 거절되고,
#   호스트는 혼자 판을 이어 가며, 두 exe 모두 제한 시간 뒤 WM_CLOSE 로 종료 코드 0 이어야 한다(멈춤 · 어긋남 없음).
# 패키지마다 로그는 그 폴더의 Saved\Logs\NetDuel.log. 호스트는 -bind=127.0.0.1(방화벽 창 없음).
# usage: run_net_duel_version_check.sh <새 패키지 폴더> <옛 패키지 폴더> <crashwalk.exe> <출력 폴더> [포트=47812] [제한 초=25]
NEW="${1:?usage: run_net_duel_version_check.sh <new package> <old package> <crashwalk.exe> <outdir> [port] [seconds]}"
OLD="${2:?old package folder}"
CRASHWALK="${3:?crashwalk.exe path}"
OUT="${4:?output folder}"
PORT="${5:-47812}"
LIMIT="${6:-25}"
export MSYS_NO_PATHCONV=1
OUTU="$(cygpath -u "$OUT")"
mkdir -p "$OUTU"

# run <이름> <호스트 패키지> <참가자 패키지>
run()
{
	local name="$1" hostPkg="$2" clientPkg="$3"
	local hostLog="$(cygpath -u "$hostPkg")/Saved/Logs/NetDuel.log"
	local clientLog="$(cygpath -u "$clientPkg")/Saved/Logs/NetDuel.log"
	rm -rf "$(cygpath -u "$hostPkg")/Saved" "$(cygpath -u "$clientPkg")/Saved"

	"$CRASHWALK" "$hostPkg\\NetDuel.exe -host=$PORT -bind=127.0.0.1 -name=Host" "$hostPkg" "$LIMIT" > "$OUTU/${name}_host_crashwalk.txt" 2>&1 &
	local hostPid=$!
	local waited=0
	until grep -q "hosting on port $PORT" "$hostLog" 2>/dev/null; do
		sleep 0.5
		waited=$((waited + 1))
		if [ $waited -gt 120 ]; then
			echo "$name: host did not open port $PORT"
			wait $hostPid
			return 1
		fi
	done
	"$CRASHWALK" "$clientPkg\\NetDuel.exe -join=127.0.0.1:$PORT -name=Guest" "$clientPkg" "$LIMIT" > "$OUTU/${name}_client_crashwalk.txt" 2>&1 &
	local clientPid=$!
	wait $hostPid
	wait $clientPid

	cp "$hostLog" "$OUTU/${name}_host.log"
	cp "$clientLog" "$OUTU/${name}_client.log"
	local refusedClient=$(grep -oE "refused: protocol [0-9]+ != [0-9]+" "$OUTU/${name}_client.log" | tail -1)
	local refusedHost=$(grep -oE "refused peer [0-9]+: protocol [0-9]+ != [0-9]+" "$OUTU/${name}_host.log" | tail -1)
	local hostCode=$(grep -h "Process exited with code" "$OUTU/${name}_host_crashwalk.txt" | tail -1)
	local clientCode=$(grep -h "Process exited with code" "$OUTU/${name}_client_crashwalk.txt" | tail -1)
	echo "== $name"
	echo "host:   ${refusedHost:-(no refusal)} | $hostCode"
	echo "client: ${refusedClient:-(no refusal)} | $clientCode"
	if [ -n "$refusedClient" ] && [ -n "$refusedHost" ] && echo "$hostCode" | grep -q "code 0 " && echo "$clientCode" | grep -q "code 0 "; then
		echo "$name: OK"
		return 0
	fi
	echo "$name: FAILED"
	return 1
}

FAILED=0
run "new_host_old_client" "$NEW" "$OLD" || FAILED=1
run "old_host_new_client" "$OLD" "$NEW" || FAILED=1
if [ $FAILED -eq 0 ]; then
	echo "version-check: OK (both directions refused at join, exit 0)"
	exit 0
fi
echo "version-check: FAILED"
exit 1
