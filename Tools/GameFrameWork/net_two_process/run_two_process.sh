#!/usr/bin/env bash
# 리슨 서버 2 프로세스 결정론 검사 (Network_TODO Phase 4).
# 호스트 1 + 클라 N(기본 2)을 서로 다른 JGConsole 프로세스로 띄워 TCP(127.0.0.1)로 명령을 진행하고,
# 끝의 순번 · 체크섬이 모두 같은지 비교한다. 각 클라는 승인마다 체크섬도 비교한다(desyncs=0 이어야 한다).
# 모드 world (Server Phase 5-6): 세션을 게임 인스턴스가 갖고 월드 · GameMasterActor · 컨트롤러를 거친다.
#   호스트가 월드 둘을 차례로 로드(명령 수 절반씩)하고, 모든 프로세스가 worlds=2 여야 한다.
# 대기방(대기방 기능 묶음 D-38): 호스트는 세션 공유 데이터를 정하고 클라 모두가 준비를 켜야 시작한다(절반에서 데이터를 하나 더 바꾼다).
#   모든 프로세스의 data(세션 공유 데이터 요약)가 같고, 판이 시작돼 준비 표시가 지워져 ready=0 이어야 한다.
#   손님 수 > 0 이면 진짜 클라보다 먼저 손님이 하나씩 들어왔다(Welcome) 나간다. 판 시작 전에 나간 자리가 비워져야 진짜 클라가 모두 들어온다.
# 재접속(창을 다시 띄워도 같은 자리로 재접속 ER-026, kernel 모드만): 재접속 1 이면 Client1 이 명령 수의 1/3 에서 창을 닫듯 나가고,
#   같은 토큰을 준 새 프로세스(Client1)가 판 도중 · 꽉 찬 방에 다시 들어온다. 같은 자리(slot)를 받고 끝 결과가 모두와 같아야 한다.
# usage: run_two_process.sh <Bin/DevelopEngine 경로> [포트] [명령 수] [출력 폴더] [kernel|world] [클라 수=2] [손님 수=0] [재접속 0|1]
#   4명 방(호스트 + 클라 3) + 손님 3: run_two_process.sh <Bin> 47771 1000 <출력> kernel 3 3
#   재접속: run_two_process.sh <Bin> 47771 1000 <출력> kernel 3 0 1
BIN="${1:?usage: run_two_process.sh <Bin/DevelopEngine> [port] [commands] [outdir] [kernel|world] [clients] [guests] [rejoin]}"
PORT="${2:-47771}"
COMMANDS="${3:-1000}"
OUT="${4:-$(pwd)/net_two_process_out}"
MODE="${5:-kernel}"
CLIENTS="${6:-2}"
GUESTS="${7:-0}"
REJOIN="${8:-0}"
PLAYERS=$((CLIENTS + 1))
FLAG=""
if [ "$MODE" = "world" ]; then
	FLAG="-world"
	if [ "$REJOIN" = "1" ]; then
		echo "two-process: rejoin is for the kernel mode"
		exit 2
	fi
fi
HOST_EXTRA=""
REJOIN_TOKEN="two-process-rejoin-token-$PORT"
if [ "$REJOIN" = "1" ]; then
	HOST_EXTRA="-rejoin-grace=60"
fi
mkdir -p "$OUT"
rm -f "$OUT"/host.txt "$OUT"/client*.txt "$OUT"/guest*.txt
cd "$BIN" || exit 2

# 클라는 호스트가 아직 안 떴으면 30 초까지 다시 접속을 시도한다. 디버그 빌드 도구는 abort 대화상자에서 멈추므로 timeout 으로 감싼다.
timeout 400 ./JGConsole.exe net.host $FLAG -port=$PORT -clients=$CLIENTS -commands=$COMMANDS -seed=12345 $HOST_EXTRA > "$OUT/host.txt" 2>&1 &
HOST_PID=$!

# 대기방 손님: 하나씩 들어와 Welcome 을 받고 나간다(앞 손님이 끝난 뒤 다음 손님).
GUESTS_OK=1
for ((i = 1; i <= GUESTS; i++)); do
	timeout 120 ./JGConsole.exe net.join $FLAG -address=127.0.0.1 -port=$PORT -players=$PLAYERS -agent-seed=$((600 + i)) -name=Guest$i -leave-after-welcome > "$OUT/guest$i.txt" 2>&1
	GUEST_EXIT=$?
	GUEST_LINE=$(grep "left after welcome" "$OUT/guest$i.txt" | tail -1)
	echo "$GUEST_LINE (exit $GUEST_EXIT)"
	if [ $GUEST_EXIT -ne 0 ] || [ -z "$GUEST_LINE" ]; then
		GUESTS_OK=0
	fi
done

CLIENT_PIDS=()
for ((i = 1; i <= CLIENTS; i++)); do
	EXTRA=""
	if [ "$REJOIN" = "1" ] && [ $i -eq 1 ]; then
		EXTRA="-token=$REJOIN_TOKEN -leave-at=$((COMMANDS / 3))"
	fi
	timeout 400 ./JGConsole.exe net.join $FLAG -address=127.0.0.1 -port=$PORT -players=$PLAYERS -agent-seed=$((500 + i)) -name=Client$i $EXTRA > "$OUT/client$i.txt" 2>&1 &
	CLIENT_PIDS+=($!)
done

# 재접속: Client1 이 판 도중 나가면(프로세스 끝) 같은 토큰으로 새 프로세스를 띄운다.
REJOIN_OK=1
if [ "$REJOIN" = "1" ]; then
	wait ${CLIENT_PIDS[0]}
	LEAVE_EXIT=$?
	LEAVE_LINE=$(grep "left at seq=" "$OUT/client1.txt" | tail -1)
	echo "$LEAVE_LINE (exit $LEAVE_EXIT)"
	timeout 400 ./JGConsole.exe net.join -address=127.0.0.1 -port=$PORT -players=$PLAYERS -agent-seed=599 -name=Client1 -token=$REJOIN_TOKEN > "$OUT/client1_rejoin.txt" 2>&1 &
	CLIENT_PIDS[0]=$!
	if [ $LEAVE_EXIT -ne 0 ] || [ -z "$LEAVE_LINE" ]; then
		REJOIN_OK=0
	fi
fi

wait $HOST_PID
HOST_EXIT=$?
CLIENT_EXITS=()
for PID in "${CLIENT_PIDS[@]}"; do
	wait $PID
	CLIENT_EXITS+=($?)
done

field()
{
	echo "$1" | sed -n "s/.* $2=\([0-9]*\).*/\1/p"
}

HOST_LINE=$(grep -E "net.host: (world=[^ ]* worlds=[0-9]* )?seq=" "$OUT/host.txt" | tail -1)
echo "$HOST_LINE (exit $HOST_EXIT)"
HS=$(field "$HOST_LINE" seq)
HC=$(field "$HOST_LINE" checksum)
HD=$(field "$HOST_LINE" data)
HR=$(field "$HOST_LINE" ready)

ALL_OK=$((GUESTS_OK * REJOIN_OK))
if [ -z "$HS" ] || [ -z "$HD" ] || [ "$HR" != "0" ] || [ $HOST_EXIT -ne 0 ]; then
	ALL_OK=0
fi
if [ "$MODE" = "world" ] && [ "$(field "$HOST_LINE" worlds)" != "2" ]; then
	ALL_OK=0
fi
if [ "$REJOIN" = "1" ] && [ "$(field "$HOST_LINE" rejoins)" != "1" ]; then
	ALL_OK=0
fi
for ((i = 1; i <= CLIENTS; i++)); do
	FILE="$OUT/client$i.txt"
	if [ "$REJOIN" = "1" ] && [ $i -eq 1 ]; then
		FILE="$OUT/client1_rejoin.txt"
	fi
	LINE=$(grep "net.join: name=.* seq=" "$FILE" | tail -1)
	EXIT=${CLIENT_EXITS[$((i - 1))]}
	echo "$LINE (exit $EXIT)"
	if [ -z "$LINE" ] || [ "$(field "$LINE" seq)" != "$HS" ] || [ "$(field "$LINE" checksum)" != "$HC" ] || [ "$(field "$LINE" desyncs)" != "0" ] \
		|| [ "$(field "$LINE" data)" != "$HD" ] || [ "$(field "$LINE" ready)" != "0" ] || [ "$EXIT" -ne 0 ]; then
		ALL_OK=0
	fi
	if [ "$MODE" = "world" ] && [ "$(field "$LINE" worlds)" != "2" ]; then
		ALL_OK=0
	fi
	# 다시 들어온 Client1 은 나가기 전과 같은 자리여야 한다
	if [ "$REJOIN" = "1" ] && [ $i -eq 1 ] && [ "$(field "$LINE" slot)" != "$(field "$LEAVE_LINE" slot)" ]; then
		ALL_OK=0
	fi
done

if [ $ALL_OK -eq 1 ]; then
	echo "two-process: OK ($MODE, clients $CLIENTS, guests $GUESTS, rejoin $REJOIN, seq $HS, checksum $HC, data $HD)"
	exit 0
fi
echo "two-process: FAILED"
exit 1
