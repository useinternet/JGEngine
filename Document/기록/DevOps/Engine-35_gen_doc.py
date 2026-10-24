# Engine-35: api_usage.json → 모듈별 표(마크다운). 사람이 쓰는 머리 · 판단 절은 따로 붙인다.
# usage: python -X utf8 -I gen_doc.py <api_usage.json> <out md>
import json
import re
import sys
from collections import defaultdict

d = json.load(open(sys.argv[1], encoding='utf-8'))
OUT = sys.argv[2]

MODS = ['Core', 'Graphics', 'GameFrameWork', 'GUI', 'GameHost', 'JGEditor']
GAMES = ['ProjectAH', 'Template', 'AudioCheck', 'GameUIDemo', 'GfwView', 'NetDuel']
SHORT = {'ProjectAH': 'AH', 'Template': '틀', 'AudioCheck': 'Aud', 'GameUIDemo': 'UI', 'GfwView': 'GV', 'NetDuel': 'ND'}
NOISE = {'std', 'Name', 'Type', 'address', 'Next'}
STL = {'push_back', 'size', 'empty', 'end', 'begin', 'find', 'clear', 'erase', 'c_str', 'insert', 'emplace_back', 'back', 'front', 'at', 'data', 'reserve', 'resize', 'count', 'emplace', 'substr', 'length', 'append', 'pop_back', 'rbegin', 'rend', 'swap', 'first', 'second'}
CLS = re.compile(r'^[HPJGIE][A-Z]')
game_methods = set(d['game_methods'])


def games(g):
    return ' '.join('%s %d' % (SHORT[k], g[k]) for k in GAMES if g.get(k))


def rep(r):
    return ' · '.join('`%s`' % x for x in r[:1])


def mod_of(decl):
    return sorted(set(m for m, h in decl))


lines = []
w = lines.append

# 모듈 요약
summary = {}
for m in MODS:
    hs = [k for k, v in d['headers'].items() if v['module'] == m]
    sy = [k for k, v in d['symbols'].items() if k not in NOISE and m in mod_of(v['decl'])]
    summary[m] = (len(hs), len(sy))

# 모듈별 표
qual_by_mod = defaultdict(list)
for k, v in d['qualified'].items():
    cls = k.split('::')[0]
    if cls in d['symbols'] and cls not in NOISE:
        for m in mod_of(d['symbols'][cls]['decl']):
            qual_by_mod[m].append((v['count'], k, v))

meth_by_cls = defaultdict(list)
meth_multi = defaultdict(list)
syms = d['symbols']


def used_in(cls):
    s = syms.get(cls)
    return set(s['games']) if s else set()


for name, v in d['methods'].items():
    if name in STL:
        continue
    # 그 게임 코드가 이름을 쓰는 엔진 클래스에만 붙인다(이름만 같은 다른 모듈 클래스로 퍼지지 않게)
    owners = sorted(set((m, c) for m, h, c in v['decl'] if c and CLS.match(c) and used_in(c)))
    if not owners:
        continue
    gs = set()
    for m, c in owners:
        gs |= used_in(c)
    count = sum(n for g, n in v['games'].items() if g in gs)
    if count == 0:
        continue
    vv = dict(v, count=count, games={g: n for g, n in v['games'].items() if g in gs})
    clash = name in game_methods
    mods = sorted(set(m for m, c in owners))
    if len(owners) == 1:
        meth_by_cls[owners[0]].append((count, name, vv, clash))
    else:
        for m in mods:
            meth_multi[m].append((count, name, vv, [c for mm, c in owners if mm == m], clash))

# 재정의: 게임 클래스의 첫 엔진 부모 클래스로 묶는다
ovr_by_mod = defaultdict(list)
ovr = defaultdict(lambda: {'classes': [], 'methods': defaultdict(int)})
for gc in d['game_classes']:
    base = next((b for b in gc['bases'] if b in syms and syms[b]['kind'] == 'type'), None)
    if not base:
        continue
    o = ovr[base]
    o['classes'].append((gc['game'], gc['class']))
    for n in gc['overrides']:
        o['methods'][n] += 1
for base, o in ovr.items():
    for m in mod_of(syms[base]['decl']):
        ovr_by_mod[m].append((len(o['classes']), base, o))

hdr_used = defaultdict(set)
for k, v in d['symbols'].items():
    if k in NOISE:
        continue
    for m, h in v['decl']:
        hdr_used[m].add(h)
for k, v in d['headers'].items():
    hdr_used[v['module']].add(k)

for m in MODS:
    w('')
    w('## %s' % m)
    hs = sorted([(v['nfiles'], k, v) for k, v in d['headers'].items() if v['module'] == m], reverse=True)
    w('')
    w('### include 하는 헤더 (%d)' % len(hs))
    w('')
    if hs:
        w('| 헤더 | 포함한 파일 | 게임별 | 대표 파일 |')
        w('|---|---|---|---|')
        for n, k, v in hs:
            w('| `%s` | %d | %s | %s |' % (k, n, games(v['games']), rep(v['rep'])))
    else:
        w('없음.')
    sy = sorted([(v['count'], k, v) for k, v in d['symbols'].items() if k not in NOISE and m in mod_of(v['decl'])], reverse=True)
    w('')
    w('### 쓰는 타입 · 매크로 · 함수 (%d)' % len(sy))
    w('')
    if sy:
        w('| 이름 | 종류 | 선언 헤더 | 쓰임 | 파일 | 게임별 | 대표 파일 |')
        w('|---|---|---|---|---|---|---|')
        kinds = {'type': '타입', 'enum': '열거', 'alias': '별칭', 'macro': '매크로', 'function': '함수'}
        for c, k, v in sy:
            hdr = [h for mm, h in v['decl'] if mm == m][0]
            w('| `%s` | %s | `%s` | %d | %d | %s | %s |' % (k, kinds.get(v['kind'], v['kind']), hdr, c, v['nfiles'], games(v['games']), rep(v['rep'])))
    else:
        w('없음.')
    q = sorted(qual_by_mod.get(m, []), reverse=True)
    w('')
    w('### 정적 함수 · 열거 값 · 상수 (`클래스::이름`, %d)' % len(q))
    w('')
    if q:
        w('| 이름 | 쓰임 | 파일 | 게임별 |')
        w('|---|---|---|---|')
        for c, k, v in q:
            w('| `%s` | %d | %d | %s |' % (k, c, v['nfiles'], games(v['games'])))
    else:
        w('없음.')
    cls_rows = sorted([(sum(x[0] for x in rows), cls, rows) for (mm, cls), rows in meth_by_cls.items() if mm == m], reverse=True)
    w('')
    w('### 멤버 함수 — 이 모듈 클래스 하나에만 있는 이름 (클래스 %d)' % len(cls_rows))
    w('')
    if cls_rows:
        w('| 클래스 | 부르는 멤버 함수(쓰임) |')
        w('|---|---|')
        for tot, cls, rows in cls_rows:
            items = ', '.join('`%s`%s %d' % (n, '†' if clash else '', c) for c, n, v, clash in sorted(rows, reverse=True))
            w('| `%s` | %s |' % (cls, items))
    else:
        w('없음.')
    mm_rows = sorted(meth_multi.get(m, []), reverse=True)
    if mm_rows:
        w('')
        w('### 멤버 함수 — 여러 클래스에 같은 이름이 있는 것 (%d, 이름으로 센 값)' % len(mm_rows))
        w('')
        w('| 이름 | 쓰임 | 이 모듈에서 그 이름을 가진 클래스 |')
        w('|---|---|---|')
        for c, n, v, owners, clash in mm_rows[:60]:
            w('| `%s`%s | %d | %s |' % (n, '†' if clash else '', c, ', '.join('`%s`' % o for o in owners[:6]) + (' 외 %d' % (len(owners) - 6) if len(owners) > 6 else '')))
        if len(mm_rows) > 60:
            w('| … | | 쓰임이 적은 %d개 생략 |' % (len(mm_rows) - 60))
    o = sorted(ovr_by_mod.get(m, []), key=lambda x: (-x[0], x[1]))
    w('')
    w('### 게임 클래스가 상속하는 엔진 클래스 (%d)' % len(o))
    w('')
    if o:
        w('| 부모 클래스 | 상속한 게임 클래스 | 게임별 | 재정의하는 함수(클래스 수) | 대표 게임 클래스 |')
        w('|---|---|---|---|---|')
        for c, base, ob in o:
            per = defaultdict(int)
            for g, k in ob['classes']:
                per[g] += 1
            meths = ', '.join('`%s` %d' % (n, k) for n, k in sorted(ob['methods'].items(), key=lambda x: (-x[1], x[0]))) or '—'
            w('| `%s` | %d | %s | %s | `%s` |' % (base, c, games(per), meths, ob['classes'][0][1]))
    else:
        w('없음.')
    # 안 쓰는 헤더
    allh = sorted(h for h, mm in d['engine_headers'].items() if mm == m)
    unused = [h for h in allh if h not in hdr_used[m]]
    byfolder = defaultdict(list)
    for h in unused:
        byfolder[h.split('/')[0] if '/' in h else '(모듈 루트)'].append(h.split('/')[-1])
    w('')
    w('### 게임이 쓰지 않는 헤더 (%d / %d)' % (len(unused), len(allh)))
    w('')
    w('| 폴더 | 헤더 |')
    w('|---|---|')
    for f in sorted(byfolder):
        w('| `%s` | %s |' % (f, ', '.join('`%s`' % x for x in byfolder[f])))

with open(OUT, 'w', encoding='utf-8', newline='\n') as f:
    f.write('\n'.join(lines) + '\n')
print('summary', summary)
for m in MODS:
    allh = [h for h, mm in d['engine_headers'].items() if mm == m]
    print(m, 'headers', len(allh), 'used', len(hdr_used[m]), 'unused', len([h for h in allh if h not in hdr_used[m]]))
