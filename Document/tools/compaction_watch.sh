#!/bin/bash
# 대화 압축 감시(엔진 총괄용) — 전체 필수 지침 10: 팀 세션이 대화 압축 3번(일 끝) · 4번(단계 끝)이 되면 교체.
# 팀 세션 대화 기록(.jsonl)에서 "customTitle" 별로 가장 새 기록의 압축 수("isCompactSummary":true)를 세어,
# 3번 이상이 되거나 늘어날 때마다 한 줄을 낸다. 엔진 총괄이 Monitor 로 띄운다(30분마다 다시):
#   Monitor command: bash Document/tools/compaction_watch.sh   (작업 폴더 C:\Develop\JGEngine)
# 처음 켤 때는 지금 3번 이상인 세션을 한 번씩 알린다. 엔진 총괄 자신은 세지 않는다(프로젝트 총괄이 챙김).
# 보는 세션: 엔진 팀 분야 세션 여섯(Core · Graphics · GameFrameWork · GUI · DevOps · Editor) + Document Writer(10-24 구조 개편) — 세션 제목이 바뀌면 아래 목록도.
# 기록은 뒤에 덧붙기만 하므로 파일마다 읽은 자리를 기억해 두고 새로 붙은 부분만 읽는다(처음 한 번만 전부 읽음).
exec perl - <<'PERL'
use strict;
use warnings;
$| = 1;
my $P = "/c/Users/jgeng/.claude/projects/C--Develop-JGEngine";
my %watch = map { $_ => 1 } ("Core", "Graphics", "GameFrameWork", "GUI", "DevOps", "Editor", "Document Writer");
my %file;   # 기록 경로 => { off, title, n }
my %last;   # 세션 이름 => 마지막으로 본 압축 수
sub scan
{
    my %best;
    for my $p (glob("$P/*.jsonl"))
    {
        my @st = stat $p or next;
        my ($size, $m) = @st[7, 9];
        next if time - $m > 86400 * 2;
        my $e = $file{$p} //= { off => 0, title => "", n => 0 };
        if ($size < $e->{off})
        {
            %$e = (off => 0, title => "", n => 0);
        }
        if ($size > $e->{off} && open(my $fh, "<:raw", $p))
        {
            seek $fh, $e->{off}, 0;
            while (my $line = <$fh>)
            {
                last unless $line =~ /\n\z/;
                $e->{off} += length $line;
                $e->{title} = $1 if $line =~ /"customTitle":"([^"]*)"/;
                $e->{n}++ if $line =~ /"isCompactSummary":true/;
            }
            close $fh;
        }
        my $t = $e->{title};
        next unless $watch{$t};
        $best{$t} = [$m, $e->{n}] if !$best{$t} || $m > $best{$t}[0];
    }
    return \%best;
}
my $b = scan();
for my $t (sort keys %$b)
{
    my $n = $b->{$t}[1];
    $last{$t} = $n;
    print "교체 대상: $t — 대화 압축 ${n}번(지금)\n" if $n >= 3;
}
while (1)
{
    sleep 120;
    $b = scan();
    for my $t (sort keys %$b)
    {
        my $n = $b->{$t}[1];
        my $p = $last{$t} // 0;
        print "교체 대상: $t — 대화 압축 ${n}번(3번 뒤 일 끝 · 4번 뒤 단계 끝)\n" if $n >= 3 && $n > $p;
        $last{$t} = $n;
    }
}
PERL
