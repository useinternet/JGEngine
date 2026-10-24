# 문서 도구가 같이 쓰는 마크다운 → HTML 변환. 이 프로젝트 문서가 쓰는 만큼만 한다:
# 제목 · 문단 · 표 · 목록(중첩) · 인용 · 코드 블록 · 굵게 · 기울임 · 코드 · 링크.
# 쓰는 곳: build_engine_requests_html.pl(엔진 요청 통합 보기) · feedback_server.pl(대시보드)
#
# ctx(해시 참조):
#   shift      제목 단계를 몇 단 내릴지(# → h(1 + shift))
#   href       링크 주소를 바꾸는 함수(받은 주소 → 쓸 주소). 없으면 그대로
#   headings   배열 참조를 주면 제목마다 {level, text, id} 를 모으고 제목에 id 를 붙인다(id_prefix-번호)
# 링크는 상대 주소 · #앵커 · http · https · file · mailto 만 걸고, 다른 주소(javascript: 등)는 글자만 남긴다.
package MdLite;
use strict; use warnings; use utf8;
use Exporter 'import';

our @EXPORT_OK = qw(render_blocks inline sections split_row is_table_sep normalize_path esc unesc plain);

sub render_blocks
{
    my ($text, $ctx) = @_;
    my @lines = split /\n/, $text, -1;
    my $n = scalar(@lines);
    my @out;
    my $i = 0;
    while ($i < $n)
    {
        my $line = $lines[$i];
        if ($line =~ /^\s*$/)
        {
            $i++;
            next;
        }
        if ($line =~ /^\s*(```+|~~~+)/)
        {
            my $fence = $1;
            my @code;
            $i++;
            while ($i < $n && $lines[$i] !~ /^\s*\Q$fence\E\s*$/)
            {
                push @code, $lines[$i];
                $i++;
            }
            $i++;
            push @out, '<pre><code>' . esc(join("\n", @code)) . '</code></pre>';
            next;
        }
        if ($line =~ /^(#{1,6})\s+(.*?)\s*#*\s*$/)
        {
            my ($hashes, $title) = ($1, $2);
            my $level = length($hashes) + ($ctx->{shift} // 0);
            $level = 6 if $level > 6;
            my $id = '';
            if (ref($ctx->{headings}) eq 'ARRAY')
            {
                my $hid = ($ctx->{id_prefix} // 'h') . '-' . (scalar(@{$ctx->{headings}}) + 1);
                push @{$ctx->{headings}}, { level => length($hashes), text => plain($title), id => $hid };
                $id = qq{ id="$hid"};
            }
            push @out, "<h$level$id>" . inline($title, $ctx) . "</h$level>";
            $i++;
            next;
        }
        if ($line =~ /^\s*([-*_])(?:\s*\1){2,}\s*$/)
        {
            push @out, '<hr>';
            $i++;
            next;
        }
        if ($line =~ /^\s*\|/ && $i + 1 < $n && is_table_sep($lines[$i + 1]))
        {
            my @head  = split_row($line);
            my @align = map { /^:-+:$/ ? 'center' : /^-+:$/ ? 'right' : '' } split_row($lines[$i + 1]);
            my @rows;
            $i += 2;
            while ($i < $n && $lines[$i] =~ /^\s*\|/)
            {
                push @rows, [split_row($lines[$i])];
                $i++;
            }
            push @out, render_table(\@head, \@align, \@rows, $ctx);
            next;
        }
        if ($line =~ /^\s*>/)
        {
            my @quote;
            while ($i < $n && $lines[$i] =~ /^\s*>\s?(.*)$/)
            {
                push @quote, $1;
                $i++;
            }
            push @out, '<blockquote>' . render_blocks(join("\n", @quote), $ctx) . '</blockquote>';
            next;
        }
        if ($line =~ /^\s*(?:[-*+]|\d+[.)])\s+/)
        {
            my @items;
            my $prev_blank = 0;
            while ($i < $n)
            {
                my $l = $lines[$i];
                if ($l =~ /^(\s*)([-*+]|\d+[.)])\s+(.*)$/)
                {
                    my ($indent, $marker, $item) = ($1, $2, $3);
                    $indent =~ s/\t/    /g;
                    push @items, {
                        indent => length($indent),
                        type   => $marker =~ /\d/ ? 'ol' : 'ul',
                        start  => $marker =~ /(\d+)/ ? $1 + 0 : 1,
                        text   => $item,
                    };
                    $prev_blank = 0;
                    $i++;
                    next;
                }
                if ($l =~ /^\s*$/)
                {
                    # 빈 줄 뒤에 목록 항목이 또 오면 같은 목록으로 본다
                    my $j = $i + 1;
                    $j++ while $j < $n && $lines[$j] =~ /^\s*$/;
                    last unless $j < $n && $lines[$j] =~ /^\s*(?:[-*+]|\d+[.)])\s+/;
                    $prev_blank = 1;
                    $i = $j;
                    next;
                }
                # 들여 쓴 줄, 또는 빈 줄 없이 이어진 줄은 앞 항목에 붙는다
                last if is_block_start($l, $i + 1 < $n ? $lines[$i + 1] : '');
                last if $prev_blank && $l !~ /^\s/;
                (my $cont = $l) =~ s/^\s+//;
                $items[-1]{text} .= "\n" . $cont;
                $i++;
            }
            push @out, render_list(\@items, $ctx);
            next;
        }
        my @para;
        while ($i < $n && $lines[$i] !~ /^\s*$/ && (!@para || !is_block_start($lines[$i], $i + 1 < $n ? $lines[$i + 1] : '')))
        {
            (my $l = $lines[$i]) =~ s/^\s+|\s+$//g;
            push @para, $l;
            $i++;
        }
        push @out, '<p>' . join("<br>\n", map { inline($_, $ctx) } @para) . '</p>';
    }
    return join("\n", @out);
}

# 제목마다 그 아래 표를 모은다: ({level, title, tables => [{head => [...], rows => [[...], ...]}]}, ...)
# 첫 제목 앞 부분은 level 0 · 제목 "" 이다. 칸은 마크다운 그대로(inline 으로 바꿔 쓴다).
sub sections
{
    my ($text) = @_;
    my @lines = split /\n/, $text, -1;
    my @secs = ({ level => 0, title => '', tables => [] });
    my $fence = '';
    for (my $i = 0; $i < @lines; $i++)
    {
        my $line = $lines[$i];
        if ($fence ne '')
        {
            $fence = '' if $line =~ /^\s*\Q$fence\E\s*$/;
            next;
        }
        if ($line =~ /^\s*(```+|~~~+)/)
        {
            $fence = $1;
            next;
        }
        if ($line =~ /^(#{1,6})\s+(.*?)\s*#*\s*$/)
        {
            push @secs, { level => length($1), title => $2, tables => [] };
            next;
        }
        if ($line =~ /^\s*\|/ && $i + 1 < @lines && is_table_sep($lines[$i + 1]))
        {
            my @head = split_row($line);
            my @rows;
            $i += 2;
            while ($i < @lines && $lines[$i] =~ /^\s*\|/)
            {
                push @rows, [split_row($lines[$i])];
                $i++;
            }
            $i--;
            push @{$secs[-1]{tables}}, { head => \@head, rows => \@rows };
        }
    }
    return @secs;
}

sub is_block_start
{
    my ($line, $next) = @_;
    return 1 if $line =~ /^\s*(?:```|~~~)/;
    return 1 if $line =~ /^#{1,6}\s/;
    return 1 if $line =~ /^\s*>/;
    return 1 if $line =~ /^\s*(?:[-*+]|\d+[.)])\s+/;
    return 1 if $line =~ /^\s*([-*_])(?:\s*\1){2,}\s*$/;
    return 1 if $line =~ /^\s*\|/ && is_table_sep($next // '');
    return 0;
}

sub is_table_sep
{
    my ($line) = @_;
    return $line =~ /^\s*\|?\s*:?-{3,}:?\s*(?:\|\s*:?-{3,}:?\s*)*\|?\s*$/ ? 1 : 0;
}

# 표 한 줄을 칸으로 나눈다. \| 는 칸 나누기가 아니라 글자 | 다.
sub split_row
{
    my ($line) = @_;
    $line =~ s/^\s*\|//;
    $line =~ s/(?<!\\)\|\s*$//;
    my @cells;
    my $cur = '';
    my @ch = split //, $line;
    for (my $k = 0; $k < @ch; $k++)
    {
        if ($ch[$k] eq '\\' && $k + 1 < @ch && $ch[$k + 1] eq '|')
        {
            $cur .= '|';
            $k++;
            next;
        }
        if ($ch[$k] eq '|')
        {
            push @cells, $cur;
            $cur = '';
            next;
        }
        $cur .= $ch[$k];
    }
    push @cells, $cur;
    s/^\s+|\s+$//g for @cells;
    return @cells;
}

sub render_table
{
    my ($head, $align, $rows, $ctx) = @_;
    my $html = '<div class="tw"><table><thead><tr>';
    for my $c (0 .. $#$head)
    {
        my $style = $align->[$c] ? qq{ style="text-align:$align->[$c]"} : '';
        $html .= "<th$style>" . inline($head->[$c], $ctx) . '</th>';
    }
    $html .= '</tr></thead><tbody>';
    for my $row (@$rows)
    {
        $html .= '<tr>';
        for my $c (0 .. $#$head)
        {
            my $style = $align->[$c] ? qq{ style="text-align:$align->[$c]"} : '';
            my $cell  = inline($row->[$c] // '', $ctx);
            # 날짜 · 번호처럼 짧고 띄어쓰기 없는 칸은 줄을 바꾸지 않는다(2026-/09-30 처럼 갈라지지 않게)
            (my $plain = $cell) =~ s/<[^>]+>//g;
            my $class = (length(unesc($plain)) <= 12 && unesc($plain) !~ /\s/) ? ' class="nw"' : '';
            $html .= "<td$class$style>" . $cell . '</td>';
        }
        $html .= '</tr>';
    }
    return $html . '</tbody></table></div>';
}

# 들여쓰기로 중첩을 만든다. 같은 깊이에서 번호 목록 ↔ 점 목록이 바뀌면 새 목록이다.
sub render_list
{
    my ($items, $ctx) = @_;
    my $html = '';
    my @stack;
    for my $it (@$items)
    {
        while (@stack && $stack[-1]{indent} > $it->{indent})
        {
            $html .= "</li></$stack[-1]{type}>";
            pop @stack;
        }
        if (@stack && $stack[-1]{indent} == $it->{indent} && $stack[-1]{type} ne $it->{type})
        {
            $html .= "</li></$stack[-1]{type}>";
            pop @stack;
        }
        if (!@stack || $it->{indent} > $stack[-1]{indent})
        {
            my $start = ($it->{type} eq 'ol' && $it->{start} != 1) ? qq{ start="$it->{start}"} : '';
            $html .= "<$it->{type}$start>";
            push @stack, { indent => $it->{indent}, type => $it->{type} };
        }
        else
        {
            $html .= '</li>';
        }
        $html .= '<li>' . join("<br>\n", map { inline($_, $ctx) } split /\n/, $it->{text});
    }
    while (@stack)
    {
        $html .= "</li></$stack[-1]{type}>";
        pop @stack;
    }
    return $html;
}

sub inline
{
    my ($s, $ctx) = @_;
    my @held;
    my $hold = sub
    {
        push @held, $_[0];
        return "\x{E000}" . $#held . "\x{E001}";
    };
    $s =~ s/(`+)(.+?)\1/$hold->('<code>' . esc(trim_code($2)) . '<\/code>')/ge;
    $s =~ s/\\([\\`*_{}\[\]()#+\-.!|<>~])/$hold->(esc($1))/ge;
    $s = esc($s);
    $s =~ s/\[([^\]]+)\]\(([^)\s]+)\)/link_html($1, $2, $ctx)/ge;
    $s =~ s/\*\*(?=\S)(.+?)(?<=\S)\*\*/<strong>$1<\/strong>/g;
    $s =~ s/(?<![*\w])\*(?=[^*\s])(.+?)(?<=[^*\s])\*(?![*\w])/<em>$1<\/em>/g;
    1 while $s =~ s/\x{E000}(\d+)\x{E001}/$held[$1]/g;
    return $s;
}

sub link_html
{
    my ($text, $raw, $ctx) = @_;
    my $url = unesc($raw);
    if (ref($ctx->{href}) eq 'CODE')
    {
        $url = $ctx->{href}->($url);
    }
    return $text unless safe_url($url);
    return '<a href="' . esc($url) . '">' . $text . '</a>';
}

sub safe_url
{
    my ($url) = @_;
    return 1 if $url !~ /^[a-z][a-z0-9+.-]*:/i;
    return $url =~ /^(?:https?|file|mailto):/i ? 1 : 0;
}

sub trim_code
{
    my ($code) = @_;
    $code =~ s/^ (.*) \z/$1/s if $code =~ /\S/;
    return $code;
}

# 목차 · 요약에 쓰는 글자만: 링크는 글자로, ` * \ 표시는 뺀다
sub plain
{
    my ($s) = @_;
    $s =~ s/\[([^\]]+)\]\([^)]*\)/$1/g;
    $s =~ s/\\(.)/$1/g;
    $s =~ s/[`*]//g;
    return $s;
}

sub normalize_path
{
    my ($path) = @_;
    my $trailing = $path =~ m{/\z};
    my @parts;
    for my $part (split m{/}, $path)
    {
        next if $part eq '' || $part eq '.';
        if ($part eq '..' && @parts && $parts[-1] ne '..')
        {
            pop @parts;
            next;
        }
        push @parts, $part;
    }
    my $result = join('/', @parts);
    $result .= '/' if $trailing && $result ne '';
    return $result;
}

sub esc
{
    my ($s) = @_;
    $s =~ s/&/&amp;/g;
    $s =~ s/</&lt;/g;
    $s =~ s/>/&gt;/g;
    $s =~ s/"/&quot;/g;
    return $s;
}

sub unesc
{
    my ($s) = @_;
    $s =~ s/&lt;/</g;
    $s =~ s/&gt;/>/g;
    $s =~ s/&quot;/"/g;
    $s =~ s/&amp;/&/g;
    return $s;
}

1;
