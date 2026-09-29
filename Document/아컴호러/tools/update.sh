#!/usr/bin/env bash
# ArkhamDB 에서 카드 데이터를 다시 받아 data/ 와 카드 목록 문서를 재생성한다.
# 사용: bash tools/update.sh [--images]
#   --images  : images/ 에 없는 카드 이미지만 추가로 받는다(기존 파일은 건드리지 않음).
# 필요: curl, perl(JSON::PP). python 은 쓰지 않는다.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

echo "[1/4] ArkhamDB 카드 전체 덤프 받기 (en, ko)"
curl -sf -m 180 -o "$TMP/cards_en.json" "https://arkhamdb.com/api/public/cards/?encounter=1"
curl -sf -m 180 -o "$TMP/cards_ko.json" "https://ko.arkhamdb.com/api/public/cards/?encounter=1"
curl -sf -m 60  -o "$ROOT/data/raw/arkhamdb_packs.json" "https://arkhamdb.com/api/public/packs/"

echo "[2/4] 코어 셋만 뽑아 data/raw 에 저장"
perl "$ROOT/tools/extract_raw.pl" "$TMP/cards_en.json" "$TMP/cards_ko.json" "$ROOT/data/raw"

echo "[3/4] data/cards.json 생성 (data/scenarios.json 의 장소 연결 병합)"
perl "$ROOT/tools/build_cards.pl" "$ROOT"

echo "[4/4] 04_플레이어_카드.md, 05_조우_카드.md 생성"
perl "$ROOT/tools/gen_lists.pl" "$ROOT"

if [ "${1:-}" = "--images" ]; then
    echo "[+] 빠진 이미지 받기"
    perl -MJSON::PP -e '
        local $/; open my $h, "<:raw", $ARGV[0] or die; my $d = decode_json(<$h>);
        for my $c (@{ $d->{cards} }) { for my $s (qw(front back)) { my $p = $c->{image}{$s} or next; print "$p\n" } }
    ' "$ROOT/data/cards.json" | while read -r rel; do
        out="$ROOT/$rel"
        [ -s "$out" ] && continue
        curl -sf -m 60 -o "$out" "https://arkhamdb.com/bundles/cards/${rel#images/}" && echo "  $rel"
    done
fi
echo "완료"
