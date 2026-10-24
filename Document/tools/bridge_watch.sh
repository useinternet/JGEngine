#!/bin/bash
# 연결 감시(엔진 총괄용) — 대시보드의 "총괄 연결됨"(Saved/Feedback/bridge.heartbeat 가 20초 안에 고쳐졌는지)을 살려 두고,
# 새 피드백 · 질문 답 · 배포 요청 · 배포 끝 · 세션 결과 파일을 한 줄씩 알린다. 엔진 총괄이 Monitor 로 띄운다(30분마다 다시):
#   Monitor command: bash Document/tools/bridge_watch.sh   (작업 폴더 C:\Develop\JGEngine)
# 켤 때 이미 있는 파일은 알리지 않으므로, 켠 뒤 inbox 중 sent/ 에 없는 것과 Decisions/answers 를 한 번 훑는다.
# 파일마다 stat 하지 않고 find 한 번으로 찍은 목록을 comm 으로 견준다(한 바퀴 2초 남짓 — 대시보드 20초 안).
D=/c/Develop/JGEngine/Saved
snap() { find "$D/Feedback/inbox" "$D/Feedback/results" "$D/Decisions/answers" "$D/Deploy" -maxdepth 1 -type f \( -name '*.json' -o -name '*.txt' \) -printf '%p@%T@\n' 2>/dev/null | sort; }
states() { for f in "$D"/Deploy/2*.json; do perl -MJSON::PP -e 'local $/; open F,"<:raw",$ARGV[0] or exit; my $j=eval{JSON::PP->new->utf8->decode(<F>)} or exit; print "$ARGV[0] ", ($j->{state}//""), "\n"' "$f"; done | grep -E ' (done|failed)$' | sort; }
prev=$(snap)
st=$(states)
while true; do
  touch "$D/Feedback/bridge.heartbeat"
  cur=$(snap)
  if [ "$cur" != "$prev" ]; then
    comm -13 <(printf '%s\n' "$prev") <(printf '%s\n' "$cur") | while IFS= read -r line; do
      p=${line%@*}; f=${p##*/}
      case "$p" in
        */inbox/*) printf '%s\n' "$prev" | grep -qF "$p@" || echo "new feedback: $f";;
        */results/*) echo "result: $f";;
        */answers/*) echo "answer: $f";;
        */Deploy/*) [ "$f" = current.json ] || printf '%s\n' "$prev" | grep -qF "$p@" || echo "deploy request: $f";;
      esac
    done
    prev=$cur
    ns=$(states)
    if [ "$ns" != "$st" ]; then
      comm -13 <(printf '%s\n' "$st") <(printf '%s\n' "$ns") | sed 's/^/deploy finished: /'
      st=$ns
    fi
  fi
  sleep 3
done
