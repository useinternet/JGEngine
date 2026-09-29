# data/cards.json 으로 사람이 읽는 카드 목록 문서 두 개를 만든다.
#   04_플레이어_카드.md, 05_조우_카드.md
# 사용: perl gen_lists.pl <아컴호러 폴더 경로>
use strict; use warnings; use utf8; use JSON::PP; use Encode qw(decode);

# 경로 인자는 UTF-8 바이트로 들어온다. 한글 파일 이름 리터럴과 이어 붙이려면 문자열로 디코드해야 한다.
my $root = decode('UTF-8', shift(@ARGV) // die "usage: perl gen_lists.pl <root>\n");
binmode STDOUT, ':encoding(UTF-8)';

sub load_json
{
    my ($path) = @_;
    open my $fh, '<:raw', $path or die "open $path: $!";
    local $/;
    my $data = decode_json(<$fh>);
    close $fh;
    return $data;
}

my $data  = load_json("$root/data/cards.json");
my @cards = @{ $data->{cards} };
my %by_code = map { $_->{code} => $_ } @cards;

# 시나리오 id(torch/arkham/tentacles) → 한국어 이름. 세트 코드와 id 가 겹쳐 헷갈리므로 이름으로 보여 준다.
my %SCENARIO_NAME;
if (-e "$root/data/scenarios.json")
{
    my $sc = load_json("$root/data/scenarios.json");
    $SCENARIO_NAME{ $_->{id} } = $_->{name_ko} for @{ $sc->{scenarios} || [] };
}

# 영문 특성(소문자) → 한국어 특성. 같은 카드의 traits.en / traits.ko 는 순서가 같다.
my %TRAIT_KO;
for my $c (@cards)
{
    my ($en, $ko) = ($c->{traits}{en}, $c->{traits}{ko});
    next unless @$en && @$en == @$ko;
    $TRAIT_KO{ lc $en->[$_] } //= $ko->[$_] for 0 .. $#$en;
}

# 텍스트 태그 → 표시 문자열. 한국어 공식 명칭은 02_용어_키워드.md 를 따른다.
# 기호 혼돈 토큰 4종은 한국어판 참조 안내서에 이름 없이 아이콘만 있어 영문 이름을 쓴다.
# 자유 격발 아이콘은 영문 데이터가 [fast], 한국어 데이터가 [free] 로 적는다.
my %TAG = (
    action => '[행동]', reaction => '[반응]', fast => '[자유]', free => '[자유]',
    willpower => '[의지]', intellect => '[지식]', combat => '[힘]', agility => '[민첩]', wild => '[만능]',
    skull => '[Skull]', cultist => '[Cultist]', tablet => '[Tablet]', elder_thing => '[Elder Thing]',
    auto_fail => '[자동 실패]', elder_sign => '[고대 표식]', per_investigator => '[조사자당]',
    guardian => '[수호자]', seeker => '[탐구자]', rogue => '[무법자]', mystic => '[신비주의자]', survivor => '[생존자]',
);
my %ICON_ORDER = (willpower => 0, intellect => 1, combat => 2, agility => 3, wild => 4);
my @FACTIONS = qw(guardian seeker rogue mystic survivor neutral);

sub md_text
{
    my ($s) = @_;
    return '' unless defined $s && $s ne '';
    $s =~ s{<b>(.*?)</b>}{**$1**}g;
    $s =~ s{<i>(.*?)</i>}{*$1*}g;
    $s =~ s{<[^>]+>}{}g;
    $s =~ s{\[\[([^\]]+)\]\]}{*$1*}g;
    $s =~ s{\[([a-z_]+)\]}{ $TAG{$1} // "[$1]" }ge;
    $s =~ s/\|/\\|/g;
    $s =~ s/\r?\n/<br>/g;
    return $s;
}

# 한국어가 없으면 영어 원문에 (EN) 표시를 붙인다.
sub loc_text
{
    my ($pair) = @_;
    return '' unless $pair;
    return md_text($pair->{ko}) if defined $pair->{ko} && $pair->{ko} ne '';
    return '(EN) ' . md_text($pair->{en}) if defined $pair->{en} && $pair->{en} ne '';
    return '';
}

sub name_cell
{
    my ($c) = @_;
    my $n = $c->{name}{ko} // $c->{name}{en};
    $n = "★ $n" if $c->{unique};
    my $s = "$n<br><sub>$c->{name}{en}</sub>";
    $s .= "<br><sub>" . ($c->{subname}{ko} // $c->{subname}{en}) . "</sub>" if $c->{subname};
    return $s =~ s/\|/\\|/gr;
}

sub icons_cell
{
    my ($c) = @_;
    my $i = $c->{skill_icons} or return '-';
    my $s = join('', map { $TAG{$_} x $i->{$_} } sort { $ICON_ORDER{$a} <=> $ICON_ORDER{$b} } grep { $i->{$_} } keys %$i);
    return $s eq '' ? '-' : $s;
}

sub traits_cell
{
    my ($c) = @_;
    my $t = $c->{traits}{ko};
    $t = $c->{traits}{en} unless $t && @$t;
    return @$t ? join(', ', @$t) : '-';
}

sub qty_cell
{
    my ($c) = @_;
    my $q = $c->{quantity};
    return $q->{core} == $q->{revised_core} ? "$q->{core}" : "$q->{core} / $q->{revised_core}";
}

sub val
{
    my ($v) = @_;
    return defined $v ? $v : '-';
}

sub stats_cell
{
    my ($c) = @_;
    if (my $e = $c->{enemy})
    {
        my $hp = val($e->{health}) . ($e->{health_per_investigator} ? '[조사자당]' : '');
        return "전투값 " . val($e->{fight}) . " / 체력 $hp / 회피값 " . val($e->{evade}) . "<br>피해 $e->{damage} · 공포 $e->{horror}";
    }
    if (my $l = $c->{location})
    {
        my $s = "장막값 " . val($l->{shroud}) . " / 단서값 " . val($l->{clues}) . ($l->{clues_per_investigator} && $l->{clues} ? '[조사자당]' : '');
        for my $side (qw(front back))
        {
            next unless $l->{$side};
            my @conn = @{ $l->{$side}{connections} || [] };
            push @conn, map { "모든 *" . ($TRAIT_KO{$_} // $_) . "* 장소" } @{ $l->{$side}{connections_trait} || [] };
            my $conn = @conn ? join(', ', @conn) : '없음';
            my $label = $side eq 'front' ? '앞' : '뒤';
            $label .= '(공개)' if ($l->{revealed_side} // '') eq $side;
            $s .= "<br>$label: 기호 " . val($l->{$side}{symbol}) . " → 연결 $conn";
        }
        return $s;
    }
    if (my $a = $c->{act})
    {
        return "단계 " . val($a->{stage}) . " / 단서 한계값 " . ($a->{clues} ? $a->{clues} . ($a->{clues_per_investigator} ? '[조사자당]' : '') : '-');
    }
    if (my $g = $c->{agenda})
    {
        return "단계 " . val($g->{stage}) . " / 파멸 한계값 " . val($g->{doom});
    }
    if (defined $c->{health} || defined $c->{sanity})
    {
        return "체력 " . val($c->{health}) . " / 정신력 " . val($c->{sanity});
    }
    return '-';
}

sub text_cell
{
    my ($c) = @_;
    my $s = loc_text($c->{text});
    if (my $b = $c->{back})
    {
        my $bn = $b->{name} ? ($b->{name}{ko} // $b->{name}{en}) : '';
        my $bt = loc_text($b->{text});
        $s .= ($s ne '' ? '<br>' : '') . "— 뒷면" . ($bn ? " «$bn»" : '') . ($bt ne '' ? ": $bt" : '') if $bn ne '' || $bt ne '';
    }
    return $s eq '' ? '-' : $s;
}

sub write_file
{
    my ($path, $lines) = @_;
    open my $fh, '>:encoding(UTF-8)', $path or die "write $path: $!";
    print $fh join("\n", @$lines), "\n";
    close $fh;
    printf "%s : %d lines\n", $path, scalar(@$lines);
}

my @HEADER_NOTE = (
    "> 자동 생성 문서입니다. `tools/gen_lists.pl` 이 `data/cards.json` 을 읽어 만듭니다. 손으로 고치지 말고 데이터나 스크립트를 고친 뒤 다시 생성하세요.",
    "> 이미지는 `images/<코드>.png`(앞면), `images/<코드>b.png`(뒷면)입니다. 아이콘 표기는 `02_용어_키워드.md` 참고.",
    "> 매수 칸의 `a / b` 는 기본판(2016) / 개정판(2020) 매수입니다. 값이 하나면 두 판이 같습니다. ★ 는 고유(✷, Unique).",
    "> 기호 혼돈 토큰 4종(Skull·Cultist·Tablet·Elder Thing)은 한국어판에 이름이 없어(아이콘만 인쇄) 영문 이름으로 적었습니다.",
);

# ---------------------------------------------------------------- 플레이어 카드
{
    my @player = grep { $_->{deck} eq 'player' } @cards;
    my @inv    = grep { $_->{type} eq 'investigator' } @player;
    my %sig    = map { my $c = $_; map { ($_ => 1) } @{ $c->{investigator}{required_cards} } } @inv;
    my @basic  = grep { ($_->{subtype} // '') eq 'basicweakness' } @player;
    my @normal = grep { $_->{type} ne 'investigator' && !$sig{ $_->{code} } && !$_->{subtype} } @player;

    my @out = ("# 플레이어 카드 목록 — 기본판(Core Set, 2016)", "", @HEADER_NOTE, "");

    my ($q_core, $q_rev) = (0, 0);
    $q_core += $_->{quantity}{core} for @player;
    $q_rev  += $_->{quantity}{revised_core} for @player;
    push @out, "## 요약", "",
        sprintf("- 플레이어 카드 %d종, 기본판 %d장 / 개정판 %d장. 조사자 %d명, 전용 카드 %d종, 기본 약점 %d종, 역할군·중립 카드 %d종.",
            scalar(@player), $q_core, $q_rev, scalar(@inv), scalar(keys %sig), scalar(@basic), scalar(@normal)),
        "- 덱에는 같은 이름의 카드를 2장까지 넣을 수 있습니다. 기본판은 역할군 카드가 대부분 1장씩이라 2장을 채우려면 기본판이 2세트 필요합니다. 개정판은 대부분 2장씩 들어 있습니다(카드 내용은 같음).",
        "",
        "| 역할군 | 종 | 매수(기본/개정) | 자산 | 이벤트 | 능력 | 레벨 0 | 레벨 1+ |",
        "|---|---|---|---|---|---|---|---|";
    for my $f (@FACTIONS)
    {
        my @l = grep { $_->{faction} eq $f } @normal;
        next unless @l;
        my ($qc, $qr) = (0, 0);
        $qc += $_->{quantity}{core} for @l;
        $qr += $_->{quantity}{revised_core} for @l;
        push @out, sprintf("| %s (%s) | %d | %d / %d | %d | %d | %d | %d | %d |",
            $l[0]{faction_name}{ko}, $l[0]{faction_name}{en}, scalar(@l), $qc, $qr,
            scalar(grep { $_->{type} eq 'asset' } @l), scalar(grep { $_->{type} eq 'event' } @l),
            scalar(grep { $_->{type} eq 'skill' } @l),
            scalar(grep { ($_->{level} // 0) == 0 } @l), scalar(grep { ($_->{level} // 0) > 0 } @l));
    }
    push @out, "";

    push @out, "## 조사자", "",
        "| 코드 | 이름 | 역할군 | 의지 | 지식 | 힘 | 민첩 | 체력 | 정신력 | 덱 | 전용 카드 |",
        "|---|---|---|---|---|---|---|---|---|---|---|";
    for my $c (@inv)
    {
        my $i = $c->{investigator};
        my $sigs = join('<br>', map { my $s = $by_code{$_}; $s ? "$_ " . ($s->{name}{ko} // $s->{name}{en}) . ($s->{subtype} ? ' (약점)' : '') : $_ } @{ $i->{required_cards} });
        push @out, sprintf("| %s | %s | %s | %d | %d | %d | %d | %d | %d | %d장 | %s |",
            $c->{code}, name_cell($c), $c->{faction_name}{ko}, @{ $i->{skills} }{qw(willpower intellect combat agility)},
            $i->{health}, $i->{sanity}, $i->{deck_size}, $sigs);
    }
    push @out, "";
    for my $c (@inv)
    {
        push @out, "### $c->{code} " . ($c->{name}{ko}) . " ($c->{name}{en})" . ($c->{subname} ? " — $c->{subname}{ko}" : ''), "",
            "- 기능: " . loc_text($c->{text}),
            "- 덱 구성(뒷면): " . loc_text($c->{back}{text}),
            "- 이미지: [앞면]($c->{image}{front}) · [뒷면]($c->{image}{back})",
            "";
    }

    my @table_head = (
        "| 코드 | 이름 | 타입 | Lv | 비용 | 슬롯 | 아이콘 | 특성 | 매수 | 텍스트 |",
        "|---|---|---|---|---|---|---|---|---|---|",
    );
    my $row = sub
    {
        my ($c) = @_;
        my $slot = $c->{slot_name} ? $c->{slot_name}{ko} . (@{ $c->{slots} } > 1 ? ' ×' . scalar(@{ $c->{slots} }) : '') : '-';
        my $stats = stats_cell($c);
        my $text = text_cell($c);
        $text = "($stats)<br>$text" if $stats ne '-';
        return sprintf("| %s | %s | %s | %s | %s | %s | %s | %s | %s | %s |",
            $c->{code}, name_cell($c), $c->{type_name}{ko}, val($c->{level}), val($c->{cost}), $slot,
            icons_cell($c), traits_cell($c), qty_cell($c), $text);
    };

    push @out, "## 전용 카드", "", "조사자 덱에 반드시 들어가는 카드입니다(덱 크기에 포함되지 않음). 약점 표시가 붙은 것은 전용 약점입니다.", "", @table_head;
    push @out, $row->($by_code{$_}) for map { @{ $_->{investigator}{required_cards} } } @inv;
    push @out, "";

    for my $f (@FACTIONS)
    {
        my @l = grep { $_->{faction} eq $f } @normal;
        next unless @l;
        push @out, "## $l[0]{faction_name}{ko} ($l[0]{faction_name}{en})", "", @table_head;
        push @out, $row->($_) for @l;
        push @out, "";
    }

    push @out, "## 기본 약점 (Basic Weakness)", "", "게임 준비 때 조사자마다 무작위로 1장씩 덱에 섞는 카드입니다.", "", @table_head;
    push @out, $row->($_) for @basic;
    push @out, "";

    write_file("$root/04_플레이어_카드.md", \@out);
}

# ---------------------------------------------------------------- 조우 카드
{
    my @enc = grep { $_->{deck} eq 'encounter' } @cards;
    my (@sets, %set_cards);
    for my $c (@enc)
    {
        my $s = $c->{encounter_set}{code};
        push @sets, $s unless $set_cards{$s};
        push @{ $set_cards{$s} }, $c;
    }

    my @out = ("# 조우 카드 목록 — 기본판(Core Set, 2016)", "", @HEADER_NOTE,
        "> ArkhamDB 한국어 데이터에는 조우 카드 본문 번역이 대부분 없습니다. 그런 칸은 `(EN)` 을 붙여 영어 원문을 보여 줍니다. 카드 이름과 특성은 한국어가 있습니다.",
        "> 주요목적·주요사건 뒷면(진행 효과)과 장소 뒷면이 함께 적혀 있어 스포일러를 포함합니다.", "");

    my ($total, $types) = (0, {});
    push @out, "## 요약", "",
        "| 조우 세트 | 코드 | 사용 시나리오 | 종 | 매수 | 구성 |",
        "|---|---|---|---|---|---|";
    for my $s (@sets)
    {
        my @l = @{ $set_cards{$s} };
        my $q = 0;
        $q += $_->{quantity}{core} for @l;
        $total += $q;
        my %t;
        $t{ $_->{type_name}{ko} } += $_->{quantity}{core} for @l;
        my $sc = join(', ', map { $SCENARIO_NAME{$_} // $_ } @{ $l[0]{encounter_set}{scenarios} || [] });
        push @out, sprintf("| %s (%s) | `%s` | %s | %d | %d | %s |", $l[0]{encounter_set}{name}{ko}, $l[0]{encounter_set}{name}{en}, $s,
            $sc eq '' ? '-' : $sc, scalar(@l), $q, join(', ', map { "$_ $t{$_}" } sort keys %t));
    }
    push @out, "", sprintf("조우 카드 %d종, 총 %d장.", scalar(@enc), $total), "";

    for my $s (@sets)
    {
        my @l = @{ $set_cards{$s} };
        push @out, "## $l[0]{encounter_set}{name}{ko} ($l[0]{encounter_set}{name}{en}) — `$s`", "",
            "| 코드 | 이름 | 타입 | 매수 | 수치 | 특성 | 승점 | 텍스트 |",
            "|---|---|---|---|---|---|---|---|";
        for my $c (@l)
        {
            push @out, sprintf("| %s | %s | %s | %d | %s | %s | %s | %s |",
                $c->{code}, name_cell($c), $c->{type_name}{ko}, $c->{quantity}{core}, stats_cell($c), traits_cell($c),
                val($c->{victory}), text_cell($c));
        }
        push @out, "";
    }

    write_file("$root/05_조우_카드.md", \@out);
}
