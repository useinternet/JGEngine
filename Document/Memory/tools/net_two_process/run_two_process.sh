#!/usr/bin/env bash
# 리슨 서버 2 프로세스 결정론 검사 (Network_TODO Phase 4).
# 호스트 1 + 클라 2 를 서로 다른 JGConsole 프로세스로 띄워 TCP(127.0.0.1)로 명령을 진행하고,
# 끝의 순번 · 체크섬이 셋 다 같은지 비교한다. 각 클라는 승인마다 체크섬도 비교한다(desyncs=0 이어야 한다).
# usage: run_two_process.sh <Bin/DevelopEngine 경로> [포트] [명령 수] [출력 폴더]
BIN="${1:?usage: run_two_process.sh <Bin/DevelopEngine> [port] [commands] [outdir]}"
PORT="${2:-47771}"
COMMANDS="${3:-1000}"
OUT="${4:-$(pwd)/net_two_process_out}"
mkdir -p "$OUT"
cd "$BIN" || exit 2

# 클라는 호스트가 아직 안 떴으면 30 초까지 다시 접속을 시도한다. 디버그 빌드 도구는 abort 대화상자에서 멈추므로 timeout 으로 감싼다.
timeout 400 ./JGConsole.exe net.host -port=$PORT -clients=2 -commands=$COMMANDS -seed=12345 > "$OUT/host.txt" 2>&1 &
HOST_PID=$!
timeout 400 ./JGConsole.exe net.join -address=127.0.0.1 -port=$PORT -players=3 -agent-seed=501 -name=ClientA > "$OUT/clientA.txt" 2>&1 &
A_PID=$!
timeout 400 ./JGConsole.exe net.join -address=127.0.0.1 -port=$PORT -players=3 -agent-seed=502 -name=ClientB > "$OUT/clientB.txt" 2>&1 &
B_PID=$!

wait $HOST_PID
HOST_EXIT=$?
wait $A_PID
A_EXIT=$?
wait $B_PID
B_EXIT=$?

HOST_LINE=$(grep "net.host: seq=" "$OUT/host.txt" | tail -1)
A_LINE=$(grep "net.join: name=" "$OUT/clientA.txt" | tail -1)
B_LINE=$(grep "net.join: name=" "$OUT/clientB.txt" | tail -1)
echo "$HOST_LINE (exit $HOST_EXIT)"
echo "$A_LINE (exit $A_EXIT)"
echo "$B_LINE (exit $B_EXIT)"

field()
{
	echo "$1" | sed -n "s/.* $2=\([0-9]*\).*/\1/p"
}

HS=$(field "$HOST_LINE" seq)
HC=$(field "$HOST_LINE" checksum)
AS=$(field "$A_LINE" seq)
AC=$(field "$A_LINE" checksum)
AD=$(field "$A_LINE" desyncs)
BS=$(field "$B_LINE" seq)
BC=$(field "$B_LINE" checksum)
BD=$(field "$B_LINE" desyncs)

if [ -n "$HS" ] && [ "$HS" = "$AS" ] && [ "$HS" = "$BS" ] && [ "$HC" = "$AC" ] && [ "$HC" = "$BC" ] \
	&& [ "$AD" = "0" ] && [ "$BD" = "0" ] && [ $HOST_EXIT -eq 0 ] && [ $A_EXIT -eq 0 ] && [ $B_EXIT -eq 0 ]; then
	echo "two-process: OK (seq $HS, checksum $HC)"
	exit 0
fi
echo "two-process: FAILED"
exit 1
