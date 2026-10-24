# Engine-35: 게임 쪽 코드가 include 하는 엔진 헤더와 쓰는 엔진 이름(타입 · 매크로 · 함수 · 멤버 함수)을 모듈별로 센다.
# usage: python -X utf8 -I api_usage.py <engine root> <out json>
import json
import os
import re
import sys
from collections import defaultdict

ENGINE = sys.argv[1]
OUT = sys.argv[2]

MODULES = {
    'Core': 'Source/Runtime/Core',
    'Graphics': 'Source/Runtime/Graphics',
    'GameFrameWork': 'Source/Runtime/GameFrameWork',
    'GUI': 'Source/Runtime/GUI',
    'GameHost': 'Source/Runtime/GameHost',
    'JGEditor': 'Source/Editor/JGEditor',
}

GAMES = {
    'ProjectAH': ['C:/Develop/ProjectAH/Source/ProjectAH', 'C:/Develop/ProjectAH/Source/ProjectAHEditor'],
    'Template': [ENGINE + '/Build/Templates/GameProject/Source/{PROJECT_NAME}', ENGINE + '/Build/Templates/GameProject/Source/{PROJECT_NAME}Editor'],
    'AudioCheck': [ENGINE + '/Tools/Core/audiocheck/game'],
    'GameUIDemo': [ENGINE + '/Tools/GameFrameWork/gameui_demo_testgame'],
    'GfwView': [ENGINE + '/Tools/GameFrameWork/gfw_phase2_testgame'],
    'NetDuel': [ENGINE + '/Tools/GameFrameWork/net_duel_testgame'],
}

EXT = ('.h', '.hpp', '.cpp', '.inl')


def read(path):
    with open(path, 'rb') as f:
        data = f.read()
    if data.startswith(b'\xef\xbb\xbf'):
        data = data[3:]
    return data.decode('utf-8', errors='replace')


def strip_code(text):
    # 주석 · 문자열 · 문자 상수를 빈칸으로(줄 수는 지킨다)
    out = []
    i = 0
    n = len(text)
    while i < n:
        c = text[i]
        if c == '/' and i + 1 < n and text[i + 1] == '/':
            j = text.find('\n', i)
            j = n if j < 0 else j
            i = j
        elif c == '/' and i + 1 < n and text[i + 1] == '*':
            j = text.find('*/', i + 2)
            j = n if j < 0 else j + 2
            out.append('\n' * text.count('\n', i, j))
            i = j
        elif c == 'R' and text.startswith('R"', i):
            m = re.match(r'R"([^(]*)\(', text[i:])
            if m:
                end = text.find(')' + m.group(1) + '"', i)
                end = n if end < 0 else end + len(m.group(1)) + 2
                out.append('""' + '\n' * text.count('\n', i, end))
                i = end
            else:
                out.append(c)
                i += 1
        elif c == '"' or c == "'":
            j = i + 1
            while j < n and text[j] != c and text[j] != '\n':
                if text[j] == '\\':
                    j += 1
                j += 1
            out.append(c + c)
            i = j + 1
        else:
            out.append(c)
            i += 1
    return ''.join(out)


def walk(root):
    for dp, dn, fn in os.walk(root):
        dn[:] = [d for d in dn if d not in ('ThirdParty', 'Imgui', 'imgui')]
        for f in fn:
            if f.endswith(EXT):
                yield os.path.join(dp, f).replace('\\', '/')


# ── 엔진 기호표 ──────────────────────────────────────────────
RE_TYPE = re.compile(r'\b(class|struct)\s+(?:alignas\([^)]*\)\s*)?(?:[A-Z0-9_]+_API\s+)?([A-Za-z_]\w*)\s*(?:final\s*)?(?=[:{])')
RE_ENUM = re.compile(r'\benum\s+(?:class\s+|struct\s+)?([A-Za-z_]\w*)\s*(?::\s*[\w:]+\s*)?\{')
RE_DEFINE = re.compile(r'^[ \t]*#[ \t]*define[ \t]+([A-Za-z_]\w*)', re.M)
RE_USING = re.compile(r'\busing\s+([A-Za-z_]\w*)\s*=')
RE_TYPEDEF = re.compile(r'\btypedef\b[^;{]*?\b([A-Za-z_]\w*)\s*;')
RE_CALLNAME = re.compile(r'(~?[A-Za-z_]\w*)\s*\(')
KEYWORDS = set('if for while switch return sizeof alignof decltype static_assert catch new delete operator template typename using namespace public private protected virtual override const static inline explicit friend constexpr noexcept throw else do case default break continue goto class struct enum union typedef auto void bool char int float double long short unsigned signed extern mutable volatile this nullptr true false'.split())

symbols = {}          # 이름 → {kind, module, header}
methods = defaultdict(set)   # 멤버 함수 이름 → {(module, header, class)}
headers = {}          # 헤더 상대 경로(모듈 기준) → (module, 절대 경로)


def add_symbol(name, kind, module, header):
    if name in KEYWORDS or len(name) < 2:
        return
    s = symbols.setdefault(name, {'kind': kind, 'decl': []})
    if (module, header) not in s['decl']:
        s['decl'].append((module, header))


def scan_header(module, path, rel):
    text = strip_code(read(path))
    for m in RE_DEFINE.finditer(read(path)):
        add_symbol(m.group(1), 'macro', module, rel)
    for m in RE_TYPE.finditer(text):
        add_symbol(m.group(2), 'type', module, rel)
    for m in RE_ENUM.finditer(text):
        add_symbol(m.group(1), 'enum', module, rel)
    for m in RE_USING.finditer(text):
        add_symbol(m.group(1), 'alias', module, rel)
    for m in RE_TYPEDEF.finditer(text):
        add_symbol(m.group(1), 'alias', module, rel)
    # 중괄호 깊이로 클래스 몸 안 · 밖을 가른다
    stack = []
    i = 0
    pending = None
    for tok in re.finditer(r'\{|\}|;|\b(class|struct|enum|namespace|union)\b\s*(?:[A-Z0-9_]+_API\s+)?([A-Za-z_]\w*)?', text):
        t = tok.group(0)
        if t == '{':
            kind, name = (pending[0], pending[1]) if pending else ('other', None)
            stack.append((kind, name, tok.end()))
            pending = None
        elif t == '}':
            if stack:
                kind, name, start = stack.pop()
                body = text[start:tok.start()]
                if kind in ('class', 'struct'):
                    collect_methods(module, rel, name, body)
        elif t == ';':
            pending = None
        else:
            pending = (tok.group(1), tok.group(2), tok.end())
    # 맨 위(이름 공간 안)의 함수 선언: 이름 공간이 아닌 중괄호 안을 지운 글에서 찾는다
    for line in top_level(text).split('\n'):
        mm = re.match(r'^\s*(?:template\s*<[^>]*>\s*)?(?:inline\s+|static\s+|constexpr\s+)*(?:[A-Z0-9_]+_API\s+)?[\w:<>,\*& ]+?[\s\*&]+([A-Z][A-Za-z0-9_]*)\s*\([^;]*\)\s*(?:const\s*)?(?:;|\{|$)', line)
        if mm:
            add_symbol(mm.group(1), 'function', module, rel)


def top_level(text):
    out = []
    stack = []
    last = 0
    for m in re.finditer(r'[{};]', text):
        ch = m.group(0)
        seg = text[last:m.start()]
        inside = any(k != 'ns' for k in stack)
        out.append(re.sub(r'[^\n]', ' ', seg) if inside else seg)
        if ch == '{':
            head = seg.split(';')[-1].split('}')[-1]
            stack.append('ns' if re.search(r'\bnamespace\b', head) or re.search(r'extern\s*""\s*$', head.strip()) else 'x')
            out.append(' ' if inside else '{')
        elif ch == '}':
            if stack:
                stack.pop()
            out.append(' ' if any(k != 'ns' for k in stack) else '}')
        else:
            out.append(' ' if inside else ';')
        last = m.end()
    out.append(text[last:])
    return ''.join(out)


def collect_methods(module, rel, cls, body):
    depth = 0
    flat = []
    for ch in body:
        if ch == '{':
            depth += 1
            flat.append(' ')
        elif ch == '}':
            depth -= 1
            flat.append(';')
        elif depth == 0:
            flat.append(ch)
    for m in RE_CALLNAME.finditer(''.join(flat)):
        name = m.group(1).lstrip('~')
        if name in KEYWORDS or name == cls or len(name) < 3:
            continue
        methods[name].add((module, rel, cls))


for module, sub in MODULES.items():
    root = ENGINE + '/' + sub
    for path in walk(root):
        rel = os.path.relpath(path, root).replace('\\', '/')
        if path.endswith(('.h', '.hpp', '.inl')):
            headers[rel] = (module, path)
            scan_header(module, path, rel)

# ── 게임 쪽 쓰임 ─────────────────────────────────────────────
RE_INCLUDE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.M)
RE_IDENT = re.compile(r'[A-Za-z_]\w*')
RE_MEMBERCALL = re.compile(r'(\.|->|::)\s*(~?[A-Za-z_]\w*)\s*(?:<[^;()]*?>)?\s*\(')
RE_QUAL = re.compile(r'\b([A-Za-z_]\w*)\s*::\s*([A-Za-z_]\w*)')
RE_OVERRIDE = re.compile(r'\b([A-Za-z_]\w*)\s*\([^;{)]*\)\s*(?:const\s*)?override\b')

game_decl = set()
game_methods = set()
game_classes = []
game_files = {}
for game, roots in GAMES.items():
    for root in roots:
        if not os.path.isdir(root):
            continue
        for path in walk(root):
            game_files[path] = game
            t = strip_code(read(path))
            for m in RE_TYPE.finditer(t):
                game_decl.add(m.group(2))
            # 게임 쪽이 스스로 선언 · 정의한 함수 이름(같은 이름 엔진 멤버 호출과 섞일 수 있음을 표시하려고)
            for m in re.finditer(r'\b([A-Za-z_]\w*)\s*::\s*~?([A-Za-z_]\w*)\s*\([^;{]*\)\s*(?:const\s*)?\{', t):
                game_methods.add(m.group(2))
            for m in re.finditer(r'^[ \t]*(?:virtual\s+|static\s+|inline\s+)*[\w:<>,\*& ]+?[\s\*&]+([A-Za-z_]\w*)\s*\([^;{]*\)\s*(?:const\s*)?(?:override\s*)?(?:;|\{)', t, re.M):
                game_methods.add(m.group(1))
            # 게임 클래스 → 부모 클래스, 그 몸에서 override 한 함수
            for m in re.finditer(r'\b(?:class|struct)\s+(?:[A-Z0-9_]+_API\s+)?([A-Za-z_]\w*)\s*(?:final\s*)?:\s*([^{;]+)\{', t):
                bases = [re.sub(r'^(public|protected|private|virtual)\s+', '', b.strip()).split('<')[0].strip() for b in m.group(2).split(',')]
                depth = 1
                j = m.end()
                while j < len(t) and depth:
                    if t[j] == '{':
                        depth += 1
                    elif t[j] == '}':
                        depth -= 1
                    j += 1
                body = t[m.end():j]
                ov = sorted(set(re.findall(r'\b([A-Za-z_]\w*)\s*\([^;{)]*\)\s*(?:const\s*)?override\b', body)))
                game_classes.append({'game': game, 'class': m.group(1), 'bases': bases, 'overrides': ov, 'file': path})

inc = defaultdict(lambda: {'files': set(), 'games': defaultdict(int)})
inc_unresolved = defaultdict(set)
use = defaultdict(lambda: {'count': 0, 'files': set(), 'games': defaultdict(int)})
mcall = defaultdict(lambda: {'count': 0, 'files': set(), 'games': defaultdict(int)})
qual = defaultdict(lambda: {'count': 0, 'files': set(), 'games': defaultdict(int)})
overrides = defaultdict(lambda: {'count': 0, 'files': set(), 'games': defaultdict(int)})
imgui = defaultdict(lambda: {'count': 0, 'files': set()})

for path, game in game_files.items():
    raw = read(path)
    here = os.path.dirname(path)
    roots = GAMES[game]
    for m in RE_INCLUDE.finditer(raw):
        name = m.group(1)
        if os.path.exists(os.path.join(here, name)) or any(os.path.exists(os.path.join(r, name)) for r in roots):
            continue
        if name.endswith('.generation.h'):
            continue
        if name in headers:
            inc[name]['files'].add(path)
            inc[name]['games'][game] += 1
        elif name == 'PCH/PCH.h':
            inc['PCH/PCH.h']['files'].add(path)
            inc['PCH/PCH.h']['games'][game] += 1
        elif '.' in name and not name.startswith(('windows', 'Windows')) and '/' in name or name.endswith('.h'):
            if not re.match(r'^(windows|winsock|ws2|d3d|dxgi|shlobj|commdlg|mmsystem|psapi|imm)', name, re.I):
                inc_unresolved[name].add(path)
    text = strip_code(raw)
    for m in RE_IDENT.finditer(text):
        n = m.group(0)
        if n in symbols:
            u = use[n]
            u['count'] += 1
            u['files'].add(path)
            u['games'][game] += 1
    for m in RE_MEMBERCALL.finditer(text):
        n = m.group(2).lstrip('~')
        if n in methods and n not in symbols:
            u = mcall[n]
            u['count'] += 1
            u['files'].add(path)
            u['games'][game] += 1
    for m in RE_QUAL.finditer(text):
        cls, fn = m.group(1), m.group(2)
        if cls in symbols and symbols[cls]['kind'] in ('type', 'enum'):
            u = qual[cls + '::' + fn]
            u['count'] += 1
            u['files'].add(path)
            u['games'][game] += 1
        if cls == 'ImGui':
            imgui[fn]['count'] += 1
            imgui[fn]['files'].add(path)
    for m in RE_OVERRIDE.finditer(text):
        n = m.group(1)
        if n in methods:
            u = overrides[n]
            u['count'] += 1
            u['files'].add(path)
            u['games'][game] += 1


def rep(files):
    fs = sorted(files, key=lambda p: (len(p), p))
    return [p.replace('C:/Develop/ProjectAH/Source/', 'AH:').replace(ENGINE + '/', '') for p in fs[:2]]


result = {
    'headers': {k: {'module': headers[k][0] if k in headers else 'PCH', 'nfiles': len(v['files']), 'games': dict(v['games']), 'rep': rep(v['files'])} for k, v in inc.items()},
    'unresolved': {k: rep(v) for k, v in inc_unresolved.items()},
    'symbols': {k: {'kind': symbols[k]['kind'], 'decl': symbols[k]['decl'], 'count': v['count'], 'nfiles': len(v['files']), 'games': dict(v['games']), 'rep': rep(v['files'])} for k, v in use.items()},
    'methods': {k: {'decl': sorted(methods[k], key=str), 'count': v['count'], 'nfiles': len(v['files']), 'games': dict(v['games']), 'rep': rep(v['files'])} for k, v in mcall.items()},
    'qualified': {k: {'count': v['count'], 'nfiles': len(v['files']), 'games': dict(v['games']), 'rep': rep(v['files'])} for k, v in qual.items()},
    'overrides': {k: {'decl': sorted(methods[k], key=str), 'count': v['count'], 'nfiles': len(v['files']), 'games': dict(v['games'])} for k, v in overrides.items()},
    'imgui': {k: {'count': v['count'], 'nfiles': len(v['files'])} for k, v in imgui.items()},
    'engine_headers': {k: v[0] for k, v in headers.items()},
    'game_files': {g: sum(1 for p, gg in game_files.items() if gg == g) for g in GAMES},
    'game_decl': sorted(game_decl),
    'game_methods': sorted(game_methods),
    'game_classes': [dict(c, file=c['file'].replace('C:/Develop/ProjectAH/Source/', 'AH:').replace(ENGINE + '/', '')) for c in game_classes],
    'header_symbols': {h: sorted(n for n, s in symbols.items() if any(hh == h for mm, hh in s['decl'])) for h in headers},
}
with open(OUT, 'w', encoding='utf-8') as f:
    json.dump(result, f, ensure_ascii=False, indent=1)
print('engine headers', len(headers), 'symbols', len(symbols), 'method names', len(methods))
print('game files', result['game_files'])
print('used headers', len(inc), 'unresolved includes', len(inc_unresolved), 'used symbols', len(use), 'member calls', len(mcall), 'qualified', len(qual), 'overrides', len(overrides), 'imgui fns', len(imgui))
