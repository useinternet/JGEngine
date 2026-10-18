// 위키 그림 그리개 — SVG 그림(2026-10-17 엔진 총괄, 사용자 결정 "SVG 로 직접 그리기")
//   node render.js              specs/<문서키>/*.js 를 모두 그려 그 문서 폴더의 그림/<file>.svg 로(문서 목록 = pages.js)
//   node render.js Core/06 Asset 문서키 · 번호로 고르기
// 원본(spec) 한 장 = module.exports = { file, type: 'flow' | 'seq' | 'class', ... } — 아래 그리는 함수 머리 주석 참고.
// 모습(2026-10-17 21:17 사용자 피드백 "이런 UI 느낌으로 — 구도 바꾸지마", 참고 그림 SuddenAttack 전체 구조.svg):
//   흰 캔버스 · Segoe UI / 맑은 고딕 · 파스텔 채움 + 같은 색 테두리 · 같은 색 진한 제목 글 · 얇은 회색 선과 열린 화살촉 ·
//   화살표 글은 선 옆(세로 선이면 오른쪽, 가로 선이면 위) · 묶음 틀은 머리띠 없이 왼쪽 위 제목.
'use strict';
const fs = require('fs');
const path = require('path');

const SPECS = path.join(__dirname, 'specs');
const FONT = "'Segoe UI', 'Malgun Gothic', 'Apple SD Gothic Neo', sans-serif";

// 색 — [채움, 테두리, 제목 글, 본문 글]
const COLOR = {
  blue: ['#E6F1FB', '#85B7EB', '#0C447C', '#185FA5'],
  green: ['#E1F5EE', '#5DCAA5', '#085041', '#0F6E56'],
  yellow: ['#FAEEDA', '#EF9F27', '#633806', '#854F0B'],
  orange: ['#FAECE7', '#F0997B', '#712B13', '#993C1D'],
  red: ['#FCEBEB', '#E24B4A', '#791F1F', '#A32D2D'],
  purple: ['#EEEDFE', '#7F77DD', '#3C3489', '#534AB7'],
  gray: ['#F1EFE8', '#B4B2A9', '#2C2C2A', '#5F5E5A'],
  white: ['#FFFFFF', '#B4B2A9', '#2C2C2A', '#5F5E5A']
};
const col = (c) => COLOR[c] || COLOR.blue;
const LINE = '#888780';     // 화살표 선
const LABEL = '#5F5E5A';    // 화살표 글
const RED = '#E24B4A';      // 금지 · 실패 길
const MEMBER = '#444441';   // 클래스 멤버 글
const SW = 1;               // 상자 테두리 굵기
const W600 = ' font-weight="600"';

const esc = (s) => String(s).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');

// 점검: 그린 뒤 경고를 모은다(글 넘침 · 상자 겹침 · 화살표 글이 상자를 덮음 · 캔버스 밖)
const WARN = [];
const boxes = [];   // 지금 그림의 상자들 { id, x, y, w, h }
const CANVAS = { w: 0, h: 0 };
const LABELS = [];  // 지금 그림의 화살표 글 자리 — 글끼리 겹치는지 본다
const overlap = (a, b, pad) => a.x < b.x + b.w - (pad || 0) && b.x < a.x + a.w - (pad || 0) && a.y < b.y + b.h - (pad || 0) && b.y < a.y + a.h - (pad || 0);

// 글자 폭 어림: 한글 · 한자 ≈ 0.95em, 영문 · 숫자 ≈ 0.56em, 빈칸 ≈ 0.3em
function textWidth(s, size) {
  let w = 0;
  for (const ch of String(s)) {
    const c = ch.codePointAt(0);
    if (ch === ' ') {
      w += 0.3;
    } else if (c > 0x2e80) {
      w += 0.95;
    } else if (/[A-Z@MW]/.test(ch)) {
      w += 0.66;
    } else {
      w += 0.56;
    }
  }
  return w * size;
}
// 빈칸에서 줄 바꾸기(한 낱말이 너무 길면 그대로 둔다)
function wrap(text, maxW, size) {
  const words = String(text).split(' ');
  const lines = [];
  let cur = '';
  for (const word of words) {
    const t = cur ? cur + ' ' + word : word;
    if (cur && textWidth(t, size) > maxW) {
      lines.push(cur);
      cur = word;
    } else {
      cur = t;
    }
  }
  if (cur) {
    lines.push(cur);
  }
  return lines;
}

function defs() {
  const chevron = (id, c) => `<marker id="${id}" viewBox="0 0 10 10" refX="8" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse"><path d="M2,1 L8,5 L2,9" fill="none" stroke="${c}" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/></marker>`;
  return `<defs>
${chevron('arrow', LINE)}
${chevron('open', LINE)}
${chevron('arrow-red', RED)}
${chevron('open-red', RED)}
<marker id="triangle" viewBox="0 0 12 12" refX="11" refY="6" markerWidth="11" markerHeight="11" orient="auto-start-reverse"><path d="M1,1 L11,6 L1,11 z" fill="#ffffff" stroke="${LINE}" stroke-width="1.2" stroke-linejoin="round"/></marker>
<marker id="diamond" viewBox="0 0 16 10" refX="15.5" refY="5" markerWidth="14" markerHeight="9" orient="auto-start-reverse"><path d="M0.5,5 L8,0.5 L15.5,5 L8,9.5 z" fill="#ffffff" stroke="${LINE}" stroke-width="1.2"/></marker>
<marker id="diamondFill" viewBox="0 0 16 10" refX="15.5" refY="5" markerWidth="14" markerHeight="9" orient="auto-start-reverse"><path d="M0.5,5 L8,0.5 L15.5,5 L8,9.5 z" fill="${LINE}"/></marker>
</defs>`;
}

// 여러 줄 글. anchor 'start' 면 x 에서 왼쪽 맞춤, 아니면 cx 가운데 맞춤
function textBlock(cx, top, rows, anchor, x0) {
  let y = top;
  const out = [];
  for (const r of rows) {
    y += r.size + (r.gap || 4);
    const x = anchor === 'start' ? x0 : cx;
    out.push(`<text x="${x}" y="${y}" text-anchor="${anchor || 'middle'}" font-size="${r.size}"${r.bold ? W600 : ''} fill="${r.color}">${esc(r.text)}</text>`);
  }
  return out.join('\n');
}
function rowsFor(node, innerW, ink, ink2) {
  const rows = [];
  if (node.title) {
    for (const l of wrap(node.title, innerW, 13)) {
      rows.push({ text: l, size: 13, bold: true, color: ink, gap: 5 });
    }
  }
  for (const line of node.lines || []) {
    const size = node.small ? 11 : 11.5;
    for (const l of wrap(line, innerW, size)) {
      rows.push({ text: l, size, color: ink2, gap: 5 });
    }
  }
  return rows;
}
const rowsHeight = (rows) => rows.reduce((h, r) => h + r.size + (r.gap || 4), 0);

// ── flow ───────────────────────────────────────────────
// spec: { file, type:'flow', w, h,
//   groups: [{ x, y, w, h, label, color? }],                 묶음 틀(제목은 왼쪽 위, 위 28px 를 제목 자리로 비워 둔다)
//   nodes:  [{ id, x, y, w, h?, color, title?, lines?:[], shape?:'rect'|'note'|'pill'|'cyl'|'diamond', small?, align?:'start'|'middle' }],
//           h 를 빼면 글에 맞춤(diamond = 갈림길). 글 맞춤은 기본 가운데
//   edges:  [{ from, to, fs?, ts?, fo?, to_off?, via?:[[x,y]], points?:[[x,y]...], dash?, label?, lx?, ly?, lw?, lpos?:'r'|'l'|'t'|'b',
//              start?, end?:'arrow'|'open'|'triangle'|'diamond'|'diamondFill'|'none', color?:'red' }],
//           fs/ts = 나가는 · 들어가는 면 't' 'b' 'l' 'r'(빼면 위치로 정함), fo/to_off = 그 면 가운데에서 비킨 거리
//           글은 (lx, ly) 또는 가장 긴 구간 가운데 옆에 — 세로 구간이면 오른쪽(lpos 'l' 이면 왼쪽), 가로 구간이면 위('b' 면 아래)
//   texts:  [{ x, y, text, size?, anchor?, color?, bold? }] }
function nodeBox(n) {
  const [fill, stroke, ink, ink2] = col(n.color);
  const left = n.align === 'start';
  const innerW = n.shape === 'diamond' ? n.w * 0.58 : left ? n.w - 28 : n.w - 18;
  const rows = rowsFor(n, innerW, ink, ink2);
  if (!n.h) {
    n.h = Math.max(40, rowsHeight(rows) + 20);
  }
  if (rowsHeight(rows) + 12 > n.h) {
    WARN.push(`상자 ${n.id}: 글이 높이를 넘침(글 ${rowsHeight(rows)} + 12 > h ${n.h}) — h 를 키우거나 비우기`);
  }
  for (const r of rows) {
    if (textWidth(r.text, r.size) > (n.shape === 'diamond' ? n.w * 0.66 : left ? n.w - 24 : n.w - 8)) {
      WARN.push(`상자 ${n.id}: 한 줄이 폭을 넘침 "${r.text}" — w 를 키우거나 lines 를 나누기`);
    }
  }
  boxes.push({ id: n.id, x: n.x, y: n.y, w: n.w, h: n.h });
  const top = n.y + (n.h - rowsHeight(rows)) / 2 - 3;
  const st = `stroke="${stroke}" stroke-width="${SW}"`;
  let shape;
  if (n.shape === 'note') {
    const f = 12;
    shape = `<path d="M${n.x + 8},${n.y} H${n.x + n.w - f} L${n.x + n.w},${n.y + f} V${n.y + n.h - 8} Q${n.x + n.w},${n.y + n.h} ${n.x + n.w - 8},${n.y + n.h} H${n.x + 8} Q${n.x},${n.y + n.h} ${n.x},${n.y + n.h - 8} V${n.y + 8} Q${n.x},${n.y} ${n.x + 8},${n.y} Z" fill="${fill}" ${st}/><path d="M${n.x + n.w - f},${n.y} V${n.y + f} H${n.x + n.w}" fill="none" ${st}/>`;
  } else if (n.shape === 'diamond') {
    shape = `<path d="M${n.x + n.w / 2},${n.y} L${n.x + n.w},${n.y + n.h / 2} L${n.x + n.w / 2},${n.y + n.h} L${n.x},${n.y + n.h / 2} Z" fill="${fill}" ${st} stroke-linejoin="round"/>`;
  } else if (n.shape === 'pill') {
    shape = `<rect x="${n.x}" y="${n.y}" width="${n.w}" height="${n.h}" rx="${n.h / 2}" fill="${fill}" ${st}/>`;
  } else if (n.shape === 'cyl') {
    const e = 9;
    shape = `<path d="M${n.x},${n.y + e} V${n.y + n.h - e} A${n.w / 2},${e} 0 0 0 ${n.x + n.w},${n.y + n.h - e} V${n.y + e}" fill="${fill}" ${st}/><ellipse cx="${n.x + n.w / 2}" cy="${n.y + e}" rx="${n.w / 2}" ry="${e}" fill="${fill}" ${st}/>`;
  } else {
    shape = `<rect x="${n.x}" y="${n.y}" width="${n.w}" height="${n.h}" rx="8" fill="${fill}" ${st}/>`;
  }
  return shape + '\n' + textBlock(n.x + n.w / 2, top, rows, left ? 'start' : 'middle', n.x + 14);
}
function sideOf(a, b) {
  const ax = a.x + a.w / 2;
  const ay = a.y + a.h / 2;
  const bx = b.x + b.w / 2;
  const by = b.y + b.h / 2;
  const dx = bx - ax;
  const dy = by - ay;
  if (Math.abs(dy) >= Math.abs(dx)) {
    return dy > 0 ? ['b', 't'] : ['t', 'b'];
  }
  return dx > 0 ? ['r', 'l'] : ['l', 'r'];
}
function anchor(n, side, off) {
  off = off || 0;
  if (side === 't') {
    return [n.x + n.w / 2 + off, n.y];
  }
  if (side === 'b') {
    return [n.x + n.w / 2 + off, n.y + n.h];
  }
  if (side === 'l') {
    return [n.x, n.y + n.h / 2 + off];
  }
  return [n.x + n.w, n.y + n.h / 2 + off];
}
// 점들을 직각으로 잇는다(앞 구간이 세로면 세로 먼저)
function orthogonal(pts, firstVertical) {
  const out = [pts[0]];
  let vertical = firstVertical;
  for (let i = 1; i < pts.length; i++) {
    const [cx, cy] = out[out.length - 1];
    const [nx, ny] = pts[i];
    if (cx !== nx && cy !== ny) {
      out.push(vertical ? [cx, ny] : [nx, cy]);
      vertical = !vertical;
    } else {
      vertical = cx === nx;
    }
    out.push([nx, ny]);
  }
  return out;
}
function routeEdge(e, byId) {
  if (e.points) {
    return e.points;
  }
  const a = byId[e.from];
  const b = byId[e.to];
  if (!a || !b) {
    throw new Error(`edge ${e.from} → ${e.to}: 모르는 상자`);
  }
  const [dfs, dts] = sideOf(a, b);
  const fs_ = e.fs || dfs;
  const ts = e.ts || dts;
  const p0 = anchor(a, fs_, e.fo);
  const p1 = anchor(b, ts, e.to_off);
  const vStart = fs_ === 't' || fs_ === 'b';
  const vEnd = ts === 't' || ts === 'b';
  let mid = [];
  if (e.via) {
    mid = e.via;
  } else if (vStart && vEnd && p0[0] !== p1[0]) {
    const my = e.my !== undefined ? e.my : Math.round((p0[1] + p1[1]) / 2);
    mid = [[p0[0], my], [p1[0], my]];
  } else if (!vStart && !vEnd && p0[1] !== p1[1]) {
    const mx = e.mx !== undefined ? e.mx : Math.round((p0[0] + p1[0]) / 2);
    mid = [[mx, p0[1]], [mx, p1[1]]];
  }
  return orthogonal([p0].concat(mid, [p1]), vStart);
}
// 글 자리: (lx, ly) 를 주면 그 점, 아니면 가장 긴 구간 가운데. 그 점이 놓인 구간이 세로인지 가로인지도 돌려준다
function labelAt(pts, e) {
  let px;
  let py;
  let bi = 1;
  if (e.lx !== undefined) {
    px = e.lx;
    py = e.ly;
    let bestD = Infinity;
    for (let i = 1; i < pts.length; i++) {
      const [x1, y1] = pts[i - 1];
      const [x2, y2] = pts[i];
      const cx = Math.max(Math.min(px, Math.max(x1, x2)), Math.min(x1, x2));
      const cy = Math.max(Math.min(py, Math.max(y1, y2)), Math.min(y1, y2));
      const d = Math.abs(cx - px) + Math.abs(cy - py);
      if (d < bestD) {
        bestD = d;
        bi = i;
      }
    }
  } else {
    let best = 0;
    for (let i = 1; i < pts.length; i++) {
      const len = Math.abs(pts[i][0] - pts[i - 1][0]) + Math.abs(pts[i][1] - pts[i - 1][1]);
      if (len > best) {
        best = len;
        bi = i;
      }
    }
    px = (pts[bi][0] + pts[bi - 1][0]) / 2;
    py = (pts[bi][1] + pts[bi - 1][1]) / 2;
  }
  const vertical = pts[bi][0] === pts[bi - 1][0] && pts[bi][1] !== pts[bi - 1][1];
  return [px, py, vertical];
}
function edgeSvg(pts, e) {
  const d = 'M' + pts.map(p => p.join(',')).join(' L');
  const end = e.end === undefined ? 'arrow' : e.end;
  // color: 'red' = 금지 · 실패 길(빨간 선 · 빨간 화살촉)
  const red = e.color === 'red';
  const attrs = [`d="${d}"`, `fill="none"`, `stroke="${red ? RED : LINE}"`, `stroke-width="${red ? 1.6 : 1.2}"`, 'stroke-linejoin="round"'];
  if (e.dash) {
    attrs.push('stroke-dasharray="5 4"');
  }
  if (end && end !== 'none') {
    attrs.push(`marker-end="url(#${red && (end === 'arrow' || end === 'open') ? end + '-red' : end})"`);
  }
  if (e.start && e.start !== 'none') {
    attrs.push(`marker-start="url(#${e.start})"`);
  }
  let s = `<path ${attrs.join(' ')}/>`;
  if (e.label) {
    const [lx, ly, vertical] = labelAt(pts, e);
    const size = 11;
    const lh = 14;
    const lines = wrap(e.label, e.lw || 170, size);
    const tw = Math.max(...lines.map(l => textWidth(l, size)));
    const n = lines.length;
    const pos = e.lpos || (vertical ? 'r' : 't');
    let lr;
    let tx;
    let anchorAttr;
    if (pos === 'r' || pos === 'l') {
      lr = { x: pos === 'r' ? lx + 6 : lx - 6 - tw - 4, y: ly - (n * lh) / 2, w: tw + 4, h: n * lh };
      tx = pos === 'r' ? lx + 8 : lx - 8;
      anchorAttr = pos === 'r' ? 'start' : 'end';
    } else {
      lr = { x: lx - tw / 2 - 2, y: pos === 't' ? ly - 5 - n * lh : ly + 5, w: tw + 4, h: n * lh };
      tx = lx;
      anchorAttr = 'middle';
    }
    for (const b of boxes) {
      if (overlap(lr, b, 2)) {
        WARN.push(`화살표 ${e.from || '?'} → ${e.to || '?'} 글 "${e.label}" 이 상자 ${b.id} 를 덮음 — lx/ly · lpos · lw 나 상자 자리를 옮기기`);
      }
    }
    // 글끼리: 실제 글꼴은 어림보다 넓게 그려질 수 있어 좌우 12px 여유를 두고 본다
    for (const o of LABELS) {
      if (overlap({ x: lr.x - 12, y: lr.y, w: lr.w + 24, h: lr.h }, o.r, 0)) {
        WARN.push(`화살표 글 "${e.label}" 와 "${o.label}" 가 겹침 — lpos · lx/ly 로 떼어 놓기`);
      }
    }
    LABELS.push({ r: lr, label: e.label });
    if (lr.x < 0 || lr.y < 0 || lr.x + lr.w > CANVAS.w || lr.y + lr.h > CANVAS.h) {
      WARN.push(`화살표 ${e.from || '?'} → ${e.to || '?'} 글 "${e.label}" 이 캔버스 밖으로 나감 — lpos · lw 나 캔버스 크기`);
    }
    lines.forEach((l, i) => {
      const y = lr.y + 11 + i * lh;
      s += `\n<text x="${tx}" y="${y.toFixed(1)}" text-anchor="${anchorAttr}" font-size="${size}" fill="${red ? '#A32D2D' : LABEL}" stroke="#ffffff" stroke-width="3" stroke-linejoin="round" paint-order="stroke">${esc(l)}</text>`;
    });
  }
  return s;
}
// 묶음 틀 — 파스텔 바탕, 왼쪽 위 제목(머리띠 없음).
// 제목은 화살표 · 상자 위에 그리고 바탕색 테두리를 둘러, 선이 지나가도 글이 가려지지 않게 한다(2026-10-17 사용자 피드백 "글씨가 선에 가려져")
const GROUP_TITLES = [];   // 지금 그림의 묶음 제목 자리 — 선이 지나가는지 본다
function groupSvgs(spec) {
  const frames = [];
  const titles = [];
  for (const g of spec.groups || []) {
    const [fill, stroke, ink] = g.color ? col(g.color) : ['#FAF9F6', '#D3D1C7', '#2C2C2A'];
    frames.push(`<rect x="${g.x}" y="${g.y}" width="${g.w}" height="${g.h}" rx="12" fill="${fill}" stroke="${stroke}" stroke-width="${SW}"${g.dash ? ' stroke-dasharray="5 4"' : ''}/>`);
    if (g.label) {
      titles.push(`<text x="${g.x + 14}" y="${g.y + 21}" text-anchor="start" font-size="13"${W600} fill="${ink}" stroke="${fill}" stroke-width="5" stroke-linejoin="round" paint-order="stroke">${esc(g.label)}</text>`);
      const tw = textWidth(g.label, 13);
      GROUP_TITLES.push({ label: g.label, r: { x: g.x + 10, y: g.y + 6, w: tw + 8, h: 20 } });
      if (tw > g.w - 22) {
        WARN.push(`묶음 "${g.label}": 제목이 폭을 넘침`);
      }
    }
  }
  return { frames, titles };
}
// 화살표 선이 묶음 제목을 지나가는지(가로 · 세로 구간과 제목 사각형이 만나는지)
function checkLineOverTitles(pts, e) {
  for (let i = 1; i < pts.length; i++) {
    const [x1, y1] = pts[i - 1];
    const [x2, y2] = pts[i];
    const seg = { x: Math.min(x1, x2) - 1, y: Math.min(y1, y2) - 1, w: Math.abs(x2 - x1) + 2, h: Math.abs(y2 - y1) + 2 };
    for (const g of GROUP_TITLES) {
      if (overlap(seg, g.r, 0)) {
        WARN.push(`화살표 ${e.from || '?'} → ${e.to || '?'} 선이 묶음 제목 "${g.label}" 을 지나감 — 선 자리(fo · mx · points)나 묶음 제목 위치를 옮기기`);
      }
    }
  }
}
const textSvgs = (spec) => (spec.texts || []).map(t => `<text x="${t.x}" y="${t.y}" text-anchor="${t.anchor || 'middle'}" font-size="${t.size || 12}"${t.bold ? W600 : ''} fill="${t.color || LABEL}">${esc(t.text)}</text>`);

function renderFlow(spec) {
  const byId = {};
  const nodeSvgs = [];
  for (const n of spec.nodes || []) {
    nodeSvgs.push(nodeBox(n));
    byId[n.id] = n;
  }
  checkBoxes(spec);
  const g = groupSvgs(spec);
  const edgeSvgs = (spec.edges || []).map(e => { const pts = routeEdge(e, byId); checkLineOverTitles(pts, e); return edgeSvg(pts, e); });
  return frame(spec.w, spec.h, g.frames.concat(edgeSvgs, nodeSvgs, g.titles, textSvgs(spec)).join('\n'), spec);
}

// ── seq ────────────────────────────────────────────────
// spec: { file, type:'seq', participants: [{ id, label, sub?, color? }], steps: [   label = 클래스 이름, sub = 한글 설명(작게)
//   { from, to, text, dash? }            메시지(from === to 면 자기 자신)
//   { note: 'text', over: [id] | [id1, id2], color? }   메모(여러 참가자 위에 걸치기, color: 'red' 등)
//   { gap: 12 }                           빈 줄 ] , colW?, w? }
function renderSeq(spec) {
  const P = spec.participants;
  const colW = spec.colW || Math.max(150, ...P.map(p => textWidth(p.label, 13) + 46));
  const left = 30;
  const x = {};
  P.forEach((p, i) => { x[p.id] = left + colW / 2 + i * colW; });
  const W = spec.w || left * 2 + colW * P.length;
  // 참가자 머리: 클래스 이름(위) + sub 한글 설명(아래, 작게)
  const hasSub = P.some(p => p.sub);
  const headH = hasSub ? 50 : 40;
  let y = 20 + headH + 26;
  const body = [];
  const size = 11.5;
  for (const s of spec.steps) {
    if (s.gap) {
      y += s.gap;
      continue;
    }
    if (s.note !== undefined) {
      const xs = s.over.map(id => x[id]);
      const minX = Math.min(...xs) - colW / 2 + 14;
      const maxX = Math.max(...xs) + colW / 2 - 14;
      const w = Math.max(maxX - minX, 150);
      const cx = (minX + maxX) / 2;
      const lines = wrap(s.note, w - 16, size);
      const h = lines.length * 16 + 12;
      const [nf, ns, , n2] = col(s.color || 'yellow');
      const x0 = cx - w / 2;
      body.push(`<path d="M${x0 + 6},${y} H${cx + w / 2 - 10} L${cx + w / 2},${y + 10} V${y + h - 6} Q${cx + w / 2},${y + h} ${cx + w / 2 - 6},${y + h} H${x0 + 6} Q${x0},${y + h} ${x0},${y + h - 6} V${y + 6} Q${x0},${y} ${x0 + 6},${y} Z" fill="${nf}" stroke="${ns}" stroke-width="${SW}"/>`);
      lines.forEach((l, i) => body.push(`<text x="${cx}" y="${y + 18 + i * 16}" text-anchor="middle" font-size="${size}" fill="${n2}">${esc(l)}</text>`));
      y += h + 16;
      continue;
    }
    const x1 = x[s.from];
    const x2 = x[s.to];
    const dash = s.dash ? ' stroke-dasharray="5 4"' : '';
    if (s.from === s.to) {
      const lines = wrap(s.text, colW - 20, size);
      lines.forEach((l, i) => body.push(`<text x="${x1 + 10}" y="${y - 6 + i * 15 - (lines.length - 1) * 15}" text-anchor="start" font-size="${size}" fill="${LABEL}" stroke="#ffffff" stroke-width="3" stroke-linejoin="round" paint-order="stroke">${esc(l)}</text>`));
      body.push(`<path d="M${x1},${y} H${x1 + 34} V${y + 22} H${x1 + 2}" fill="none" stroke="${LINE}" stroke-width="1.2" stroke-linejoin="round"${dash} marker-end="url(#arrow)"/>`);
      y += 22 + 36;
      continue;
    }
    const lines = wrap(s.text, Math.abs(x2 - x1) - 16, size);
    // 글 뒤에 흰 바탕 — 가운데를 지나는 다른 생명선이 글을 가르지 않게
    lines.forEach((l, i) => {
      const tw = textWidth(l, size) + 8;
      body.push(`<rect x="${((x1 + x2) / 2 - tw / 2).toFixed(1)}" y="${y - 19 - (lines.length - 1 - i) * 15}" width="${tw.toFixed(1)}" height="15" fill="#ffffff"/>`);
    });
    lines.forEach((l, i) => body.push(`<text x="${(x1 + x2) / 2}" y="${y - 7 - (lines.length - 1 - i) * 15}" text-anchor="middle" font-size="${size}" fill="${LABEL}">${esc(l)}</text>`));
    body.push(`<path d="M${x1},${y} H${x2 + (x2 > x1 ? -2 : 2)}" fill="none" stroke="${LINE}" stroke-width="1.2"${dash} marker-end="url(#${s.dash ? 'open' : 'arrow'})"/>`);
    y += 44;
  }
  const H = y + 10;
  const heads = [];
  const lifelines = [];
  for (const p of P) {
    const [fill, stroke, ink] = col(p.color);
    const w = Math.min(colW - 16, Math.max(110, textWidth(p.label, 13) + 28));
    if (textWidth(p.label, 13) > w - 8) {
      WARN.push(`참가자 ${p.id}: 이름이 칸을 넘침 — colW 를 키우기`);
    }
    heads.push(`<rect x="${x[p.id] - w / 2}" y="20" width="${w}" height="${headH}" rx="8" fill="${fill}" stroke="${stroke}" stroke-width="${SW}"/>`);
    if (p.sub) {
      heads.push(`<text x="${x[p.id]}" y="${20 + 21}" text-anchor="middle" font-size="13"${W600} fill="${ink}">${esc(p.label)}</text>`);
      heads.push(`<text x="${x[p.id]}" y="${20 + 38}" text-anchor="middle" font-size="11" fill="${col(p.color)[3]}">${esc(p.sub)}</text>`);
    } else {
      heads.push(`<text x="${x[p.id]}" y="${20 + headH / 2 + 5}" text-anchor="middle" font-size="13"${W600} fill="${ink}">${esc(p.label)}</text>`);
    }
    lifelines.push(`<path d="M${x[p.id]},${20 + headH} V${H - 6}" stroke="#C9C7BD" stroke-width="1" stroke-dasharray="4 4"/>`);
  }
  return frame(W, H, lifelines.concat(heads, body).join('\n'), spec);
}

// ── class ──────────────────────────────────────────────
// spec: { file, type:'class', w, h, groups?: (flow 와 같음), nodes?: (flow 와 같은 상자),
//   classes: [{ id, x, y, w, name, stereo?, members?:[], color?, stack?, badge? }], edges: (flow 와 같음 — start/end 에 triangle · diamond · open) }
//   stereo = 이름 아래 작은 한글 설명(역할 · 차례)
function renderClass(spec) {
  const byId = {};
  const parts = [];
  for (const c of spec.classes) {
    const [fill, stroke, ink, ink2] = col(c.color);
    const m = c.members || [];
    const head = c.stereo ? 44 : 30;
    c.h = head + (m.length ? m.length * 18 + 10 : 0);
    // stack = 뒤에 겹쳐 그릴 카드 수(같은 클래스 객체가 여러 개임을 보여 줌), badge = 머리 오른쪽 작은 글(예: '× 18')
    const stack = c.stack || 0;
    for (let k = stack; k >= 1; k--) {
      parts.push(`<rect x="${c.x + k * 6}" y="${c.y + k * 6}" width="${c.w}" height="${c.h}" rx="8" fill="${fill}" stroke="${stroke}" stroke-width="${SW}" opacity="${(1 - k * 0.25).toFixed(2)}"/>`);
    }
    parts.push(`<rect x="${c.x}" y="${c.y}" width="${c.w}" height="${c.h}" rx="8" fill="#ffffff" stroke="${stroke}" stroke-width="${SW}"/>`);
    parts.push(`<path d="M${c.x + 8},${c.y} H${c.x + c.w - 8} A8,8 0 0 1 ${c.x + c.w},${c.y + 8} V${c.y + head} H${c.x} V${c.y + 8} A8,8 0 0 1 ${c.x + 8},${c.y} Z" fill="${fill}" stroke="${stroke}" stroke-width="${SW}"/>`);
    // 클래스 이름이 위, 한글 설명(stereo)은 그 아래 작게(2026-10-17 사용자 피드백 "클래스이름 / 한글 설명")
    parts.push(`<text x="${c.x + c.w / 2}" y="${c.y + (c.stereo ? 20 : 20)}" text-anchor="middle" font-size="13"${W600} fill="${ink}">${esc(c.name)}</text>`);
    if (c.stereo) {
      parts.push(`<text x="${c.x + c.w / 2}" y="${c.y + 36}" text-anchor="middle" font-size="11" fill="${ink2}">${esc(c.stereo)}</text>`);
    }
    m.forEach((t, i) => parts.push(`<text x="${c.x + 12}" y="${c.y + head + 18 + i * 18}" text-anchor="start" font-size="11.5" fill="${MEMBER}">${esc(t)}</text>`));
    byId[c.id] = c;
    if (c.badge) {
      parts.push(`<text x="${c.x + c.w - 10}" y="${c.y + 20}" text-anchor="end" font-size="12"${W600} fill="#A32D2D">${esc(c.badge)}</text>`);
    }
    boxes.push({ id: c.id, x: c.x, y: c.y, w: c.w + stack * 6, h: c.h + stack * 6 });
    if (textWidth(c.name, 13) > c.w - 8 || (c.stereo && textWidth(c.stereo, 11) > c.w - 8) || m.some(t => textWidth(t, 11.5) > c.w - 20)) {
      WARN.push(`클래스 ${c.id}: 글이 폭을 넘침 — w 를 키우기`);
    }
  }
  const nodeSvgs = [];
  for (const n of spec.nodes || []) {
    nodeSvgs.push(nodeBox(n));
    byId[n.id] = n;
  }
  checkBoxes(spec);
  const g = groupSvgs(spec);
  const edgeSvgs = (spec.edges || []).map(e => { const pts = routeEdge(e, byId); checkLineOverTitles(pts, e); return edgeSvg(pts, e); });
  return frame(spec.w, spec.h, g.frames.concat(edgeSvgs, parts, nodeSvgs, g.titles, textSvgs(spec)).join('\n'), spec);
}

function checkBoxes(spec) {
  for (let i = 0; i < boxes.length; i++) {
    const a = boxes[i];
    if (a.x < 0 || a.y < 0 || a.x + a.w > spec.w || a.y + a.h > spec.h) {
      WARN.push(`상자 ${a.id}: 캔버스(${spec.w}×${spec.h}) 밖으로 나감`);
    }
    for (let j = i + 1; j < boxes.length; j++) {
      if (overlap(a, boxes[j], 0)) {
        WARN.push(`상자 ${a.id} 와 ${boxes[j].id} 가 겹침`);
      }
    }
  }
  for (const g of spec.groups || []) {
    if (g.x < 0 || g.y < 0 || g.x + g.w > spec.w || g.y + g.h > spec.h) {
      WARN.push(`묶음 "${g.label}": 캔버스 밖으로 나감`);
    }
  }
}

function frame(w, h, inner, spec) {
  return `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${w} ${h}" width="${w}" height="${h}" font-family="${esc(FONT)}" role="img" aria-label="${esc(spec.alt || spec.file)}">
${defs()}
<rect x="0" y="0" width="${w}" height="${h}" fill="#ffffff"/>
${inner}
</svg>
`;
}

function render(spec) {
  WARN.length = 0;
  boxes.length = 0;
  LABELS.length = 0;
  GROUP_TITLES.length = 0;
  CANVAS.w = spec.w || 0;
  CANVAS.h = spec.h || 0;
  if (spec.type === 'seq') {
    CANVAS.w = CANVAS.h = Infinity;
    return renderSeq(spec);
  }
  if (spec.type === 'class') {
    return renderClass(spec);
  }
  return renderFlow(spec);
}

module.exports = { render, textWidth, wrap };

if (require.main === module) {
  // node render.js                 모든 문서
  // node render.js Core            한 문서
  // node render.js Core/06 Asset   문서 · 번호로 고르기(번호는 파일 이름 앞부분)
  const PAGES = require('./pages.js');
  const args = process.argv.slice(2);
  let total = 0;
  let warned = 0;
  for (const key of Object.keys(PAGES)) {
    const dir = path.join(SPECS, key);
    if (!fs.existsSync(dir)) {
      continue;
    }
    const picks = args.filter(a => a === key || a.startsWith(key + '/')).map(a => a.slice(key.length + 1)).filter(Boolean);
    if (args.length && !args.some(a => a === key || a.startsWith(key + '/'))) {
      continue;
    }
    const out = process.env.WIKI_SVG_OUT || path.join(path.dirname(PAGES[key]), '그림');
    fs.mkdirSync(out, { recursive: true });
    const files = fs.readdirSync(dir).filter(f => f.endsWith('.js')).sort().filter(f => !picks.length || picks.some(o => f.startsWith(o)));
    for (const f of files) {
      delete require.cache[require.resolve(path.join(dir, f))];
      const spec = require(path.join(dir, f));
      const svg = render(spec);
      fs.writeFileSync(path.join(out, spec.file + '.svg'), svg, 'utf8');
      total++;
      if (WARN.length) {
        warned++;
      }
      console.log('그림', key + '/' + spec.file + '.svg', (svg.length / 1024).toFixed(1) + 'KB', WARN.length ? `— 경고 ${WARN.length}` : '— 점검 통과');
      WARN.forEach(w => console.log('   ! ' + w));
    }
  }
  console.log(`모두 ${total}장 · 경고 있는 그림 ${warned}장`);
}
