// 위키 문서의 ```mermaid 블록을 순서대로 SVG 그림 줄로 바꾼다(원본 번호 NN = 문서 안 Mermaid 순서 00, 01, …)
//   node apply_wiki.js <문서키>          바꾸기(바꾸기 전 사본은 _backup/) — 문서키 = pages.js
//   node apply_wiki.js <문서키> --check  바꾸지 않고 짝만 보기
// 그림 원본 specs/<문서키>/NN_*.js 와 그린 그림(node render.js <문서키>)이 모두 있어야 바꾼다.
'use strict';
const fs = require('fs');
const path = require('path');
const PAGES = require('./pages.js');

const check = process.argv.includes('--check');
const key = process.argv.slice(2).find(a => !a.startsWith('--'));
if (!key || !PAGES[key]) {
  console.log('문서키를 주세요:', Object.keys(PAGES).join(', '));
  process.exit(1);
}
const WIKI = PAGES[key];
const IMG = path.join(path.dirname(WIKI), '그림');
const dir = path.join(__dirname, 'specs', key);
const specs = {};
for (const f of fs.existsSync(dir) ? fs.readdirSync(dir).filter(n => /^\d\d_.*\.js$/.test(n)) : []) {
  specs[f.slice(0, 2)] = require(path.join(dir, f));
}
const raw = fs.readFileSync(WIKI, 'utf8');
const crlf = raw.includes('\r\n');
let t = raw.replace(/\r\n/g, '\n');
let i = 0;
const missing = [];
t = t.replace(/```mermaid\n[\s\S]*?```/g, (block) => {
  const nn = String(i).padStart(2, '0');
  i++;
  const s = specs[nn];
  if (!s) {
    missing.push(nn);
    return block;
  }
  if (!fs.existsSync(path.join(IMG, s.file + '.svg'))) {
    missing.push(nn + '(svg 없음)');
    return block;
  }
  return `![${(s.alt || s.file).replace(/[[\]]/g, '')}](그림/${s.file}.svg)`;
});
console.log(key, '— Mermaid', i, '개 · 바꿀 수 있는 것', i - missing.length, missing.length ? '· 빠진 것 ' + missing.join(', ') : '');
if (check) {
  process.exit(0);
}
if (missing.length) {
  console.log('빠진 그림이 있어 바꾸지 않음');
  process.exit(1);
}
fs.mkdirSync(path.join(__dirname, '_backup'), { recursive: true });
fs.copyFileSync(WIKI, path.join(__dirname, '_backup', key + '_' + Date.now() + '.md'));
fs.writeFileSync(WIKI, crlf ? t.replace(/\n/g, '\r\n') : t, 'utf8');
console.log(key, '— 바꿈', i, '장');
