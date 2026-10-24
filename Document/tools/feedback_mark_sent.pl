# 엔진 총괄이 피드백 한 건을 작업으로 나눠 넘긴 뒤 부른다: Saved/Feedback/inbox/<id>.json 에 나눈 내용(triage)과
# 작업(tasks: 누구에게 · 무엇을 · 넘긴 결과)을 더해 sent/<id>.json 으로 옮긴다.
# 대시보드(Document/대시보드.html)가 tasks 와 results/<id>.<역할>.txt(맡은 세션이 끝나면 쓴다)를 합쳐 진행을 보여 준다.
# 사용: perl feedback_mark_sent.pl <id> <spec.json>
#   spec.json = {"triage": "한 줄 요약",
#                "tasks": [{"role": "logic|presentation|support|qa|lead", "to": "게임 로직 구현", "summary": "맡긴 일 한 줄",
#                           "delivery": "delivered|queued|failed|self", "messageId": "...", "error": "..."}]}
#   delivery: delivered = 바로 시작, queued = 그 세션 대기열, failed = 못 넘김(error 에 이유), self = 총괄이 직접 한다
use strict; use warnings; use utf8;
use JSON::PP;
use POSIX qw(strftime);
use Cwd qw(abs_path);
use File::Basename qw(dirname);

binmode(STDOUT, ':encoding(UTF-8)');
binmode(STDERR, ':encoding(UTF-8)');

my ($id, $spec_path) = @ARGV;
die "usage: perl feedback_mark_sent.pl <id> <spec.json>\n"
    unless defined $spec_path && $id =~ /^[0-9]{8}-[0-9]{6}-[0-9a-f]{4}$/;

# 역할 키는 세션 넷 + 총괄(10-19 구조 개편). 옛 키(scenario · build · resources · qa1 · qa2)는 지난 기록에만 있다 — 새 작업에 쓰지 않는다.
my %ROLES      = map { $_ => 1 } qw(core graphics gameframework gui devops editor writer lead);
my %DELIVERIES = map { $_ => 1 } qw(delivered queued failed self);

(my $self_path = __FILE__) =~ s{\\}{/}g;
my $dir = abs_path(dirname($self_path) . '/../..') . '/Saved/Feedback';
my $in  = "$dir/inbox/$id.json";
my $out = "$dir/sent/$id.json";

my $item = read_json($in);
my $spec = read_json($spec_path);
die "spec: tasks 가 없다\n" unless ref($spec->{tasks}) eq 'ARRAY' && @{$spec->{tasks}};

my @tasks;
for my $t (@{$spec->{tasks}})
{
    die "spec: role 은 core · graphics · gameframework · gui · devops · editor · writer · lead 중 하나\n" unless $ROLES{$t->{role} // ''};
    die "spec: delivery 는 delivered · queued · failed · self 중 하나\n" unless $DELIVERIES{$t->{delivery} // ''};
    my %task = map { defined $t->{$_} ? ($_ => $t->{$_}) : () } qw(role to summary delivery messageId error);
    push @tasks, \%task;
}

$item->{triage} = $spec->{triage} if defined $spec->{triage};
$item->{tasks}  = \@tasks;
$item->{sentAt} = strftime('%Y-%m-%d %H:%M:%S', localtime);

open(my $o, '>:raw', "$out.tmp") or die "write $out.tmp: $!\n";
print $o JSON::PP->new->utf8->canonical->pretty->encode($item);
close($o) or die "close $out.tmp: $!\n";
rename("$out.tmp", $out) or die "rename $out.tmp: $!\n";
unlink($in) or die "unlink $in: $!\n";
print "sent/$id.json — " . join(', ', map { "$_->{role}:$_->{delivery}" } @tasks) . "\n";

sub read_json
{
    my ($path) = @_;
    open(my $fh, '<:raw', $path) or die "open $path: $!\n";
    local $/;
    my $data = decode_json(<$fh>);
    close($fh);
    die "$path: JSON 객체가 아니다\n" unless ref($data) eq 'HASH';
    return $data;
}
