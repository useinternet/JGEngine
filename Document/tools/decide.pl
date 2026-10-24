# 정해진 질문을 Document/결정사항.md "확인할 것" 표에서 "정한 것" 표 맨 위로 옮긴다. 사용자가 대시보드에서 보낸 답
# (Saved/Decisions/answers/<번호>.json)은 Saved/Decisions/applied/ 로 옮겨 정한 것 · 반영한 곳을 덧붙인다.
# 엔진 총괄이 답을 맡은 세션에 넘긴 뒤 부른다. 오래 갈 결정은 CLAUDE.md "현재 결정"에도 따로 넣는다.
# 사용: perl Document/tools/decide.pl <Q-번호> --decision "<정한 것>" [--applied "<반영한 곳>"]
use strict; use warnings; use utf8;
use Encode qw(decode_utf8 encode_utf8);
use Getopt::Long qw(GetOptionsFromArray);
use JSON::PP;
use POSIX qw(strftime);
BEGIN { (my $dir = __FILE__) =~ s{\\}{/}g; unshift(@INC, $dir =~ m{^(.*)/} ? $1 : '.'); }
use Decisions qw(project_root doc_rel read_doc write_doc with_lock find_table split_cells esc_cell today);

binmode(STDOUT, ':encoding(UTF-8)');
binmode(STDERR, ':encoding(UTF-8)');

my @args = map { decode_utf8($_) } @ARGV;
my ($decision, $applied, $help) = ('', '', 0);
GetOptionsFromArray(\@args, 'decision=s' => \$decision, 'applied=s' => \$applied, 'help|h' => \$help) or usage();
usage() if $help || @args != 1;
my $id = uc($args[0]);
usage("번호는 Q-001 꼴: $args[0]") unless $id =~ /^Q-\d{3,}$/;
s/^\s+|\s+$//g for ($decision, $applied);
usage('--decision(정한 것)이 비었다') if $decision eq '';

my $question = with_lock(sub
{
    my $lines = read_doc();
    my $open = find_table($lines, qr/확인할\s*것/) or die doc_rel() . ' 에 "## 확인할 것" 표가 없다' . "\n";
    my @head = split_cells($lines->[$open->{head}]);
    my ($qcol) = grep { $head[$_] eq '질문' } 0 .. $#head;
    my ($at) = grep { (split_cells($lines->[$_]))[0] eq $id } @{$open->{rows}};
    die "$id 가 \"확인할 것\" 표에 없다(이미 정했거나 번호가 틀림)\n" unless defined $at;
    my @cells = split_cells($lines->[$at]);
    my $q = $cells[$qcol // 2] // '';
    my ($tcol) = grep { $head[$_] eq '제목' } 0 .. $#head;
    my $title = defined $tcol ? ($cells[$tcol] // '') : '';
    splice(@$lines, $at, 1);

    my $done = find_table($lines, qr/정한\s*것/) or die doc_rel() . ' 에 "## 정한 것" 표가 없다' . "\n";
    # 열은 "정한 것" 머리 줄의 이름으로 채운다
    my @dhead = split_cells($lines->[$done->{head}]);
    my %val = ('번호' => $id, '제목' => $title, '질문' => $q, '결정' => esc_cell($decision), '정한 날' => today(), '반영' => esc_cell($applied));
    my $row = '| ' . join(' | ', map { $val{$_} // '' } @dhead) . ' |';
    splice(@$lines, $done->{sep} + 1, 0, $row);
    write_doc($lines);
    return $q;
});

# 받은 답 → applied/
my $base    = project_root() . '/Saved/Decisions';
my $answer  = "$base/answers/$id.json";
my $applied_dir = "$base/applied";
mkdir($base) unless -d $base;
mkdir($applied_dir) unless -d $applied_dir;
my $json = JSON::PP->new->utf8->canonical->pretty;
my $rec = {};
if (open(my $fh, '<:raw', $answer))
{
    local $/;
    $rec = eval { $json->decode(<$fh>) } // {};
    close($fh);
}
my $had_answer = %$rec ? 1 : 0;
$rec->{id}        = $id;
$rec->{question}  //= $question;
$rec->{decision}  = $decision;
$rec->{applied}   = $applied;
$rec->{decidedAt} = strftime('%Y-%m-%d %H:%M:%S', localtime);
my $out = "$applied_dir/$id.json";
open(my $oh, '>:raw', "$out.tmp$$") or die "write $out: $!\n";
print $oh $json->encode($rec);
close($oh);
rename("$out.tmp$$", $out) or die "rename $out: $!\n";
unlink($answer) if $had_answer;

print "$id → \"정한 것\"" . ($had_answer ? ' · 받은 답은 Saved/Decisions/applied/' : '') . "\n";
exit 0;

sub usage
{
    my ($msg) = @_;
    print STDERR "$msg\n" if $msg;
    print STDERR "usage: perl Document/tools/decide.pl <Q-번호> --decision \"<정한 것>\" [--applied \"<반영한 곳>\"]\n";
    exit 2;
}
