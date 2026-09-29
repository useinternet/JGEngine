# data/raw 의 ArkhamDB 코어 셋 덤프(한/영)를 엔진이 읽기 쉬운 형태로 합쳐 data/cards.json 을 만든다.
# - 한국어(ko)와 영어(en)를 한 카드에 병기한다.
# - 개정판 코어(rcore)의 매수를 quantity.revised_core 로 붙인다.
# - data/scenarios.json 이 있으면 장소 심볼/연결을 location 에 합친다.
# 사용: perl build_cards.pl <아컴호러 폴더 경로>
use strict; use warnings; use utf8; use JSON::PP;

my $root = shift @ARGV or die "usage: perl build_cards.pl <root>\n";
my $raw  = "$root/data/raw";

# 실제 카드가 아닌 항목: 01000 = 무작위 기본 약점 자리표시자, 98004 = 로랜드 뱅크스 재판(01001 과 동일)
my %EXCLUDE = ('01000' => 1, '98004' => 1);

sub load_json
{
    my ($path) = @_;
    open my $fh, '<:raw', $path or die "open $path: $!";
    local $/;
    my $data = decode_json(<$fh>);
    close $fh;
    return $data;
}

my $ko_list  = load_json("$raw/arkhamdb_core_ko.json");
my $en_list  = load_json("$raw/arkhamdb_core_en.json");
my $rcore    = load_json("$raw/arkhamdb_rcore_en.json");
my %en       = map { $_->{code} => $_ } @$en_list;

my %revised_qty;
for my $r (@$rcore)
{
    my $orig = $r->{duplicate_of_code} or next;
    $revised_qty{$orig} += $r->{quantity};
}

my $locations = {};
my %scenario_of_set;
if (-e "$root/data/scenarios.json")
{
    my $sc = load_json("$root/data/scenarios.json");
    $locations = $sc->{locations} || {};
    # 조우 덱 세트(encounter_sets) 외에 무작위로 고르는 세트(random_encounter_sets, 3편 권속 4종)와
    # 따로 쓰는 세트(aside_sets, 2편 추종자 덱)도 그 시나리오에서 쓰는 세트로 본다.
    my %known_set = map { ($_->{encounter_code} // '') => 1 } @$ko_list;
    my $collect;
    $collect = sub
    {
        my ($node, $out) = @_;
        if (ref $node eq 'ARRAY') { $collect->($_, $out) for @$node; }
        elsif (ref $node eq 'HASH') { $collect->($_, $out) for values %$node; }
        elsif (defined $node && $known_set{$node}) { $out->{$node} = 1; }
    };
    for my $s (@{ $sc->{scenarios} || [] })
    {
        my %sets;
        $collect->($s->{$_}, \%sets) for qw(encounter_sets random_encounter_sets aside_sets);
        push @{ $scenario_of_set{$_} }, $s->{id} for sort keys %sets;
    }
}

my %SLOT = (
    'Hand'      => ['hand'],
    'Hand x2'   => ['hand', 'hand'],
    'Arcane'    => ['arcane'],
    'Arcane x2' => ['arcane', 'arcane'],
    'Accessory' => ['accessory'],
    'Ally'      => ['ally'],
    'Body'      => ['body'],
    'Tarot'     => ['tarot'],
);

# ArkhamDB 한국어 데이터는 번역이 없는 필드에 영어 원문을 그대로 넣어 둔다(조우 카드 본문 대부분).
# 한글이 한 글자도 없고 영어와 같으면 번역 없음으로 보고 ko 를 null 로 둔다. 사용하는 쪽은 en 으로 대체한다.
sub pair
{
    my ($ko, $en) = @_;
    return undef if !defined $ko && !defined $en;
    $ko = undef if defined $ko && defined $en && $ko eq $en && $ko !~ /\p{Hangul}/;
    return { ko => $ko, en => $en };
}

sub split_traits
{
    my ($s) = @_;
    return [] unless defined $s && $s ne '';
    return [ grep { $_ ne '' } map { s/^\s+|\s+$//gr } split /\./, $s ];
}

sub num_or_null
{
    my ($v) = @_;
    return defined $v ? $v + 0 : undef;
}

# 영어 텍스트에서 키워드와 능력 아이콘을 뽑는다(원문 텍스트는 그대로 따로 보관).
sub parse_text
{
    my ($text) = @_;
    my (%kw, %icons, %info);
    return ([], {}, {}) unless defined $text && $text ne '';

    for my $line (split /\n/, $text)
    {
        # 키워드는 줄 맨 앞에 "Hunter. Massive." 나 "Fast. Play only during your turn." 처럼 온다.
        my $plain = $line =~ s/<[^>]+>//gr;
        $plain =~ s/^\s+|\s+$//g;
        while ($plain =~ s/^(Fast|Hunter|Retaliate|Aloof|Surge|Peril|Massive|Alert)\.\s*//)
        {
            $kw{lc $1} = 1;
        }
    }
    if ($text =~ /Uses \((\d+|X) ([a-z]+)\)/)
    {
        $kw{uses} = 1;
        $info{uses} = { count => ($1 eq 'X' ? 'X' : $1 + 0), type => $2 };
    }
    $kw{revelation} = 1 if $text =~ /<b>Revelation<\/b>/;
    $kw{forced}     = 1 if $text =~ /<b>Forced<\/b>/;
    $kw{objective}  = 1 if $text =~ /<b>Objective<\/b>/;
    if ($text =~ /<b>Spawn<\/b>\s*-\s*([^\n]+)/)
    {
        $kw{spawn} = 1;
        $info{spawn} = $1 =~ s/<[^>]+>//gr;
    }
    if ($text =~ /<b>Prey<\/b>\s*-\s*([^\n]+)/)
    {
        $kw{prey} = 1;
        $info{prey} = $1 =~ s/<[^>]+>//gr;
    }
    while ($text =~ /<b>(Fight|Evade|Investigate|Parley|Resign)\.<\/b>/g)
    {
        $kw{lc $1} = 1;
    }
    $icons{action}   = () = $text =~ /\[action\]/g;
    $icons{reaction} = () = $text =~ /\[reaction\]/g;
    $icons{free}     = () = $text =~ /\[fast\]/g;
    delete $icons{$_} for grep { !$icons{$_} } keys %icons;

    return ([ sort keys %kw ], \%icons, \%info);
}

my @cards;
for my $k (sort { $a->{position} <=> $b->{position} || $a->{code} cmp $b->{code} } @$ko_list)
{
    my $code = $k->{code};
    next if $EXCLUDE{$code};
    my $e = $en{$code} or die "no english card for $code";
    my $type = $k->{type_code};
    my $is_encounter = defined $k->{encounter_code} ? 1 : 0;

    my %c = (
        code        => $code,
        position    => $k->{position} + 0,
        name        => pair($k->{name}, $k->{real_name}),
        type        => $type,
        type_name   => pair($k->{type_name}, $e->{type_name}),
        faction     => $k->{faction_code},
        faction_name => pair($k->{faction_name}, $e->{faction_name}),
        deck        => $is_encounter ? 'encounter' : 'player',
        quantity    => {
            core         => $k->{quantity} + 0,
            revised_core => $is_encounter ? $k->{quantity} + 0 : ($revised_qty{$code} // 0),
        },
        unique      => $k->{is_unique} ? JSON::PP::true : JSON::PP::false,
        double_sided => $k->{double_sided} ? JSON::PP::true : JSON::PP::false,
        traits      => { ko => split_traits($k->{traits}), en => split_traits($e->{real_traits} // $k->{real_traits}) },
        text        => pair($k->{text}, $e->{text}),
        flavor      => pair($k->{flavor}, $e->{flavor}),
        illustrator => $k->{illustrator},
        image       => {
            front => $k->{imagesrc}     ? 'images/' . ($k->{imagesrc} =~ s{.*/}{}r)     : undef,
            back  => $k->{backimagesrc} ? 'images/' . ($k->{backimagesrc} =~ s{.*/}{}r) : undef,
        },
        arkhamdb_url => "https://arkhamdb.com/card/$code",
    );

    $c{subname} = pair($k->{subname}, $e->{subname}) if defined $k->{subname} || defined $e->{subname};
    $c{subtype} = $k->{subtype_code} if $k->{subtype_code};
    $c{subtype_name} = pair($k->{subtype_name}, $e->{subtype_name}) if $k->{subtype_code};
    $c{victory} = $k->{victory} + 0 if defined $k->{victory};
    $c{linked_to} = $k->{linked_to_code} if $k->{linked_to_code};
    $c{hidden} = JSON::PP::true if $k->{hidden};
    $c{errata_date} = substr($k->{errata_date}{date}, 0, 10) if ref $k->{errata_date};
    $c{tags} = [ grep { $_ ne '' } split /\./, $k->{tags} ] if $k->{tags};
    if (defined $k->{back_name} || defined $k->{back_text} || defined $k->{back_flavor})
    {
        $c{back} = {
            name   => pair($k->{back_name}, $e->{back_name}),
            text   => pair($k->{back_text}, $e->{back_text}),
            flavor => pair($k->{back_flavor}, $e->{back_flavor}),
        };
    }

    if ($is_encounter)
    {
        $c{encounter_set} = {
            code      => $k->{encounter_code},
            name      => pair($k->{encounter_name}, $e->{encounter_name}),
            position  => num_or_null($k->{encounter_position}),
            scenarios => $scenario_of_set{ $k->{encounter_code} } || [],
        };
    }
    else
    {
        $c{deck_limit} = num_or_null($k->{deck_limit});
        $c{level} = num_or_null($k->{xp}) if $type ne 'investigator';
        $c{restrictions} = { investigator => [ sort keys %{ $k->{restrictions}{investigator} } ] } if ref $k->{restrictions};
    }

    if ($type eq 'asset' || $type eq 'event')
    {
        $c{cost} = num_or_null($k->{cost});
    }
    if ($type eq 'asset' && $e->{real_slot})
    {
        $c{slots} = $SLOT{ $e->{real_slot} } or die "unknown slot '$e->{real_slot}' on $code";
        $c{slot_name} = pair($k->{slot}, $e->{real_slot});
    }
    if ($type eq 'asset' && (defined $k->{health} || defined $k->{sanity}))
    {
        $c{health} = num_or_null($k->{health});
        $c{sanity} = num_or_null($k->{sanity});
    }
    if ($type =~ /^(asset|event|skill)$/ || ($type eq 'treachery' && !$is_encounter) || ($type eq 'enemy' && !$is_encounter))
    {
        my %icons = map { $_ => ($k->{"skill_$_"} // 0) + 0 } qw(willpower intellect combat agility wild);
        $c{skill_icons} = \%icons if grep { $_ } values %icons;
    }
    if ($type eq 'investigator')
    {
        my $req = $k->{deck_requirements} || {};
        $c{investigator} = {
            skills => { map { $_ => $k->{"skill_$_"} + 0 } qw(willpower intellect combat agility) },
            health => $k->{health} + 0,
            sanity => $k->{sanity} + 0,
            deck_size => $req->{size} + 0,
            required_cards => [ sort keys %{ $req->{card} || {} } ],
            random_requirements => $req->{random} || [],
            deck_options => $k->{deck_options} || [],
        };
    }
    if ($type eq 'enemy')
    {
        $c{enemy} = {
            fight  => num_or_null($k->{enemy_fight}),
            health => num_or_null($k->{health}),
            health_per_investigator => $k->{health_per_investigator} ? JSON::PP::true : JSON::PP::false,
            evade  => num_or_null($k->{enemy_evade}),
            damage => num_or_null($k->{enemy_damage}) // 0,
            horror => num_or_null($k->{enemy_horror}) // 0,
        };
    }
    if ($type eq 'location')
    {
        my $loc = $locations->{$code} || {};
        $c{location} = {
            shroud => num_or_null($k->{shroud}),
            clues  => num_or_null($k->{clues}),
            clues_per_investigator => ($k->{clues} && !$k->{clues_fixed}) ? JSON::PP::true : JSON::PP::false,
        };
        for my $side (qw(front back))
        {
            next unless $loc->{$side};
            $c{location}{$side} = { symbol => $loc->{$side}{symbol}, connections => $loc->{$side}{connections} || [] };
            # 기호가 아니라 특성으로 이어지는 연결(큰길: "다른 모든 숲 장소와 연결")
            $c{location}{$side}{connections_trait} = $loc->{$side}{connections_trait} if $loc->{$side}{connections_trait};
        }
        $c{location}{revealed_side} = $loc->{revealed_side} if $loc->{revealed_side};
    }
    if ($type eq 'act')
    {
        $c{act} = {
            stage => num_or_null($k->{stage}),
            clues => num_or_null($k->{clues}),
            clues_per_investigator => ($k->{clues} && !$k->{clues_fixed}) ? JSON::PP::true : JSON::PP::false,
        };
    }
    if ($type eq 'agenda')
    {
        $c{agenda} = { stage => num_or_null($k->{stage}), doom => num_or_null($k->{doom}) };
    }

    # 앞뒷면을 함께 본다(주요목적·주요사건 뒷면에도 폭로·목표 지시가 있다).
    my ($kw, $icons, $info) = parse_text(join("\n", grep { defined && $_ ne '' } $e->{text}, $e->{back_text}));
    push @$kw, 'victory' if defined $k->{victory};
    push @$kw, 'unique'  if $k->{is_unique};
    $c{keywords} = [ sort @$kw ];
    $c{ability_icons} = $icons if %$icons;
    $c{uses} = $info->{uses} if $info->{uses};
    $c{spawn} = $info->{spawn} if $info->{spawn};
    $c{prey}  = $info->{prey}  if $info->{prey};

    push @cards, \%c;
}

my @ORDER = qw(schema_version generated source pack card_count cards
    code position name subname released type type_name subtype subtype_name faction faction_name deck
    encounter_set quantity deck_limit unique level cost slots slot_name traits skill_icons health sanity
    investigator enemy location act agenda victory keywords ability_icons uses spawn prey restrictions
    text flavor double_sided back linked_to hidden errata_date tags illustrator image arkhamdb_url
    ko en front scenarios symbol connections connections_trait revealed_side shroud clues clues_per_investigator stage doom
    fight evade damage horror health_per_investigator skills deck_size required_cards random_requirements deck_options
    willpower intellect combat agility wild count);
my %RANK; @RANK{@ORDER} = (0 .. $#ORDER);

my $pack = (grep { $_->{code} eq 'core' } @{ load_json("$raw/arkhamdb_packs.json") })[0];
my $out = {
    schema_version => 1,
    generated => sprintf('%04d-%02d-%02d', (localtime)[5] + 1900, (localtime)[4] + 1, (localtime)[3]),
    source => 'ArkhamDB public API (https://arkhamdb.com/api/public/cards/?encounter=1, ko.arkhamdb.com for Korean)',
    pack => { code => 'core', name => { ko => '기본판', en => 'Core Set' }, released => $pack->{available} },
    card_count => scalar(@cards),
    cards => \@cards,
};

my $json = JSON::PP->new->utf8->pretty->indent_length(2)->sort_by(sub {
    my ($x, $y) = ($JSON::PP::a, $JSON::PP::b);
    ($RANK{$x} // 999) <=> ($RANK{$y} // 999) || $x cmp $y;
});
open my $fh, '>:raw', "$root/data/cards.json" or die $!;
print $fh $json->encode($out);
close $fh;
printf "cards.json: %d cards (player %d, encounter %d)\n", scalar(@cards),
    scalar(grep { $_->{deck} eq 'player' } @cards), scalar(grep { $_->{deck} eq 'encounter' } @cards);
