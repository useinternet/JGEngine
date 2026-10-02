# 빌드 로그(MSBuild normal + cl /Bt+ + link /time) 집계.
# 사용: python analyze.py <log> [<log> ...]
import re
import sys
from collections import defaultdict

TIME_RE = re.compile(r"time\((?P<tool>[^)]*\\(?P<dll>c1xx|c2)\.dll)\)=(?P<sec>[0-9.]+)s < (?P<a>\d+) - (?P<b>\d+) >.*\[(?P<path>[^\]]+)\]")
LINK_RE = re.compile(r"^\s*Final: Total time = (?P<sec>[0-9.]+)s")
LIB_RE = re.compile(r"Creating library .*\\(?P<name>[A-Za-z0-9_]+)\.lib")
PROJ_PERF_RE = re.compile(r"^\s+(?P<ms>\d+) ms\s+(?P<proj>\S+\.(vcxproj|sln|metaproj))\s+(?P<calls>\d+) calls")
TASK_PERF_RE = re.compile(r"^\s+(?P<ms>\d+) ms\s+(?P<task>[A-Za-z_]+)\s+(?P<calls>\d+) calls")
HOST_RE = re.compile(r"\\bin\\(Host[A-Za-z0-9]+)\\")


def module_of(path):
    p = path.replace("/", "\\")
    for key in ("\\Source\\Runtime\\", "\\Source\\Editor\\", "\\Source\\Programs\\", "\\Temp\\CodeGen\\"):
        i = p.find(key)
        if i >= 0:
            rest = p[i + len(key):]
            mod = rest.split("\\", 1)[0]
            if key == "\\Temp\\CodeGen\\":
                return mod + " (codegen)"
            return mod
    if p.endswith("\\Source\\PCH\\PCH.cpp"):
        return "PCH.cpp (x projects)"
    return "other"


def analyze(log_path):
    files = {}  # key: (path, a) -> rec
    fe = {}
    be = {}
    links = []
    host = set()
    proj_perf = []
    task_perf = []
    section = None
    with open(log_path, encoding="utf-8", errors="replace") as f:
        for line in f:
            m = TIME_RE.search(line)
            if m:
                host.update(HOST_RE.findall(m.group("tool")))
                path = m.group("path")
                rec = (float(m.group("sec")), int(m.group("a")), int(m.group("b")))
                (fe if m.group("dll") == "c1xx" else be).setdefault(path, []).append(rec)
                continue
            m = LINK_RE.match(line)
            if m:
                links.append(float(m.group("sec")))
                continue
            if "Project Performance Summary:" in line:
                section = "proj"
                continue
            if "Task Performance Summary:" in line:
                section = "task"
                continue
            if "Target Performance Summary:" in line:
                section = "target"
                continue
            if section == "proj":
                m = PROJ_PERF_RE.match(line)
                if m:
                    proj_perf.append((int(m.group("ms")), m.group("proj").split("\\")[-1], int(m.group("calls"))))
            elif section == "task":
                m = TASK_PERF_RE.match(line)
                if m:
                    task_perf.append((int(m.group("ms")), m.group("task"), int(m.group("calls"))))

    per_mod = defaultdict(lambda: {"n": 0, "fe": 0.0, "be": 0.0, "max": 0.0, "maxfile": "", "start": None, "end": None})
    intervals = []
    tu = []
    for path, recs in fe.items():
        for sec, a, b in recs:
            mod = module_of(path)
            d = per_mod[mod]
            d["n"] += 1
            d["fe"] += sec
            intervals.append((a, b))
            d["start"] = a if d["start"] is None else min(d["start"], a)
            d["end"] = b if d["end"] is None else max(d["end"], b)
    for path, recs in be.items():
        for sec, a, b in recs:
            mod = module_of(path)
            d = per_mod[mod]
            d["be"] += sec
            intervals.append((a, b))
            d["end"] = b if d["end"] is None else max(d["end"], b)
    for path in set(fe) | set(be):
        t_fe = sum(r[0] for r in fe.get(path, []))
        t_be = sum(r[0] for r in be.get(path, []))
        n = max(len(fe.get(path, [])), 1)
        tu.append((t_fe + t_be, t_fe, t_be, n, path))
    for path, rec in [(p, r) for p, rs in fe.items() for r in rs]:
        pass
    for mod, d in per_mod.items():
        pass
    # per TU max
    for total, t_fe, t_be, n, path in tu:
        mod = module_of(path)
        if total / n > per_mod[mod]["max"]:
            per_mod[mod]["max"] = total / n
            per_mod[mod]["maxfile"] = path.split("\\")[-1]

    print(f"== {log_path}")
    print(f"compiler host: {sorted(host)}")
    total_fe = sum(d["fe"] for d in per_mod.values())
    total_be = sum(d["be"] for d in per_mod.values())
    n_tu = sum(d["n"] for d in per_mod.values())
    print(f"TU compiles: {n_tu}, front-end (c1xx) sum {total_fe:.1f}s, back-end (c2) sum {total_be:.1f}s, total {total_fe + total_be:.1f}s")
    if intervals:
        t0 = min(a for a, b in intervals)
        t1 = max(b for a, b in intervals)
        span = (t1 - t0) / 1e7
        busy = 0.0
        # concurrency profile: sweep
        ev = []
        for a, b in intervals:
            ev.append((a, 1))
            ev.append((b, -1))
        ev.sort()
        cur = 0
        last = ev[0][0]
        acc = defaultdict(float)
        for t, dlt in ev:
            acc[cur] += (t - last) / 1e7
            cur += dlt
            last = t
        busy = sum(k * v for k, v in acc.items())
        print(f"compile span {span:.1f}s, average concurrent compiles {busy / span:.2f}")
        hist = sorted(acc.items())
        print("time by number of concurrent compiles: " + ", ".join(f"{k}:{v:.0f}s" for k, v in hist if v >= 1.0))
    print()
    print(f"{'module':28s} {'TUs':>4s} {'c1xx s':>8s} {'c2 s':>7s} {'sum s':>7s} {'span s':>7s} {'max TU s':>8s}  slowest TU")
    for mod, d in sorted(per_mod.items(), key=lambda kv: -(kv[1]["fe"] + kv[1]["be"])):
        span = ((d["end"] - d["start"]) / 1e7) if d["start"] is not None else 0.0
        print(f"{mod:28s} {d['n']:4d} {d['fe']:8.1f} {d['be']:7.1f} {d['fe'] + d['be']:7.1f} {span:7.1f} {d['max']:8.2f}  {d['maxfile']}")
    print()
    print("slowest 15 TUs (c1xx + c2):")
    for total, t_fe, t_be, n, path in sorted(tu, reverse=True)[:15]:
        short = path.split("\\wtbs\\")[-1] if "\\wtbs\\" in path else path
        print(f"  {total:6.2f}s (fe {t_fe:5.2f} be {t_be:5.2f}, x{n})  {short}")
    print()
    if links:
        print(f"links: {len(links)}, sum {sum(links):.1f}s, max {max(links):.1f}s, values: " + ", ".join(f"{x:.1f}" for x in sorted(links, reverse=True)))
    if task_perf:
        print("task performance (top):")
        for ms, task, calls in sorted(task_perf, reverse=True)[:8]:
            print(f"  {ms / 1000:7.1f}s  {task} ({calls} calls)")
    if proj_perf:
        print("project performance (top):")
        for ms, proj, calls in sorted(proj_perf, reverse=True)[:16]:
            print(f"  {ms / 1000:7.1f}s  {proj}")
    print()


if __name__ == "__main__":
    for p in sys.argv[1:]:
        analyze(p)
