# ArkhamDB 전체 카드 덤프(cards_en.json / cards_ko.json)에서 코어 셋(core)과
# 개정판 코어(rcore) 카드만 뽑아 data/raw/ 에 코드 순으로 저장한다.
# 사용: perl extract_raw.pl <cards_en.json> <cards_ko.json> <out_dir>
use strict; use warnings; use JSON::PP;

my ($en_path, $ko_path, $out_dir) = @ARGV;
die "usage: perl extract_raw.pl <cards_en.json> <cards_ko.json> <out_dir>\n" unless $out_dir;

sub load_json
{
    my ($path) = @_;
    open my $fh, '<:raw', $path or die "open $path: $!";
    local $/;
    my $data = decode_json(<$fh>);
    close $fh;
    return $data;
}

sub save_json
{
    my ($path, $data) = @_;
    my $json = JSON::PP->new->utf8->pretty->canonical;
    open my $fh, '>:raw', $path or die "write $path: $!";
    print $fh $json->encode($data);
    close $fh;
    printf "%s : %d cards\n", $path, scalar(@$data);
}

for my $lang (['en', $en_path], ['ko', $ko_path])
{
    my ($tag, $path) = @$lang;
    my $all = load_json($path);
    for my $pack (qw(core rcore))
    {
        my @cards = sort { $a->{code} cmp $b->{code} } grep { $_->{pack_code} eq $pack } @$all;
        save_json("$out_dir/arkhamdb_${pack}_${tag}.json", \@cards);
    }
}
