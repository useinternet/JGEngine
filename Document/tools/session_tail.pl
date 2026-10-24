#!/usr/bin/perl
# session_tail.pl — 예전 Claude 세션 기록(.jsonl)의 끝부분을 읽기 좋게 뽑는다.
# 세션을 새로 만들어 이어받을 때 쓴다(10-17, 총괄): 마지막 요약(대화 압축) + 그 뒤 주고받은 글.
#
#   perl Document/tools/session_tail.pl <기록.jsonl> [--last N] [--chars C] [--no-summary] [--tools]
#     --last N      끝에서 N 개 글(기본 40)
#     --chars C     글 하나당 최대 글자 수(기본 1500, 요약은 C*10)
#     --no-summary  마지막 대화 압축 요약을 빼고 끝부분만
#     --tools       도구 호출도 한 줄씩(Bash 설명 · 고친 파일)
#
# 기록 자리: C:/Users/jgeng/.claude/projects/C--Develop-JGEngine/<세션 uuid>.jsonl
use strict;
use warnings;
use utf8;
use JSON::PP;

binmode STDOUT, ':encoding(UTF-8)';

my ($path, $last, $chars, $summary, $tools) = (undef, 40, 1500, 1, 0);
while (@ARGV)
{
    my $a = shift @ARGV;
    if ($a eq '--last') { $last = shift @ARGV; }
    elsif ($a eq '--chars') { $chars = shift @ARGV; }
    elsif ($a eq '--no-summary') { $summary = 0; }
    elsif ($a eq '--tools') { $tools = 1; }
    else { $path = $a; }
}
die "usage: session_tail.pl <transcript.jsonl> [--last N] [--chars C] [--no-summary] [--tools]\n" unless defined $path && -f $path;

my $json = JSON::PP->new->utf8;

sub cut
{
    my ($s, $n) = @_;
    $s =~ s/\s+$//;
    return length($s) > $n ? substr($s, 0, $n) . " …(" . (length($s) - $n) . "자 더)" : $s;
}

# 한 줄(JSON) → [시각, 누가, 글] 목록
sub entries
{
    my ($line) = @_;
    my $o = eval { $json->decode($line) };
    return () unless ref $o eq 'HASH';
    my $type = $o->{type} // '';
    return () unless $type eq 'user' || $type eq 'assistant';
    return () if $o->{isSidechain};
    my $ts = $o->{timestamp} // '';
    $ts =~ s/^\d{4}-(\d\d-\d\dT\d\d:\d\d).*/$1/;
    my $msg = $o->{message} // {};
    my $c = $msg->{content};
    my @out;
    my $who = $type eq 'user' ? ($o->{isCompactSummary} ? 'SUMMARY' : 'USER') : 'CLAUDE';
    if (!ref $c)
    {
        push @out, [$ts, $who, $c] if defined $c && length $c;
        return @out;
    }
    for my $b (@$c)
    {
        next unless ref $b eq 'HASH';
        my $bt = $b->{type} // '';
        if ($bt eq 'text' && length($b->{text} // ''))
        {
            push @out, [$ts, $who, $b->{text}];
        }
        elsif ($bt eq 'tool_use' && $tools)
        {
            my $in = $b->{input} // {};
            my $what = $in->{description} // $in->{file_path} // $in->{to} // $in->{title} // '';
            push @out, [$ts, 'TOOL', ($b->{name} // '?') . ($what ne '' ? ": $what" : '')];
        }
    }
    return @out;
}

# 1) 마지막 대화 압축 요약 — 파일 전체를 줄 단위로 훑는다
my ($sumLine, $sumNo) = (undef, -1);
my $total = 0;
open my $fh, '<:raw', $path or die "open $path: $!\n";
while (my $l = <$fh>)
{
    $total++;
    if ($summary && index($l, '"isCompactSummary":true') >= 0)
    {
        ($sumLine, $sumNo) = ($l, $total);
    }
}
close $fh;

# 2) 끝부분 — 뒤에서부터 넉넉히 읽어 글 N 개를 모은다
my $size = -s $path;
my @tail;
for my $span (4_000_000, 16_000_000, 64_000_000, $size)
{
    my $from = $size > $span ? $size - $span : 0;
    open my $th, '<:raw', $path or die;
    seek $th, $from, 0;
    <$th> if $from > 0;    # 잘린 첫 줄 버림
    my @lines = <$th>;
    close $th;
    my $startNo = $total - scalar(@lines) + 1;
    @tail = ();
    for my $i (0 .. $#lines)
    {
        next if $sumNo > 0 && $startNo + $i <= $sumNo;    # 요약 앞 글은 요약에 들어 있다
        push @tail, entries($lines[$i]);
    }
    last if scalar(grep { $_->[1] ne 'TOOL' } @tail) >= $last || $from == 0 || ($sumNo > 0 && $startNo <= $sumNo);
}
my @talk = grep { $_->[1] ne 'TOOL' } @tail;
if (@talk > $last)
{
    my $keepFrom = $talk[-$last];
    my $i = 0;
    $i++ while $tail[$i] != $keepFrom;
    @tail = @tail[$i .. $#tail];
}

print "# $path\n# 줄 $total 개, 크기 " . int($size / 1048576) . "MB\n\n";
if (defined $sumLine)
{
    my ($e) = entries($sumLine);
    print "=== 마지막 대화 압축 요약 (줄 $sumNo, $e->[0]) ===\n" . cut($e->[2], $chars * 10) . "\n\n" if $e;
}
print "=== 그 뒤 끝부분 (글 " . scalar(grep { $_->[1] ne 'TOOL' } @tail) . " 개) ===\n";
for my $e (@tail)
{
    if ($e->[1] eq 'TOOL')
    {
        print "  · [$e->[0]] $e->[2]\n";
        next;
    }
    print "\n--- [$e->[0]] $e->[1]\n" . cut($e->[2], $chars) . "\n";
}
