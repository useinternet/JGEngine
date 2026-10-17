// 위키 문서의 그림 줄 `![설명](그림/<file>.svg)` 설명글(alt)을 specs 의 alt 로 맞춘다(그림 글자를 고친 뒤)
//   node sync_alt.js               모든 문서(문서 목록 = pages.js)
//   node sync_alt.js Core Asset    고른 문서만
//   --check                        바꾸지 않고 다른 것만 보기
// 바꾸기 전 사본은 _backup/
'use strict';
const fs = require('fs');
const path = require('path');
const PAGES = require('./pages.js');

const check = process.argv.includes('--check');
const keys = process.argv.slice(2).filter(a => !a.startsWith('--'));
const esc = (s) => s.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');

for (const key of Object.keys(PAGES)) {
  const dir = path.join(__dirname, 'specs', key);
  if ((keys.length && !keys.includes(key)) || !fs.existsSync(dir)) {
    continue;
  }
  const WIKI = PAGES[key];
  let t = fs.readFileSync(WIKI, 'utf8');
  let changed = 0;
  const missing = [];
  for (const f of fs.readdirSync(dir).filter(n => /^\d\d_.*\.js$/.test(n)).sort()) {
    delete require.cache[require.resolve(path.join(dir, f))];
    const s = require(path.join(dir, f));
    const alt = (s.alt || s.file).replace(/[[\]]/g, '');
    const m = t.match(new RegExp('!\\[[^\\]]*\\]\\(그림/' + esc(s.file) + '\\.svg\\)'));
    if (!m) {
      missing.push(s.file);
      continue;
    }
    const line = `![${alt}](그림/${s.file}.svg)`;
    if (m[0] !== line) {
      changed++;
      if (check) {
        console.log('  다름', s.file);
      }
      t = t.replace(m[0], line);
    }
  }
  console.log(key, '— 그림 줄 바꿀 것', changed, missing.length ? '· 문서에 없는 그림 ' + missing.join(', ') : '');
  if (check || !changed) {
    continue;
  }
  fs.mkdirSync(path.join(__dirname, '_backup'), { recursive: true });
  fs.copyFileSync(WIKI, path.join(__dirname, '_backup', key + '_' + Date.now() + '.md'));
  fs.writeFileSync(WIKI, t, 'utf8');
  console.log(key, '— 맞춤');
}
