"""Графики к отчёту: время и пропускная способность от N (SVG)."""
import csv
import math
import os
import sys

raw, outdir = sys.argv[1], sys.argv[2]
rows = list(csv.DictReader(open(raw)))
for r in rows:
    r["N"] = int(r["N"])
    r["min_ms"] = float(r["min_ms"])
    r["gbps"] = float(r["gbps"])

sizes = sorted({r["N"] for r in rows})
stages = ["E0", "E1", "E2", "E3", "E4"]
colors = ["#888888", "#1f77b4", "#2ca02c", "#d62728", "#9467bd"]

W, H, L, R, T, B = 700, 420, 70, 140, 40, 55


def best(stage, n, field):
    """Значение у самого быстрого варианта этапа при данном N."""
    cand = [r for r in rows if r["stage"] == stage and r["N"] == n]
    return min(cand, key=lambda r: r["min_ms"])[field]


def chart(path, title, ylabel, field, logy):
    series = {st: [(n, best(st, n, field)) for n in sizes] for st in stages}
    vals = [v for s in series.values() for _, v in s]
    lo, hi = min(vals), max(vals)
    if logy:
        lo, hi = math.log10(lo), math.log10(hi)
    else:
        lo, hi = 0.0, hi * 1.1

    def x(n):
        a, b = math.log2(sizes[0]), math.log2(sizes[-1])
        return L + (W - L - R) * (math.log2(n) - a) / (b - a)

    def y(v):
        v = math.log10(v) if logy else v
        return H - B - (H - T - B) * (v - lo) / (hi - lo)

    out = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" '
           f'font-family="sans-serif" font-size="12">',
           f'<rect width="{W}" height="{H}" fill="white"/>',
           f'<text x="{W/2}" y="20" text-anchor="middle" font-size="14">{title}</text>',
           f'<line x1="{L}" y1="{H-B}" x2="{W-R}" y2="{H-B}" stroke="black"/>',
           f'<line x1="{L}" y1="{T}" x2="{L}" y2="{H-B}" stroke="black"/>']

    for n in sizes:
        out.append(f'<text x="{x(n):.1f}" y="{H-B+18}" text-anchor="middle">{n}</text>')
    out.append(f'<text x="{(L+W-R)/2:.1f}" y="{H-8}" text-anchor="middle">N</text>')

    for i in range(5):
        v = lo + (hi - lo) * i / 4
        yy = y(10 ** v if logy else v)
        label = f"{10 ** v:.3g}" if logy else f"{v:.0f}"
        out.append(f'<line x1="{L}" y1="{yy:.1f}" x2="{W-R}" y2="{yy:.1f}" '
                   f'stroke="#dddddd"/>')
        out.append(f'<text x="{L-8}" y="{yy+4:.1f}" text-anchor="end">{label}</text>')
    out.append(f'<text x="16" y="{(T+H-B)/2:.1f}" transform="rotate(-90 16 '
               f'{(T+H-B)/2:.1f})" text-anchor="middle">{ylabel}</text>')

    for st, color in zip(stages, colors):
        pts = " ".join(f"{x(n):.1f},{y(v):.1f}" for n, v in series[st])
        out.append(f'<polyline points="{pts}" fill="none" stroke="{color}" '
                   f'stroke-width="2"/>')
        for n, v in series[st]:
            out.append(f'<circle cx="{x(n):.1f}" cy="{y(v):.1f}" r="3" fill="{color}"/>')

    for i, (st, color) in enumerate(zip(stages, colors)):
        yy = T + 10 + i * 18
        out.append(f'<line x1="{W-R+10}" y1="{yy}" x2="{W-R+34}" y2="{yy}" '
                   f'stroke="{color}" stroke-width="2"/>')
        out.append(f'<text x="{W-R+40}" y="{yy+4}">{st}</text>')

    out.append("</svg>")
    open(path, "w").write("\n".join(out))


os.makedirs(outdir, exist_ok=True)
chart(os.path.join(outdir, "time.svg"),
      "Время лучшего варианта этапа", "время, мс", "min_ms", True)
chart(os.path.join(outdir, "bandwidth.svg"),
      "Достигнутая пропускная способность", "ГБ/с", "gbps", False)
