# 결정 사항 질문(Q-…)에 시안 그림을 붙인다. 대시보드 "결정 사항" 탭의 그 질문 카드(답할 질문 · 정한 것 모두)에
# 배경 글 아래 작은 그림으로 보이고, 누르면 크게 보인다(10-15 사용자 — UI 모양을 바꿀 때는 지금 · 시안 비교 그림을 먼저 보이고 고르게).
# 그림 목록은 표 밖 Saved/Decisions/images/<Q-번호>.txt 에 한 줄에 하나(저장소 기준 경로)로 둔다 — 결정사항.md 의 표 열은 그대로.
# 질문을 새로 올릴 때는 new_question.pl --image 로 같이 붙이고, 이미 올린 질문은 이 도구로 붙인다.
# 사용: perl Document/tools/question_images.pl <Q-번호> <그림 경로> [<그림 경로> …]    목록을 이것으로 바꾼다
#       perl Document/tools/question_images.pl --add <Q-번호> <그림 경로> [<그림 경로> …]   있는 목록 뒤에 더한다
#       perl Document/tools/question_images.pl --clear <Q-번호>                         목록을 지운다
#       perl Document/tools/question_images.pl <Q-번호>                                 지금 목록을 보인다
# 그림 경로는 저장소 기준(예: Saved/UIShots/7e0b_resources/compare_c2.png). 대시보드 서버가 보여 줄 수 있는 곳
# (Saved/UIShots/ · Saved/Feedback/attachments/ · Saved/QA/ 아래)의 png · jpg · jpeg · webp · gif 만 받는다. 파일이 있어야 한다.
# new_question.pl 이 이 파일을 require 해서 같은 검사 · 쓰기를 쓴다(그때는 아래 main 이 돌지 않는다).
package QuestionImages;
use strict; use warnings; use utf8;
use Encode qw(decode_utf8 encode_utf8);
use Getopt::Long qw(GetOptionsFromArray);
BEGIN { (my $dir = __FILE__) =~ s{\\}{/}g; unshift(@INC, $dir =~ m{^(.*)/} ? $1 : '.'); }
use Decisions qw(project_root doc_rel read_doc find_table split_cells);

our @ALLOWED = ('Saved/UIShots/', 'Saved/Feedback/attachments/', 'Saved/QA/');   # 서버(feedback_server.pl /api/image)와 같은 곳
our $LIST_DIR = 'Saved/Decisions/images';

sub list_rel { return "$LIST_DIR/$_[0].txt"; }

# 그림 경로 하나 → 저장소 기준 경로. 받지 못할 경로면 죽는다
sub check_path
{
    my ($in) = @_;
    my $p = $in // '';
    $p =~ s/^\s+|\s+$//g;
    $p =~ s{\\}{/}g;
    die "그림 경로가 비었다\n" if $p eq '';
    # 저장소 안의 절대 경로(C:/Develop/JGEngine/… · /c/Develop/JGEngine/…)는 저장소 기준으로 바꿔 준다
    if ($p =~ m{^(?:/|[A-Za-z]:)})
    {
        my $root = project_root();
        my @roots = ($root);
        push @roots, "$1:/$2" if $root =~ m{^/([A-Za-z])/(.*)\z};
        push @roots, "/" . lc($1) . "/$2" if $root =~ m{^([A-Za-z]):/(.*)\z};
        my ($hit) = grep { lc(substr($p, 0, length($_) + 1)) eq lc("$_/") } @roots;
        die "저장소($root) 밖의 경로는 받지 않는다 — 저장소 기준 경로로: $in\n" unless $hit;
        $p = substr($p, length($hit) + 1);
    }
    $p =~ s{^(?:\./)+}{};
    die "경로에는 영문 · 숫자 · . _ - / 만 쓴다(대시보드 서버가 그 밖의 글자는 열지 않는다): $in\n" unless $p =~ m{\A[A-Za-z0-9._/-]+\z};
    die "경로에 . · .. · 빈 칸(//)을 쓰지 않는다: $in\n" if grep { $_ eq '' || $_ eq '.' || $_ eq '..' } split(m{/}, $p, -1);
    die "그림(png · jpg · jpeg · webp · gif)만 붙인다: $in\n" unless $p =~ /\.(?:png|jpe?g|webp|gif)\z/i;
    die "대시보드가 보여 줄 수 있는 곳(" . join(' · ', @ALLOWED) . " 아래)의 그림만 붙인다: $in\n"
        unless grep { substr($p, 0, length($_)) eq $_ } @ALLOWED;
    die "그림 파일이 없다(저장소 기준 경로인지 확인): $p\n" unless -f project_root() . "/$p";
    return $p;
}

# 여러 경로 → 검사한 경로(같은 것은 한 번만)
sub check_paths
{
    my %seen;
    return grep { !$seen{$_}++ } map { check_path($_) } @_;
}

sub read_list
{
    my ($id) = @_;
    my $path = project_root() . '/' . list_rel($id);
    open(my $fh, '<:raw', $path) or return ();
    local $/;
    my $text = decode_utf8(<$fh> // '');
    close($fh);
    $text =~ s/^\x{FEFF}//;
    return grep { $_ ne '' } map { s/^\s+|\s+$//gr } split /\r?\n/, $text;
}

# 목록을 쓴다(UTF-8 · LF). 빈 목록이면 파일을 지운다
sub write_list
{
    my ($id, @paths) = @_;
    my $root = project_root();
    my $dir  = "$root/$LIST_DIR";
    my $path = "$root/" . list_rel($id);
    if (!@paths)
    {
        unlink($path);
        return;
    }
    for my $d ("$root/Saved", "$root/Saved/Decisions", $dir)
    {
        mkdir($d) unless -d $d;
    }
    my $tmp = "$path.tmp$$";
    open(my $fh, '>:raw', $tmp) or die "write $tmp: $!\n";
    print $fh encode_utf8(join('', map { "$_\n" } @paths));
    close($fh) or die "close $tmp: $!\n";
    rename($tmp, $path) or do { my $err = $!; unlink($tmp); die "rename $tmp: $err\n"; };
}

# 결정사항.md 의 "확인할 것" · "정한 것" 표에 그 번호가 있는지
sub question_exists
{
    my ($id) = @_;
    my $lines = read_doc();
    for my $re (qr/확인할\s*것/, qr/정한\s*것/)
    {
        my $t = find_table($lines, $re) or next;
        my @head = split_cells($lines->[$t->{head}]);
        my ($col) = grep { $head[$_] eq '번호' } 0 .. $#head;
        $col //= 0;
        return 1 if grep { ((split_cells($lines->[$_]))[$col] // '') eq $id } @{$t->{rows}};
    }
    return 0;
}

sub main
{
    binmode(STDOUT, ':encoding(UTF-8)');
    binmode(STDERR, ':encoding(UTF-8)');
    my @args = map { decode_utf8($_) } @_;
    my ($add, $clear, $help) = (0, 0, 0);
    GetOptionsFromArray(\@args, 'add' => \$add, 'clear' => \$clear, 'help|h' => \$help) or usage();
    usage() if $help || !@args || ($add && $clear);
    my $id = uc(shift @args);
    usage("번호는 Q-001 꼴: $id") unless $id =~ /^Q-\d{3,}$/;
    usage('--clear 에는 그림 경로를 주지 않는다') if $clear && @args;
    usage('--add 에는 그림 경로가 하나 이상 있어야 한다') if $add && !@args;
    if (!question_exists($id))
    {
        print STDERR "$id 가 " . doc_rel() . " 의 \"확인할 것\" · \"정한 것\" 표에 없다(번호가 틀림?)\n";
        exit 1;
    }
    my @old = read_list($id);
    if (!@args && !$clear)
    {
        print @old ? join('', map { "$_\n" } @old) : "$id 에 붙인 그림이 없다\n";
        exit 0;
    }
    my @new = eval { check_paths(@args) };
    if ($@)
    {
        print STDERR $@;
        exit 2;
    }
    if ($add)
    {
        my %seen;
        @new = grep { !$seen{$_}++ } (@old, @new);
    }
    write_list($id, $clear ? () : @new);
    if ($clear)
    {
        print "$id 의 그림 목록을 지웠다\n";
    }
    else
    {
        print "$id 그림 " . scalar(@new) . "장 → " . list_rel($id) . " — 대시보드 결정 사항 탭의 그 질문에 보인다\n";
        print "  $_\n" for @new;
    }
    exit 0;
}

sub usage
{
    my ($msg) = @_;
    print STDERR "$msg\n" if $msg;
    print STDERR "usage: perl Document/tools/question_images.pl [--add] <Q-번호> <그림 경로> [<그림 경로> ...] | --clear <Q-번호> | <Q-번호>\n";
    exit 2;
}

main(@ARGV) unless caller;
1;
