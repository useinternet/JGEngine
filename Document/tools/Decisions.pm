# Document/결정사항.md 를 고치는 도구(new_question.pl · decide.pl)가 같이 쓰는 것.
# 문서는 줄 배열로 다루고, 표는 제목("## 확인할 것" · "## 정한 것") 아래 첫 표를 찾는다.
# 두 세션이 동시에 고치지 않게 Saved/Locks/decisions 로 잠근다.
package Decisions;
use strict; use warnings; use utf8;
use Encode qw(encode_utf8 decode_utf8);
use Time::HiRes qw(sleep);
use Cwd qw(abs_path);
use File::Basename qw(dirname);
use Exporter 'import';
our @EXPORT_OK = qw(project_root doc_rel read_doc write_doc with_lock find_table split_cells max_id esc_cell circled today);

(my $self_path = __FILE__) =~ s{\\}{/}g;
my $root  = abs_path(dirname($self_path) . '/../..');
my $DOC   = 'Document/결정사항.md';
my $locks = "$root/Saved/Locks";
my $lock  = "$locks/decisions";

sub project_root { return $root; }
sub doc_rel      { return $DOC; }
sub doc_path     { return $root . '/' . encode_utf8($DOC); }

sub read_doc
{
    open(my $fh, '<:raw', doc_path()) or die "$DOC 을 열지 못했다: $!\n";
    local $/;
    my $text = decode_utf8(<$fh> // '');
    close($fh);
    $text =~ s/^\x{FEFF}//;
    $text =~ s/\r\n?/\n/g;
    my @lines = split /\n/, $text, -1;
    pop @lines if @lines && $lines[-1] eq '';   # 끝 줄바꿈
    return \@lines;
}

sub write_doc
{
    my ($lines) = @_;
    my $path = doc_path();
    my $tmp  = "$path.tmp$$";
    open(my $fh, '>:raw', $tmp) or die "write $tmp: $!\n";
    print $fh encode_utf8(join("\n", @$lines) . "\n");
    close($fh) or die "close $tmp: $!\n";
    rename($tmp, $path) or do { my $err = $!; unlink($tmp); die "rename $tmp: $err\n"; };
}

# 잠근 채 함수를 부르고, 끝나면(죽어도) 푼다
sub with_lock
{
    my ($code) = @_;
    mkdir($locks) unless -d $locks;
    my $ok = 0;
    for (1 .. 50)
    {
        if (mkdir($lock))
        {
            $ok = 1;
            last;
        }
        # 30초 넘게 남은 잠금은 죽은 것으로 보고 치운다
        my @st = stat($lock);
        rmdir($lock) if @st && time() - $st[9] > 30;
        sleep(0.2);
    }
    die "결정 사항 잠금(Saved/Locks/decisions)을 10초 동안 잡지 못했다\n" unless $ok;
    my @ret = eval { $code->() };
    my $err = $@;
    rmdir($lock);
    die $err if $err;
    return wantarray ? @ret : $ret[0];
}

# 제목(정규식)에 맞는 절의 첫 표: {head => 머리 줄, sep => 구분 줄, rows => [첫 행, …], end => 표 다음 줄}
# 행이 없으면 rows 는 비고 end = sep + 1
sub find_table
{
    my ($lines, $title_re) = @_;
    my $in = 0;
    for (my $i = 0; $i < @$lines; $i++)
    {
        my $line = $lines->[$i];
        if ($line =~ /^(#{1,6})\s+(.*?)\s*$/)
        {
            last if $in;
            $in = $2 =~ $title_re ? 1 : 0;
            next;
        }
        next unless $in;
        if ($line =~ /^\s*\|/ && $i + 1 < @$lines && $lines->[$i + 1] =~ /^\s*\|?\s*:?-{3,}/)
        {
            my $end = $i + 2;
            $end++ while $end < @$lines && $lines->[$end] =~ /^\s*\|/;
            return { head => $i, sep => $i + 1, rows => [ $i + 2 .. $end - 1 ], end => $end };
        }
    }
    return undef;
}

# 표 한 줄 → 칸(앞뒤 공백 뺌). \| 는 글자 | 로 남긴다(\| 그대로)
sub split_cells
{
    my ($line) = @_;
    (my $s = $line) =~ s/^\s*\|//;
    $s =~ s/\|\s*$//;
    my @cells;
    my $cur = '';
    while ($s =~ /\G(\\.|[^\\|]+|\\|\|)/g)
    {
        if ($1 eq '|')
        {
            push @cells, $cur;
            $cur = '';
        }
        else
        {
            $cur .= $1;
        }
    }
    push @cells, $cur;
    s/^\s+|\s+$//g for @cells;
    return @cells;
}

# 문서 전체(확인할 것 · 정한 것)에서 가장 큰 Q 번호
sub max_id
{
    my ($lines) = @_;
    my $max = 0;
    for my $line (@$lines)
    {
        while ($line =~ /\bQ-(\d+)\b/g)
        {
            $max = $1 + 0 if $1 > $max;
        }
    }
    return $max;
}

# 칸에 넣을 글: 줄바꿈은 <br> 로(대시보드가 줄을 나눠 보임), | 는 \| 로
sub esc_cell
{
    my ($s) = @_;
    $s //= '';
    $s =~ s/^\s+|\s+$//g;
    $s =~ s/[ \t]*\r?\n[ \t]*/<br>/g;
    $s =~ s/(?<!\\)\|/\\|/g;
    return $s;
}

# 선택지 → "① 가 ② 나"
sub circled
{
    my @opts = grep { $_ ne '' } map { esc_cell($_) } @_;
    my @mark = map { chr(0x2460 + $_) } 0 .. 19;
    return join(' ', map { $mark[$_] . ' ' . $opts[$_] } 0 .. $#opts);
}

sub today
{
    my @t = localtime;
    return sprintf('%04d-%02d-%02d', $t[5] + 1900, $t[4] + 1, $t[3]);
}

1;
