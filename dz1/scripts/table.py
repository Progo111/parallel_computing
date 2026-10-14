"""Таблицы для отчёта из results/raw.csv (markdown на stdout)."""
import csv
import sys

rows = list(csv.DictReader(open(sys.argv[1])))
for r in rows:
    r["N"] = int(r["N"])
    r["min_ms"] = float(r["min_ms"])
    r["gbps"] = float(r["gbps"])

sizes = sorted({r["N"] for r in rows})
stages = ["E0", "E1", "E2", "E3", "E4"]


def best(stage, n, mode=None):
    """Лучший (по времени) вариант этапа при данном N."""
    cand = [r for r in rows if r["stage"] == stage and r["N"] == n
            and (mode is None or r["mode"] == mode)]
    return min(cand, key=lambda r: r["min_ms"])


def head(cols):
    print("| " + " | ".join(cols) + " |")
    print("|" + "---|" * len(cols))


print("## Лучшее время этапа, мс\n")
head(["Этап"] + [f"N = {n}" for n in sizes])
for st in stages:
    cells = []
    for n in sizes:
        b = best(st, n)
        cells.append(f"{b['min_ms']:.4f} ({b['tag']}, {b['mode']})")
    print("| " + " | ".join([st] + cells) + " |")

print("\n## Ускорение относительно E0 (лучшее время этапа / лучшее время E0)\n")
head(["Этап"] + [f"N = {n}" for n in sizes])
for st in stages:
    cells = [f"{best('E0', n)['min_ms'] / best(st, n)['min_ms']:.1f}x" for n in sizes]
    print("| " + " | ".join([st] + cells) + " |")

print("\n## Достигнутая пропускная способность, ГБ/с\n")
head(["Этап"] + [f"N = {n}" for n in sizes])
for st in stages:
    cells = [f"{best(st, n)['gbps']:.1f}" for n in sizes]
    print("| " + " | ".join([st] + cells) + " |")

for st in stages[1:]:
    tags = sorted({r["tag"] for r in rows if r["stage"] == st})
    if len(tags) < 2:
        continue
    print(f"\n## Подбор параметров, {st} (время слитого прохода, мс)\n")
    head(["Вариант", "Ключи"] + [f"N = {n}" for n in sizes])
    for tag in tags:
        by_n = {r["N"]: r for r in rows
                if r["tag"] == tag and r["mode"] == "fused"}
        flags = next(r["flags"] for r in rows if r["tag"] == tag)
        cells = [f"{by_n[n]['min_ms']:.4f}" for n in sizes]
        print("| `" + tag + "` | `" + flags + "` | " + " | ".join(cells) + " |")

print("\n## Раздельные проходы против слитого (лучший вариант, мс)\n")
head(["N", "sep", "fused", "выигрыш"])
for n in sizes:
    sep = min(best(st, n, "sep")["min_ms"] for st in stages[1:])
    fus = min(best(st, n, "fused")["min_ms"] for st in stages[1:])
    print(f"| {n} | {sep:.4f} | {fus:.4f} | {sep / fus:.2f}x |")
