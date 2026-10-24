#!/usr/bin/perl
# 피드백 하나를 "작업"으로 나눈다(10-15 사용자 6d18: "피드백 곧이곧대로 요청중인거에 다넣지 말고 알아서 잘 나눠서").
# 피드백(sent/<id>.json)은 총괄이 나눈 것으로 끝내고, 오래 걸리는 일은 제목이 붙은 작업 항목(sent/<새 id>.json, kind "work")으로
# 따로 둔다. 작업으로 옮기는 역할의 결과 파일(results/<id>.<역할>.txt)은 새 작업 id 로 옮긴다 — 맡은 세션에는 새 id 를 알린다.
#
# 사용: perl feedback_split.pl <피드백 id> <spec.json>
#   spec = { "lead_result": "피드백에 남길 총괄 결과 한두 줄",
#            "works": [ { "title": "작업 제목(화면에 보이는 말)",
#                         "tasks": [ {"role": "logic", "to": "게임 로직 구현", "summary": "…", "delivery": "delivered"} ],
#                         "move_results": ["logic"] } ] }
#   피드백에 남는 일: 작업으로 옮긴 역할의 task 를 빼고, 총괄(lead) task 를 하나 두며 lead 결과를 쓴다.
use strict;
use warnings;
use utf8;
use JSON::PP;
use File::Basename qw(dirname);
use Cwd qw(abs_path);
use POSIX qw(strftime);

binmode STDOUT, ':encoding(UTF-8)';
my ($id, $spec_path) = @ARGV;
die "usage: perl feedback_split.pl <id> <spec.json>\n"
    unless defined $spec_path && $id =~ /^[0-9]{8}-[0-9]{6}-[0-9a-f]{4}$/;

# 역할 키는 세션 넷 + 총괄(10-19 구조 개편). 옛 키(scenario · build · resources · qa1 · qa2)는 지난 결과 파일을 옮길 때(move_results)만 받는다.
my %ROLES     = map { $_ => 1 } qw(core graphics gameframework gui devops editor writer lead);
my %OLD_ROLES = map { $_ => 1 } qw();
(my $self_path = __FILE__) =~ s{\\}{/}g;
my $dir     = abs_path(dirname($self_path) . '/../..') . '/Saved/Feedback';
my $results = "$dir/results";
my $parent_path = "$dir/sent/$id.json";
die "sent/$id.json 이 없다 — 먼저 feedback_mark_sent.pl 로 넘긴다\n" unless -f $parent_path;

my $json   = JSON::PP->new->utf8->canonical->pretty;
my $parent = read_json($parent_path);
my $spec   = read_json($spec_path);
die "spec: works 가 없다\n" unless ref($spec->{works}) eq 'ARRAY' && @{$spec->{works}};

my $now = time();
my (%moved, @split);
my $n = 0;
for my $w (@{$spec->{works}})
{
    die "spec: work title 이 없다\n" unless defined $w->{title} && $w->{title} ne '';
    die "spec: work tasks 가 없다\n" unless ref($w->{tasks}) eq 'ARRAY' && @{$w->{tasks}};
    for my $t (@{$w->{tasks}})
    {
        die "spec: role 은 core · graphics · gameframework · gui · devops · editor · writer · lead 중 하나\n" unless $ROLES{$t->{role} // ''};
    }
    for my $role (@{$w->{move_results} // []})
    {
        die "spec: move_results 의 역할 $role 이 틀렸다\n" unless $ROLES{$role} || $OLD_ROLES{$role};
    }
    # 새 id: 지금 시각(작업마다 1초씩 띄움) + 무작위 4자리 — 피드백 id 와 같은 꼴
    my $wid;
    do
    {
        $wid = strftime('%Y%m%d-%H%M%S', localtime($now + $n)) . '-' . sprintf('%04x', int(rand(65536)));
    } while (-e "$dir/sent/$wid.json" || -e "$dir/inbox/$wid.json");
    $n++;
    my $item = {
        id        => $wid,
        kind      => 'work',
        parent    => $id,
        text      => $w->{title},
        createdAt => strftime('%Y-%m-%d %H:%M:%S', localtime($now)),
        sentAt    => strftime('%Y-%m-%d %H:%M:%S', localtime($now)),
        triage    => "피드백 $id 에서 나눈 작업",
        tasks     => [ map { my $t = $_; +{ map { defined $t->{$_} ? ($_ => $t->{$_}) : () } qw(role to summary delivery messageId) } } @{$w->{tasks}} ],
    };
    write_json("$dir/sent/$wid.json", $item);
    for my $role (@{$w->{move_results} // []})
    {
        my ($from, $to) = ("$results/$id.$role.txt", "$results/$wid.$role.txt");
        if (-f $from)
        {
            rename($from, $to) or die "rename $from: $!\n";
            print "결과 옮김: $id.$role.txt → $wid.$role.txt\n";
        }
        $moved{$role} = 1;
    }
    push @split, { id => $wid, title => $w->{title} };
    print "작업 $wid — $w->{title}\n";
}

# 피드백: 옮긴 역할의 task 를 빼고 lead task 하나(총괄이 나눔)
my @keep = grep { !$moved{$_->{role} // ''} } @{$parent->{tasks} // []};
push @keep, { role => 'lead', to => '엔진 총괄', summary => '작업으로 나눔', delivery => 'self' } unless grep { ($_->{role} // '') eq 'lead' } @keep;
$parent->{tasks} = \@keep;
$parent->{split} = [ @{$parent->{split} // []}, @split ];
write_json($parent_path, $parent);
if (defined $spec->{lead_result} && $spec->{lead_result} ne '')
{
    my $text = $spec->{lead_result};
    utf8::encode($text);
    open(my $o, '>:raw', "$results/$id.lead.txt") or die "write lead result: $!\n";
    print $o "$text\n";
    close($o);
}
print "피드백 $id — 남은 task: " . join(', ', map { $_->{role} } @keep) . "\n";

sub read_json
{
    my ($path) = @_;
    open(my $h, '<:raw', $path) or die "read $path: $!\n";
    local $/;
    my $bytes = <$h>;
    close($h);
    return JSON::PP->new->utf8->decode($bytes);
}

sub write_json
{
    my ($path, $data) = @_;
    open(my $o, '>:raw', "$path.tmp") or die "write $path.tmp: $!\n";
    print $o $json->encode($data);
    close($o) or die "close $path.tmp: $!\n";
    rename("$path.tmp", $path) or die "rename $path.tmp: $!\n";
}
