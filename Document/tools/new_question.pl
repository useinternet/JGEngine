# 사용자에게 물을 것을 Document/결정사항.md "확인할 것" 표에 한 줄 더한다. 대시보드 "결정 사항" 탭에 답하는 칸과 함께 보인다.
# 번호(Q-<번호>)는 문서에 나온 가장 큰 번호 + 1. 두 세션이 같은 번호를 쓰지 않게 잠근다(Decisions.pm).
# 사용: perl Document/tools/new_question.pl --from "<세션>" --area "<분야>" --title "<짧은 이름>" --why "<전후 사정>" --q "<질문>"
#            [--option "<선택지>" …(여러 번)] [--now "<지금 구현>"] [--image "<그림 경로>" …(여러 번)]
# --why(배경)는 꼭 쓴다(사용자 10-03: "이해 되게 전후 사정 설명해서"). 쉬운 말로, 화면에서 보이는 것으로. 한 덩어리로 쓰지 않고
#   줄을 나눠 쓴다(줄바꿈은 칸에 <br> 로 들어가 대시보드에서 줄이 나뉜다 — 10-24 사용자 "들여쓰기랑 줄바꿈 좀 개선"):
#     첫 줄 무엇을 묻는지(보고면 무엇을 했는지) / 사용자가 하신 말 원문
#     지금 상황 두세 줄(무엇을 하다가 생긴 질문인지 · 지금 어떻게 돼 있는지 · 앞서 정한 것과 어떻게 이어지는지)
#     ① ② ③ 선택지마다 한 줄 — 고르면 무엇이 달라지는지(좋은 점 · 대가)
#     권하는 것과 까닭 한 줄
#   짧은 문장, 코드 이름만 쓰지 않는다. --now 도 길면 줄을 나눈다.
# --title 은 그 질문을 부를 짧은 이름이다(예: "배포 번호 세는 법"). 대시보드가 글 속 번호(Q-…) 옆에 붙여 보여 준다(사용자 10-03).
#       perl Document/tools/new_question.pl --next      다음 번호만 출력
# 선택지는 대시보드에서 고르기 단추가 된다(①②… 는 이 도구가 붙인다). 사용자는 고르지 않고 글로만 답할 수도 있다.
# "지금 구현"은 답을 기다리는 동안 어떻게 해 두었는지(되돌리기 쉬운 쪽으로).
# --image 는 시안 · 비교 그림(저장소 기준 경로, 예: Saved/UIShots/7e0b_resources/compare_c2.png — 여러 장이면 여러 번).
#   대시보드 결정 사항 탭의 그 질문에 배경 아래 작은 그림으로 보이고, 누르면 크게 보인다. 목록은 표 밖
#   Saved/Decisions/images/<Q-번호>.txt(결정사항.md 표 열은 그대로). Saved/UIShots/ · Saved/Feedback/attachments/ · Saved/QA/ 아래의
#   png · jpg · jpeg · webp · gif 만, 파일이 있어야 한다(아니면 질문을 올리지 않고 끝난다). 이미 올린 질문은 question_images.pl 로.
use strict; use warnings; use utf8;
use Encode qw(decode_utf8);
use Getopt::Long qw(GetOptionsFromArray);
BEGIN { (my $dir = __FILE__) =~ s{\\}{/}g; unshift(@INC, $dir =~ m{^(.*)/} ? $1 : '.'); }
use Decisions qw(doc_rel read_doc write_doc with_lock find_table split_cells max_id esc_cell circled today);
require 'question_images.pl';   # 그림 경로 검사 · 목록 쓰기(QuestionImages — 같은 폴더)

binmode(STDOUT, ':encoding(UTF-8)');
binmode(STDERR, ':encoding(UTF-8)');

my @args = map { decode_utf8($_) } @ARGV;
my ($from, $area, $title, $why, $q, $now, $next, $help) = ('', '', '', '', '', '', 0, 0);
my (@options, @images);
GetOptionsFromArray(\@args,
    'from=s'   => \$from,
    'area=s'   => \$area,
    'why=s'    => \$why,
    'title=s'  => \$title,
    'q=s'      => \$q,
    'option=s' => \@options,
    'now=s'    => \$now,
    'image=s'  => \@images,
    'next'     => \$next,
    'help|h'   => \$help,
) or usage();
usage() if $help;

if ($next)
{
    printf "Q-%03d\n", max_id(read_doc()) + 1;
    exit 0;
}
s/^\s+|\s+$//g for ($from, $area, $title, $why, $q, $now);
usage('--q(질문)가 비었다') if $q eq '';
usage('--from(올린 세션)이 비었다') if $from eq '';
usage('--title(짧은 이름)이 비었다 — 예: "배포 번호 세는 법"') if $title eq '';
usage('--why(전후 사정)가 비었다 — 무엇을 하다가 · 지금 어떻게 돼 있고 · 고르면 무엇이 달라지는지 쉬운 말로') if $why eq '';
$area = '기타' if $area eq '';
usage('선택지는 20개까지') if @options > 20;
# 그림은 질문을 올리기 전에 검사한다(틀리면 아무것도 쓰지 않고 끝난다)
@images = eval { QuestionImages::check_paths(@images) };
if ($@)
{
    print STDERR $@;
    exit 2;
}

my $id = with_lock(sub
{
    my $lines = read_doc();
    my $t = find_table($lines, qr/확인할\s*것/) or die doc_rel() . ' 에 "## 확인할 것" 표가 없다' . "\n";
    my $id = sprintf('Q-%03d', max_id($lines) + 1);
    # 열은 머리 줄의 이름으로 채운다(열 순서가 바뀌어도 된다)
    my @head = split_cells($lines->[$t->{head}]);
    my %val = ('번호' => $id, '제목' => esc_cell($title), '분야' => esc_cell($area), '배경' => esc_cell($why), '질문' => esc_cell($q),
               '선택지' => circled(@options), '지금 구현' => esc_cell($now), '올린 곳' => esc_cell($from), '날짜' => today());
    my $row = '| ' . join(' | ', map { $val{$_} // '' } @head) . ' |';
    splice(@$lines, $t->{end}, 0, $row);
    # 그림 목록을 먼저 쓴다(그림이 없으면 같은 번호로 남은 옛 목록을 지운다 — 다른 질문의 그림이 붙지 않게)
    QuestionImages::write_list($id, @images);
    write_doc($lines);
    return $id;
});
print "$id → " . doc_rel() . " \"확인할 것\" — 대시보드 결정 사항 탭에 보인다. 답이 오면 엔진 총괄이 알려 준다.\n";
print "  그림 " . scalar(@images) . "장 → " . QuestionImages::list_rel($id) . "\n" if @images;
exit 0;

sub usage
{
    my ($msg) = @_;
    print STDERR "$msg\n" if $msg;
    print STDERR "usage: perl Document/tools/new_question.pl --from \"<세션>\" --area \"<분야>\" --title \"<짧은 이름>\" --why \"<전후 사정>\" --q \"<질문>\" [--option \"<선택지>\" ...] [--now \"<지금 구현>\"] [--image \"<그림 경로>\" ...] | --next\n";
    exit 2;
}
