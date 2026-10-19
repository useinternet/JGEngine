// 대화 압축 감시(엔진 총괄용) — 전체 필수 지침 10 · CLAUDE.md 5절: 분야 세션이 대화 압축 3번(일 끝) · 4번(단계 끝)이 되면 교체.
// 엔진 세션 대화 기록(.jsonl)에서 세션 제목("customTitle")마다 가장 새 기록의 압축 수("isCompactSummary":true)를 세어,
// 3번 이상이 되거나 늘어날 때마다 한 줄을 낸다. 엔진 총괄이 Monitor 로 띄운다(30분마다 다시):
//   node C:/Develop/JGEngine/Document/Memory/tools/compaction_watch.js
//   node C:/Develop/JGEngine/Document/Memory/tools/compaction_watch.js --list   지금 압축 수만 보고 끝냄
// 켤 때는 지금 3번 이상인 세션을 한 번씩 알린다. 엔진 총괄 자신은 세지 않는다(프로젝트 총괄이 챙김).
// 보는 세션: 세션 구성표(C:/obsidian/ProjectJG/01. 운영/세션 구성.md)의 엔진 분야 세션 일곱 — 이름이 바뀌면 아래 목록도.
// 기록은 뒤에 덧붙기만 하므로 파일마다 읽은 자리를 기억해 두고 새로 붙은 부분만 읽는다(처음 한 번만 전부 읽음).
// 시험용: --dir=<기록 폴더> · --poll=<초>
'use strict';

const fs = require('fs');
const path = require('path');
const os = require('os');

const args = process.argv.slice(2);
const argOf = (name, def) =>
{
  const a = args.find(x => x.startsWith('--' + name + '='));
  return a ? a.slice(name.length + 3) : def;
};

const DIR = argOf('dir', path.join(os.homedir(), '.claude', 'projects', 'C--Develop-JGEngine'));
const WATCH = ['Core', 'GameFrameWorks', 'GUI', 'JGEditor', 'Graphics', 'DevOps', 'Server'];
const POLL_MS = Number(argOf('poll', '120')) * 1000;
const RECENT_MS = 2 * 86400 * 1000;   // 이틀 넘게 안 바뀐 기록은 읽지 않는다

const files = new Map();   // 기록 경로 → { off, title, n }
const last = new Map();    // 세션 이름 → 마지막으로 본 압축 수

function readNew(p, e, size)
{
  const fd = fs.openSync(p, 'r');
  try
  {
    const buf = Buffer.alloc(size - e.off);
    const got = fs.readSync(fd, buf, 0, buf.length, e.off);
    if (got <= 0)
    {
      return;
    }
    const end = buf.lastIndexOf(0x0a, got - 1);
    if (end < 0)
    {
      return;
    }
    e.off += end + 1;
    for (const line of buf.toString('utf8', 0, end + 1).split('\n'))
    {
      const m = /"customTitle":"([^"]*)"/.exec(line);
      if (m)
      {
        e.title = m[1];
      }
      if (line.includes('"isCompactSummary":true'))
      {
        e.n++;
      }
    }
  }
  finally
  {
    fs.closeSync(fd);
  }
}

// 세션 이름 → { m: 가장 새 기록의 수정 시각, n: 그 기록의 압축 수, file }
function scan()
{
  const best = new Map();
  let names = [];
  try
  {
    names = fs.readdirSync(DIR).filter(f => f.endsWith('.jsonl'));
  }
  catch (err)
  {
    return best;
  }
  const now = Date.now();
  for (const f of names)
  {
    const p = path.join(DIR, f);
    let st;
    try
    {
      st = fs.statSync(p);
    }
    catch (err)
    {
      continue;
    }
    if (now - st.mtimeMs > RECENT_MS)
    {
      continue;
    }
    let e = files.get(p);
    if (!e || st.size < e.off)
    {
      e = { off: 0, title: '', n: 0 };
      files.set(p, e);
    }
    if (st.size > e.off)
    {
      try
      {
        readNew(p, e, st.size);
      }
      catch (err)
      {
        // 못 읽으면 다음 차례에 다시
      }
    }
    if (!WATCH.includes(e.title))
    {
      continue;
    }
    const b = best.get(e.title);
    if (!b || st.mtimeMs > b.m)
    {
      best.set(e.title, { m: st.mtimeMs, n: e.n, file: f });
    }
  }
  return best;
}

const first = scan();
if (args.includes('--list'))
{
  for (const t of WATCH)
  {
    const b = first.get(t);
    console.log(b ? `${t}: 대화 압축 ${b.n}번 (${b.file})` : `${t}: 최근 이틀 기록 없음`);
  }
  process.exit(0);
}
for (const t of WATCH)
{
  const b = first.get(t);
  if (!b)
  {
    continue;
  }
  last.set(t, b.n);
  if (b.n >= 3)
  {
    console.log(`교체 대상: ${t} — 대화 압축 ${b.n}번(지금)`);
  }
}
setInterval(() =>
{
  const now = scan();
  for (const t of WATCH)
  {
    const b = now.get(t);
    if (!b)
    {
      continue;
    }
    const before = last.get(t) || 0;
    if (b.n >= 3 && b.n > before)
    {
      console.log(`교체 대상: ${t} — 대화 압축 ${b.n}번(3번 뒤 일 끝 · 4번 뒤 단계 끝)`);
    }
    last.set(t, b.n);
  }
}, POLL_MS);
