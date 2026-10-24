# AudioCheck verification (Audio module, D-37): compares the game's AUDIOCHECK step log with the loopback output level log.
# Usage: python analyze_audiocheck.py <game stdout or log> <audio_loopback_rms output>
# Prints one line per check and "AUDIOCHECK RESULT: PASS" (exit 0) or "FAIL" (exit 6).
# The loopback level is the real device output after the Windows volume, so only ratios and silence are compared.
import re
import sys


def load(game_path, loop_path):
    steps = {}
    for line in open(game_path, encoding="utf-8", errors="replace"):
        match = re.search(r"AUDIOCHECK (\d+) t=([\d.]+) (.+?) playing=(\d+)", line)
        if match:
            steps[match.group(3)] = (int(match.group(1)), int(match.group(4)))
    levels = []
    for line in open(loop_path, encoding="utf-8", errors="replace"):
        match = re.match(r"RMS (\d+) ([\d.]+) ([\d.]+)", line)
        if match:
            levels.append((int(match.group(1)), float(match.group(3))))
    return steps, levels


def main():
    steps, levels = load(sys.argv[1], sys.argv[2])
    needed = ["effect x8", "burst done", "music B", "music A", "music A steady", "music bus 0.25", "music bus 1.0", "rain on",
              "master 0", "master 1", "stop music and rain", "effects 0.5 and effect", "effects 1.0 and effect",
              "background mute", "background active", "background lower", "background active again", "done"]
    missing = [name for name in needed if name not in steps]
    if missing or not levels:
        print("AUDIOCHECK RESULT: FAIL - missing steps %s or no loopback levels" % missing)
        return 6

    def at(name, offset=0.0):
        return steps[name][0] + int(offset * 1000)

    def mean(begin, end):
        values = [value for (stamp, value) in levels if begin <= stamp < end]
        return sum(values) / len(values) if values else -1.0

    def energy(begin, end):
        # 구간 RMS 제곱의 합(= 에너지). 짧은 소리 전체가 구간 안에 있으면 50ms 구간 경계가 어디 걸려도 값이 같다
        return sum(value * value for (stamp, value) in levels if begin <= stamp < end)

    def bins(begin, end, size=250):
        return [mean(start, start + size) for start in range(begin, end - size + 1, size)]

    floor = mean(levels[0][0], at("effect x8", -0.2))
    threshold = max(floor * 10.0, 2e-4)
    results = []

    def check(condition, text, detail):
        results.append(condition)
        print("%s  %s  (%s)" % ("PASS" if condition else "FAIL", text, detail))

    print("floor before the game made sound: %.6f (threshold %.6f)" % (floor, threshold))
    if floor > 1e-4:
        print("WARN  other sound is playing on this PC - level checks may be off")

    # 짧은 효과음은 에너지로 비교한다(크기 비율 = 에너지 비율의 제곱근)
    burst_energy = energy(at("effect x8"), at("effect x8", 0.6))
    single_energy = energy(at("effects 1.0 and effect"), at("effects 1.0 and effect", 0.4))
    burst_ratio = (burst_energy / single_energy) ** 0.5 if single_energy > 0 else 0.0
    check(burst_energy > 0 and burst_ratio > 2.0 and steps["burst done"][1] >= 3,
          "(a) eight rapid effects overlap", "burst vs one effect level ratio %.2f (8 incoherent copies about 2.8), playing at burst end %d" % (burst_ratio, steps["burst done"][1]))

    music_b = mean(at("music B", 1.0), at("music A"))
    check(music_b > threshold, "(b) music B (ogg, streamed) is heard", "%.5f" % music_b)

    cross = bins(at("music A"), at("music A", 2.0))
    steady = mean(at("music A steady"), at("music bus 0.25"))
    check(min(cross) > threshold and steady > threshold, "(b) cross-fade B -> A has no silent gap and A loops",
          "cross-fade 250 ms bins min %.5f, steady A %.5f" % (min(cross), steady))

    lowered = mean(at("music bus 0.25", 0.05), at("music bus 1.0"))
    ratio = lowered / steady if steady > 0 else 0.0
    check(0.20 <= ratio <= 0.30, "(c) music bus 0.25 lowers the music to a quarter", "ratio %.3f" % ratio)

    silent = mean(at("master 0", 0.1), at("master 1"))
    back = mean(at("master 1", 0.1), at("stop music and rain"))
    check(silent < 1e-4 and back > threshold, "(c) master 0 silences everything, master 1 brings it back", "master 0 %.6f, master 1 %.5f" % (silent, back))

    first = mean(at("stop music and rain"), at("stop music and rain", 0.25))
    middle = mean(at("stop music and rain", 0.5), at("stop music and rain", 0.75))
    after = mean(at("stop music and rain", 1.1), at("effects 0.5 and effect"))
    check(first > middle > after and after < 1e-4, "(b) stop with a 1 s fade lowers and then stops", "%.5f > %.5f > %.6f" % (first, middle, after))

    half = energy(at("effects 0.5 and effect"), at("effects 0.5 and effect", 0.4))
    full = energy(at("effects 1.0 and effect"), at("effects 1.0 and effect", 0.4))
    effect_ratio = (half / full) ** 0.5 if full > 0 else 0.0
    check(0.45 <= effect_ratio <= 0.55, "(c) effects bus 0.5 halves an effect", "level ratio %.3f (from energy)" % effect_ratio)

    # 창이 뒤로 가면(WM_ACTIVATEAPP): 단계 사이가 2초 = 곡 A 한 주기라 같은 자리끼리 비교된다. 0.25초 페이드 + 장치 지연을 빼고 0.35초부터 잰다.
    muted = mean(at("background mute", 0.35), at("background active"))
    active = mean(at("background active", 0.35), at("background lower"))
    lower = mean(at("background lower", 0.35), at("background active again"))
    again = mean(at("background active again", 0.35), at("done"))
    check(muted < 1e-4 and active > threshold, "window inactive + Mute silences, active brings it back", "muted %.6f, active %.5f" % (muted, active))
    lower_ratio = lower / active if active > 0 else 0.0
    again_ratio = again / active if active > 0 else 0.0
    check(0.25 <= lower_ratio <= 0.35 and 0.9 <= again_ratio <= 1.1, "window inactive + Lower(0.3) lowers to 0.3, active restores",
          "lower ratio %.3f, again ratio %.3f" % (lower_ratio, again_ratio))

    passed = all(results)
    print("AUDIOCHECK RESULT: %s (%d/%d)" % ("PASS" if passed else "FAIL", sum(1 for r in results if r), len(results)))
    return 0 if passed else 6


if __name__ == "__main__":
    sys.exit(main())
