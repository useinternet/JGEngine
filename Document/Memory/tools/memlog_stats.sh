#!/usr/bin/env bash
# jg_log.txt 의 [Memory] 할당/해제 줄을 리플레이해 풀 사용 통계를 낸다. (Git Bash, awk)
# 사용: bash Document/Memory/tools/memlog_stats.sh <jg_log 사본>  [샘플 간격 줄 수, 기본 40000]
# 주의: Bin/DevelopEngine/jg_log.txt 는 실행마다 덮어써지므로 먼저 복사한 뒤 그 사본을 넘길 것.
# 주의: 2026-09-28(Memory_TODO 1-1)부터 할당/해제 줄은 JG_MEMORY_TRACE 빌드에서만 나온다.
#       Source/Runtime/Core/Memory/MemoryPool.cpp 맨 위 `#define JG_MEMORY_TRACE 0` 을 1 로 바꿔 빌드한 뒤 실행한 로그를 넘길 것.
#       기본 빌드 로그를 넘기면 allocs/deallocs 가 0 으로 나온다(청크 생성·종료 줄과 Dismatch 경고만 남는다).
set -euo pipefail
F="${1:?jg_log 파일 경로}"
STEP="${2:-40000}"

echo "== 총계 =="
echo "lines: $(wc -l < "$F")"
echo "allocs: $(grep -c '\] Allocated ' "$F")  deallocs: $(grep -c '\] Deallocated ' "$F")  dismatch: $(grep -c 'Dismatch' "$F")"
echo "chunk init: $(grep -c 'Chunk Initi' "$F")  chunk shutdown: $(grep -c 'Chunk Shutdown' "$F")  error/critical: $(grep -cE '\[(error|critical)\]' "$F" || true)"
echo "memory lines: $(grep -c '\[Memory\]' "$F")  PSO created: $(grep -c 'Graphics PSO created' "$F" || true)"
echo
echo "== 청크 ID (첫 번째가 메인 스레드) =="
grep -oE "\[ID: [0-9]+\] Memory Chunk Initi" "$F" | grep -oE "[0-9]+" || true
echo

awk -v step="$STEP" '
function parse(line, kind,    a, s, id) {
	match(line, /\[ID: [0-9]+\]/); id = substr(line, RSTART + 5, RLENGTH - 6)
	if (kind == "A") { match(line, /Allocated [0-9]+ byte/);   s = substr(line, RSTART + 10, RLENGTH - 15) + 0 }
	else             { match(line, /Deallocated [0-9]+ byte/); s = substr(line, RSTART + 12, RLENGTH - 17) + 0 }
	match(line, /address : [0-9a-f]+/); a = substr(line, RSTART + 10, RLENGTH - 10)
	CUR_ID = id; CUR_S = s; CUR_A = a
}
/\] Allocated [0-9]+ byte/ {
	parse($0, "A")
	k = CUR_ID SUBSEP CUR_S
	live[k]++; total[k]++; if (live[k] > peak[k]) peak[k] = live[k]
	liveAll[CUR_ID]++; if (liveAll[CUR_ID] > peakAll[CUR_ID]) peakAll[CUR_ID] = liveAll[CUR_ID]
	if (!(CUR_ID in first)) first[CUR_ID] = 1
	if (CUR_S >= 8192) big[++nb] = NR " Allocated " CUR_S " [" CUR_ID "]"
}
/\] Deallocated [0-9]+ byte/ {
	parse($0, "D")
	k = CUR_ID SUBSEP CUR_S
	live[k]--; liveAll[CUR_ID]--
	if (CUR_S >= 8192) big[++nb] = NR " Deallocated " CUR_S " [" CUR_ID "]"
}
NR % step == 0 {
	samples[++ns] = NR
	for (k in live) snap[ns, k] = live[k]
}
END {
	print "== 청크별 크기 클래스: 종료 시 live / 피크 live / 누적 할당 =="
	for (id in first) {
		printf "chunk %s : peak total live %d\n", id, peakAll[id]
		for (s = 4; s <= 2097152; s *= 2) {
			k = id SUBSEP s
			if (k in total) printf "  %8d B : live %6d  peak %6d  allocs %8d\n", s, live[k], peak[k], total[k]
		}
	}
	print ""
	print "== 프레임 누수 추적: " step " 줄 간격 live 추이 (종료 시 live > 0 이거나 변화가 있는 클래스만) =="
	for (id in first) {
		for (s = 4; s <= 2097152; s *= 2) {
			k = id SUBSEP s
			if (!(k in total)) continue
			changed = 0
			for (i = 2; i <= ns; i++) if (snap[i, k] != snap[i - 1, k]) changed = 1
			if (!changed && live[k] == 0) continue
			printf "chunk %s %7d B :", id, s
			for (i = 1; i <= ns; i++) printf " %d", snap[i, k]
			printf "  | end %d\n", live[k]
		}
	}
	print ""
	print "== 8KB 이상 블록 이벤트 (해시 버킷 배열 성장 등) =="
	for (i = 1; i <= nb && i <= 60; i++) print "  " big[i]
	if (nb > 60) print "  ... (" nb " total)"
}' "$F"
