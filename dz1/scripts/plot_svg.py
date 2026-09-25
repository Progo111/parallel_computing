#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Графики отчёта (§8.2 ТЗ) в виде самодостаточных SVG — без сторонних библиотек.

ДЗ №1 «Освоение векторизации», вариант 3 (C = A+B, D = A-B), fp32, Apple M4 / NEON.

Пример:
    python3 scripts/plot_svg.py --raw results/raw.csv --outdir report/img \\
            --bandwidth results/bandwidth.csv

Рисует:
    time_vs_n.svg       время (мс) от N, лог-лог, кривая на этап + границы L1d/L2 (§8.2.2)
    bandwidth_vs_n.svg  достигнутая ПСП (ГиБ/с) от N, лог по X, линия измеренного пика
    roofline.svg        roofline: AI (FLOP/Б) x GFLOP/s, крыша ПСП + потолок FLOPS (§8.2.3)
    speedup.svg         speedup этапов vs baseline, сгруппированные столбцы

Разбор CSV переиспользуется из scripts/report_table.py (лежит рядом).
"""

from __future__ import annotations

import argparse
import math
import os
import sys
from typing import Dict, List, Optional, Sequence, Tuple

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from report_table import (  # noqa: E402  (импорт после правки sys.path — так надо)
    DEFAULT_L1D, DEFAULT_L2, DEFAULT_PEAK_GB, DEFAULT_PEAK_GIB, DEFAULT_SPREAD_LIMIT,
    GIB, Config, Data, Peak, elem_size, load_bandwidth, load_raw, pick_dtype, warn,
)

# --------------------------------------------------------------------------
# Палитра (проверена валидатором: категориальная, светлый фон, CVD ΔE >= 8)
# --------------------------------------------------------------------------

SURFACE = "#ffffff"
INK = "#0b0b0b"
INK_2 = "#52514e"
INK_3 = "#8b8a85"
GRID = "#e6e5e1"
AXIS = "#b9b8b3"

SERIES_COLORS = ["#2a78d6", "#eb6834", "#1baf7a", "#eda100", "#e87ba4", "#008300"]
# Форма маркера дублирует цвет (вторичное кодирование: печать, ч/б, дальтонизм).
SERIES_SHAPES = ["circle", "square", "triangle", "diamond", "plus", "cross"]

W, H = 900, 560
PAD_L, PAD_R, PAD_T, PAD_B = 78, 176, 54, 66

FONT = "Helvetica, Arial, 'DejaVu Sans', sans-serif"


def esc(s: str) -> str:
    return (str(s).replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
            .replace('"', "&quot;"))


def nz(v: float) -> str:
    """Короткая запись координаты."""
    return ("%.2f" % v).rstrip("0").rstrip(".")


class Svg:
    """Минимальный построитель SVG-документа."""

    def __init__(self, w: int = W, h: int = H, title: str = "", subtitle: str = ""):
        self.w, self.h = w, h
        self.body: List[str] = []
        self.title = title
        self.subtitle = subtitle

    def add(self, s: str) -> None:
        self.body.append(s)

    def rect(self, x, y, w, h, fill, **kw) -> None:
        extra = "".join(' %s="%s"' % (k.replace("_", "-"), esc(v)) for k, v in kw.items())
        self.add('<rect x="%s" y="%s" width="%s" height="%s" fill="%s"%s/>'
                 % (nz(x), nz(y), nz(max(w, 0)), nz(max(h, 0)), fill, extra))

    def line(self, x1, y1, x2, y2, stroke, width=1.0, dash=None, opacity=None) -> None:
        d = ' stroke-dasharray="%s"' % dash if dash else ""
        o = ' stroke-opacity="%s"' % opacity if opacity is not None else ""
        self.add('<line x1="%s" y1="%s" x2="%s" y2="%s" stroke="%s" stroke-width="%s"%s%s/>'
                 % (nz(x1), nz(y1), nz(x2), nz(y2), stroke, nz(width), d, o))

    def text(self, x, y, s, size=12, fill=INK_2, anchor="start", weight="normal",
             rotate=None, family=FONT) -> None:
        tr = ' transform="rotate(%s %s %s)"' % (nz(rotate), nz(x), nz(y)) if rotate else ""
        self.add('<text x="%s" y="%s" font-family="%s" font-size="%s" fill="%s" '
                 'text-anchor="%s" font-weight="%s"%s>%s</text>'
                 % (nz(x), nz(y), family, nz(size), fill, anchor, weight, tr, esc(s)))

    def polyline(self, pts: Sequence[Tuple[float, float]], stroke: str, width=2.0,
                 dash=None) -> None:
        if len(pts) < 2:
            return
        d = ' stroke-dasharray="%s"' % dash if dash else ""
        s = " ".join("%s,%s" % (nz(x), nz(y)) for x, y in pts)
        self.add('<polyline points="%s" fill="none" stroke="%s" stroke-width="%s" '
                 'stroke-linejoin="round" stroke-linecap="round"%s/>' % (s, stroke, nz(width), d))

    def marker(self, x, y, shape: str, color: str, size: float = 4.6) -> None:
        """Маркер с 2 px кольцом цвета фона — чтобы точки не слипались."""
        ring = ' stroke="%s" stroke-width="1.6"' % SURFACE
        if shape == "circle":
            self.add('<circle cx="%s" cy="%s" r="%s" fill="%s"%s/>'
                     % (nz(x), nz(y), nz(size), color, ring))
        elif shape == "square":
            s = size * 1.78
            self.add('<rect x="%s" y="%s" width="%s" height="%s" fill="%s"%s/>'
                     % (nz(x - s / 2), nz(y - s / 2), nz(s), nz(s), color, ring))
        elif shape == "triangle":
            s = size * 1.25
            pts = [(x, y - s), (x + s * 0.92, y + s * 0.7), (x - s * 0.92, y + s * 0.7)]
            self.add('<polygon points="%s" fill="%s"%s/>'
                     % (" ".join("%s,%s" % (nz(a), nz(b)) for a, b in pts), color, ring))
        elif shape == "diamond":
            s = size * 1.25
            pts = [(x, y - s), (x + s, y), (x, y + s), (x - s, y)]
            self.add('<polygon points="%s" fill="%s"%s/>'
                     % (" ".join("%s,%s" % (nz(a), nz(b)) for a, b in pts), color, ring))
        elif shape == "plus":
            s, t = size * 1.25, size * 0.42
            self.add('<path d="M%s %s h%s v%s h%s v%s h%s v%s h%s v%s h%s Z" fill="%s"%s/>'
                     % (nz(x - t), nz(y - s), nz(2 * t), nz(s - t), nz(s - t), nz(2 * t),
                        nz(-(s - t)), nz(s - t), nz(-2 * t), nz(-(s - t)), nz(-(s - t)),
                        color, ring))
        else:  # cross
            s = size * 1.05
            self.add('<path d="M%s %s L%s %s M%s %s L%s %s" stroke="%s" stroke-width="2.6" '
                     'stroke-linecap="round" fill="none"/>'
                     % (nz(x - s), nz(y - s), nz(x + s), nz(y + s),
                        nz(x + s), nz(y - s), nz(x - s), nz(y + s), color))

    def render(self) -> str:
        head = ['<?xml version="1.0" encoding="UTF-8"?>',
                '<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d" '
                'viewBox="0 0 %d %d" role="img">' % (self.w, self.h, self.w, self.h)]
        if self.title:
            head.append("<title>%s</title>" % esc(self.title))
        if self.subtitle:
            head.append("<desc>%s</desc>" % esc(self.subtitle))
        head.append('<rect width="%d" height="%d" fill="%s"/>' % (self.w, self.h, SURFACE))
        return "\n".join(head + self.body + ["</svg>", ""])


# --------------------------------------------------------------------------
# Оси
# --------------------------------------------------------------------------


class LogScale:
    """Логарифмическая ось. tight=True — границы по данным (+ небольшой отступ),
    иначе округление до целых декад."""

    def __init__(self, vmin: float, vmax: float, p0: float, p1: float, tight: bool = True):
        vmin = max(vmin, 1e-12)
        vmax = max(vmax, vmin * 1.02)
        lo, hi = math.log10(vmin), math.log10(vmax)
        if tight:
            pad = max((hi - lo) * 0.04, 0.02)
            self.lo, self.hi = lo - pad, hi + pad
        else:
            self.lo, self.hi = math.floor(lo), math.ceil(hi)
            if self.hi - self.lo < 1:
                self.hi = self.lo + 1
        self.p0, self.p1 = p0, p1

    def __call__(self, v: float) -> float:
        v = max(v, 1e-12)
        f = (math.log10(v) - self.lo) / (self.hi - self.lo)
        return self.p0 + f * (self.p1 - self.p0)

    def ticks(self) -> List[Tuple[float, str, bool]]:
        mults = (1, 2, 5) if (self.hi - self.lo) >= 0.9 else (1, 1.5, 2, 3, 5, 7)
        out: List[Tuple[float, str, bool]] = []
        for k in range(int(math.floor(self.lo)), int(math.ceil(self.hi)) + 1):
            for m in mults:
                v = m * (10.0 ** k)
                lg = math.log10(v)
                if lg < self.lo - 1e-9 or lg > self.hi + 1e-9:
                    continue
                out.append((v, fmt_tick(v), m == 1))
        return out


class LinScale:
    def __init__(self, vmin: float, vmax: float, p0: float, p1: float, nice: int = 6):
        if not math.isfinite(vmin) or not math.isfinite(vmax) or vmax <= vmin:
            vmin, vmax = 0.0, max(vmax, 1.0)
        span = vmax - vmin
        step = nice_step(span / max(nice, 1))
        self.lo = math.floor(vmin / step) * step
        self.hi = math.ceil(vmax / step) * step
        if self.hi <= self.lo:
            self.hi = self.lo + step
        self.step = step
        self.p0, self.p1 = p0, p1

    def __call__(self, v: float) -> float:
        f = (v - self.lo) / (self.hi - self.lo)
        return self.p0 + f * (self.p1 - self.p0)

    def ticks(self) -> List[Tuple[float, str, bool]]:
        out = []
        k = 0
        v = self.lo
        while v <= self.hi + self.step * 1e-6:
            out.append((v, fmt_tick(v), True))
            k += 1
            v = self.lo + k * self.step
        return out


def nice_step(raw: float) -> float:
    if raw <= 0 or not math.isfinite(raw):
        return 1.0
    mag = 10.0 ** math.floor(math.log10(raw))
    for m in (1, 2, 2.5, 5, 10):
        if raw <= m * mag:
            return m * mag
    return 10 * mag


def fmt_tick(v: float) -> str:
    if v == 0:
        return "0"
    a = abs(v)
    if a >= 1000:
        return "%d" % round(v) if a < 1e6 else "%g" % v
    if a >= 1:
        return ("%.0f" % v) if abs(v - round(v)) < 1e-9 else ("%.1f" % v)
    if a >= 0.01:
        return ("%.2f" % v).rstrip("0").rstrip(".")
    return "%g" % v


def draw_frame(svg: Svg, title: str, subtitle: str, xlabel: str, ylabel: str,
               plot: Tuple[float, float, float, float]) -> None:
    x0, y0, x1, y1 = plot
    svg.text(PAD_L - 48, 26, title, size=16, fill=INK, weight="600")
    if subtitle:
        svg.text(PAD_L - 48, 44, subtitle, size=11.5, fill=INK_3)
    svg.text((x0 + x1) / 2, y1 + 44, xlabel, size=12.5, fill=INK_2, anchor="middle")
    svg.text(22, (y0 + y1) / 2, ylabel, size=12.5, fill=INK_2, anchor="middle",
             rotate=-90)


def draw_axes(svg: Svg, xs, ys, plot, x_ticks=None, y_ticks=None,
              stagger_x: bool = False) -> None:
    x0, y0, x1, y1 = plot
    for v, lab, major in (y_ticks if y_ticks is not None else ys.ticks()):
        py = ys(v)
        if py < y0 - 0.5 or py > y1 + 0.5:
            continue
        svg.line(x0, py, x1, py, GRID, 1.0 if major else 0.6,
                 opacity=None if major else 0.6)
        svg.text(x0 - 9, py + 4, lab, size=11, fill=INK_3, anchor="end")
    last_x = -1e9
    row = 0
    for v, lab, major in (x_ticks if x_ticks is not None else xs.ticks()):
        px = xs(v)
        if px < x0 - 0.5 or px > x1 + 0.5:
            continue
        svg.line(px, y0, px, y1, GRID, 1.0 if major else 0.6,
                 opacity=None if major else 0.6)
        if stagger_x:
            row = 1 - row if px - last_x < 34 else 0
        elif px - last_x < 30:
            continue  # подписи слиплись бы — тик рисуем, подпись пропускаем
        last_x = px
        svg.text(px, y1 + 17 + row * 14, lab, size=11, fill=INK_3, anchor="middle")
    svg.line(x0, y1, x1, y1, AXIS, 1.2)
    svg.line(x0, y0, x0, y1, AXIS, 1.2)


def draw_legend(svg: Svg, entries: List[Tuple[str, str, Optional[str]]], x: float,
                y: float, title: str = "") -> float:
    """entries: (подпись, цвет, форма маркера или None для линии-примечания).
    Возвращает Y ниже последней строки легенды."""
    if title:
        svg.text(x, y, title, size=11, fill=INK_3, weight="600")
        y += 16
    for label, color, shape in entries:
        svg.line(x, y - 4, x + 18, y - 4, color, 2.4)
        if shape:
            svg.marker(x + 9, y - 4, shape, color, 4.2)
        svg.text(x + 25, y, label, size=11.5, fill=INK_2)
        y += 19
    return y


# --------------------------------------------------------------------------
# Данные для графиков
# --------------------------------------------------------------------------


def stage_series(data: Data) -> List[Tuple[str, List[Config]]]:
    """Одна кривая на этап: выбирается ОДИН вариант+режим, лучший в среднем по всем N.

    Критерий — геометрическое среднее отношения `t / t(лучший при этом N)`; за
    отсутствующие N начисляется штраф. Фиксированный вариант на этап нужен, чтобы
    кривая ГиБ/с не прыгала при смене модели трафика (sep 16 Б ↔ fused 12 Б).
    """
    out: List[Tuple[str, List[Config]]] = []
    for stage in data.stages:
        cands: Dict[Tuple[str, str], List[Config]] = {}
        for c in data.select(stage=stage):
            if math.isfinite(c.tmin):
                cands.setdefault((c.variant, c.mode), []).append(c)
        best_at = {n: data.best(stage=stage, n=n) for n in data.ns}
        scored = []
        for key, cfgs in cands.items():
            acc, k = 0.0, 0
            for c in cfgs:
                b = best_at.get(c.n)
                if b is None or c.tmin <= 0 or b.tmin <= 0:
                    continue
                acc += math.log(c.tmin / b.tmin)
                k += 1
            if k == 0:
                continue
            penalty = (len(data.ns) - k) * 0.5
            scored.append((acc / k + penalty, key, cfgs))
        if not scored:
            continue
        scored.sort(key=lambda s: (s[0], s[1]))
        _, _key, cfgs = scored[0]
        out.append((stage, sorted(cfgs, key=lambda c: c.n)))
    return out


def resolve_labels(labels: List[Tuple[float, float, str, str]],
                   dy: float = 13.0) -> List[Tuple[float, float, str, str]]:
    """Грубое разведение подписей по вертикали, чтобы не наезжали друг на друга."""
    out: List[Tuple[float, float, str, str]] = []
    for x, y, txt, anchor in sorted(labels, key=lambda t: t[1]):
        wq = len(txt) * 5.4
        xa = x if anchor == "start" else x - wq / 2
        moved = True
        guard = 0
        while moved and guard < 40:
            moved = False
            guard += 1
            for ox, oy, otxt, oanchor in out:
                ow = len(otxt) * 5.4
                oxa = ox if oanchor == "start" else ox - ow / 2
                if abs(oy - y) < 11 and not (xa + wq < oxa or oxa + ow < xa):
                    y += dy
                    moved = True
                    break
        out.append((x, y, txt, anchor))
    return out


def draw_note(svg: Svg, x: float, y: float, text: str, width: int = 30,
              max_lines: int = 6, size: float = 9.5) -> float:
    """Примечание мелким шрифтом с переносом. Возвращает Y после последней строки."""
    for chunk in wrap(text, width)[:max_lines]:
        svg.text(x, y, chunk, size=size, fill=INK_3)
        y += 12
    return y


def stage_style(i: int) -> Tuple[str, str]:
    return SERIES_COLORS[i % len(SERIES_COLORS)], SERIES_SHAPES[i % len(SERIES_SHAPES)]


def cache_boundaries(l1d: int, l2: int, esz: int) -> List[Tuple[float, float, str, str]]:
    """Границы режимов кэша: (N_fused, N_sep, заголовок, подпись).

    Рабочий набор `fused` (4 матрицы) больше, чем у `sep` (3), поэтому граница
    для fused наступает раньше — между ними рисуется полоса перехода.
    """
    out = []
    for name, size in (("L1d", l1d), ("L2", l2)):
        n_fused = math.sqrt(size / (4.0 * esz))
        n_sep = math.sqrt(size / (3.0 * esz))
        if n_sep > 1:
            out.append((n_fused, n_sep, "граница %s (%s)" % (name, human(size)),
                        "N ≈ %.0f (fused) … %.0f (sep)" % (n_fused, n_sep)))
    return out


def human(b: float) -> str:
    if b >= 1024 ** 2:
        return "%.0f МиБ" % (b / 1024 ** 2)
    return "%.0f КиБ" % (b / 1024)


def draw_cache_bands(svg: Svg, xs, plot, args, esz: int, labels: bool = True) -> None:
    """Вертикальные полосы на границах L1d / L2 (граница fused … граница sep)."""
    x0, y0, x1, y1 = plot
    for n_fused, n_sep, head, sub in cache_boundaries(args.l1d, args.l2, esz):
        pa, pb = xs(n_fused), xs(n_sep)
        if pb < x0 or pa > x1:
            continue
        pa, pb = max(pa, x0), min(pb, x1)
        if pb - pa >= 1.5:
            svg.rect(pa, y0, pb - pa, y1 - y0, INK_3, fill_opacity="0.10")
        svg.line(pa, y0, pa, y1, INK_3, 1.3, dash="6 4")
        svg.line(pb, y0, pb, y1, INK_3, 1.3, dash="6 4")
        if labels:
            svg.text(pb + 5, y0 + 13, head, size=10.5, fill=INK_3)
            svg.text(pb + 5, y0 + 25, sub, size=10.5, fill=INK_3)
        else:
            svg.text(pb + 5, y0 + 13, head.replace("граница ", ""), size=10.5, fill=INK_3)


def write_svg(path: str, svg: Svg) -> None:
    os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
    with open(path, "w", encoding="utf-8") as fh:
        fh.write(svg.render())
    print("plot_svg: %s" % path, file=sys.stderr)


# --------------------------------------------------------------------------
# График 1 — время от N (лог-лог), §8.2.2
# --------------------------------------------------------------------------


def plot_time_vs_n(data: Data, args, esz: int, path: str) -> bool:
    series = stage_series(data)
    if not series:
        warn("нет данных для time_vs_n.svg")
        return False
    x0, y0, x1, y1 = PAD_L, PAD_T, W - PAD_R, H - PAD_B
    all_t = [c.tmin * 1000.0 for _, pts in series for c in pts]
    xs = LogScale(min(data.ns), max(data.ns), x0, x1)
    ys = LogScale(min(all_t), max(all_t), y1, y0)

    svg = Svg(title="Время прохода от N", subtitle="лог-лог; кривая на этап")
    draw_frame(svg, "Время одного прохода от размера матрицы",
               "лог-лог; на кривой — лучший вариант этапа; серые полосы — границы L1d/L2 "
               "(левый край — fused, правый — sep)",
               "N (матрица N×N, fp32)", "время, мс", (x0, y0, x1, y1))
    xt = [(float(n), str(n), True) for n in data.ns]
    draw_axes(svg, xs, ys, (x0, y0, x1, y1), x_ticks=xt, stagger_x=True)

    draw_cache_bands(svg, xs, (x0, y0, x1, y1), args, esz, labels=True)

    legend = []
    for i, (stage, pts) in enumerate(series):
        color, shape = stage_style(i)
        coords = [(xs(c.n), ys(c.tmin * 1000.0)) for c in pts]
        svg.polyline(coords, color, 2.0)
        for (px, py), c in zip(coords, pts):
            svg.marker(px, py, shape, color)
        legend.append(("%s — %s/%s" % (stage, pts[-1].variant, pts[-1].mode), color, shape))
    draw_legend(svg, legend, x1 + 18, y0 + 12, "Этап (лучший вариант)")
    svg.text(x1 + 18, y1 - 4, "идеал: наклон 2 (время ~ N²)", size=10.5, fill=INK_3)
    write_svg(path, svg)
    return True


# --------------------------------------------------------------------------
# График 2 — достигнутая ПСП от N
# --------------------------------------------------------------------------


def plot_bandwidth_vs_n(data: Data, peak: Peak, args, esz: int, path: str) -> bool:
    series = stage_series(data)
    if not series:
        warn("нет данных для bandwidth_vs_n.svg")
        return False
    x0, y0, x1, y1 = PAD_L, PAD_T, W - PAD_R, H - PAD_B
    vals = [c.gibs for _, pts in series for c in pts] + [peak.gib]
    xs = LogScale(min(data.ns), max(data.ns), x0, x1)
    ys = LinScale(0.0, max(vals) * 1.08, y1, y0)

    svg = Svg(title="Достигнутая ПСП от N", subtitle="ГиБ/с по фактическому трафику")
    draw_frame(svg, "Достигнутая пропускная способность памяти",
               "трафик = bytes_per_elem · 2·N² (§6.4.1 ТЗ); лог по X",
               "N (матрица N×N, fp32)", "ГиБ/с", (x0, y0, x1, y1))
    xt = [(float(n), str(n), True) for n in data.ns]
    draw_axes(svg, xs, ys, (x0, y0, x1, y1), x_ticks=xt, stagger_x=True)

    draw_cache_bands(svg, xs, (x0, y0, x1, y1), args, esz, labels=False)

    if peak.gib > 0:
        py = ys(peak.gib)
        if y0 <= py <= y1:
            svg.line(x0, py, x1, py, INK, 1.3, dash="3 4", opacity=0.55)
            svg.text(x0 + 6, py - 6, "измеренный пик ПСП = %.1f ГиБ/с (%.0f ГБ/с)"
                     % (peak.gib, peak.gib * GIB / 1e9), size=10.5, fill=INK_2)

    legend = []
    for i, (stage, pts) in enumerate(series):
        color, shape = stage_style(i)
        coords = [(xs(c.n), ys(c.gibs)) for c in pts]
        svg.polyline(coords, color, 2.0)
        for (px, py), c in zip(coords, pts):
            svg.marker(px, py, shape, color)
        legend.append(("%s — %s/%s" % (stage, pts[-1].variant, pts[-1].mode), color, shape))
    ly = draw_legend(svg, legend, x1 + 18, y0 + 12, "Этап (лучший вариант)")
    svg.text(x1 + 18, ly + 10, "Источник пика ПСП:", size=10, fill=INK_3, weight="600")
    draw_note(svg, x1 + 18, ly + 24, peak.source, width=30, max_lines=8)
    write_svg(path, svg)
    return True


def wrap(s: str, width: int) -> List[str]:
    words, lines, cur = s.split(), [], ""
    for w_ in words:
        if len(cur) + len(w_) + 1 > width and cur:
            lines.append(cur)
            cur = w_
        else:
            cur = (cur + " " + w_).strip()
    if cur:
        lines.append(cur)
    return lines


# --------------------------------------------------------------------------
# График 3 — roofline, §8.2.3
# --------------------------------------------------------------------------


def plot_roofline(data: Data, peak: Peak, args, path: str) -> bool:
    n = args.roofline_n if args.roofline_n in data.ns else (data.ns[-1] if data.ns else None)
    if n is None:
        warn("нет данных для roofline.svg")
        return False
    pts: List[Tuple[str, Config]] = []
    for stage in data.stages:
        for mode in data.modes:
            cfg = data.best(stage=stage, mode=mode, n=n)
            if cfg is not None:
                pts.append((stage, cfg))
    if not pts:
        warn("нет данных для roofline.svg при N = %d" % n)
        return False

    peak_flops = args.peak_gflops
    flops_src = "задан ключом --peak-gflops"
    if peak_flops is None:
        peak_flops = args.simd_lanes * args.fp_pipes * args.clock_ghz
        flops_src = ("оценка: %d лейнов fp32 × %d портов FADD × %.2f ГГц = %.1f GFLOP/s"
                     % (args.simd_lanes, args.fp_pipes, args.clock_ghz, peak_flops))

    ai_vals = [1.0 / c.bpe for _, c in pts]
    gf_vals = [c.gflops for _, c in pts]
    bw_bytes = peak.gib * GIB
    ridge = (peak_flops * 1e9 / bw_bytes) if bw_bytes > 0 and peak_flops > 0 else max(ai_vals)

    x0, y0, x1, y1 = PAD_L, PAD_T, W - PAD_R, H - PAD_B
    xs = LogScale(min(ai_vals) / 2.2, max(max(ai_vals), ridge) * 3.0, x0, x1)
    ys = LogScale(min(gf_vals) / 2.2, max(max(gf_vals), peak_flops) * 1.6, y1, y0)

    svg = Svg(title="Roofline", subtitle="N = %d" % n)
    draw_frame(svg, "Roofline: где находится задача C = A+B, D = A−B",
               "N = %d; точки — лучший вариант этапа в каждом режиме; совпавшие точки "
               "разнесены по горизонтали" % n,
               "арифметическая интенсивность, FLOP/байт", "GFLOP/s", (x0, y0, x1, y1))
    draw_axes(svg, xs, ys, (x0, y0, x1, y1))

    # Наклонная крыша ПСП: GFLOP/s = AI * Peak_BW.
    roof = []
    steps = 60
    for k in range(steps + 1):
        lg = xs.lo + (xs.hi - xs.lo) * k / steps
        ai = 10.0 ** lg
        gf = min(ai * bw_bytes / 1e9, peak_flops if peak_flops > 0 else math.inf)
        roof.append((xs(ai), ys(max(gf, 1e-9))))
    svg.polyline(roof, INK, 2.0)
    svg.text(x0 + 8, ys(min(gf_vals) / 1.6), "крыша ПСП: GFLOP/s = AI × %.1f ГиБ/с"
             % (bw_bytes / GIB), size=10.5, fill=INK_2)
    if peak_flops > 0:
        py = ys(peak_flops)
        if y0 <= py <= y1:
            svg.text(x1 - 6, py - 7, "потолок FLOPS = %.1f GFLOP/s" % peak_flops,
                     size=10.5, fill=INK_2, anchor="end")
        px = xs(ridge)
        if x0 <= px <= x1:
            svg.line(px, y0, px, y1, INK_3, 1.2, dash="6 4")
            svg.text(px + 5, y1 - 8, "перегиб AI = %.3f FLOP/Б" % ridge, size=10.5, fill=INK_3)

    # Точки этапов часто совпадают (одна модель трафика, обе упёрлись в ПСП) —
    # кластеризуем и разносим по горизонтали, подпись на кластер одна.
    placed = []
    for stage, cfg in pts:
        i = data.stages.index(stage) if stage in data.stages else 0
        color, shape = stage_style(i)
        placed.append((xs(1.0 / cfg.bpe), ys(cfg.gflops), stage, cfg.mode, color, shape))
    clusters: List[List[tuple]] = []
    for p in placed:
        for cl in clusters:
            if abs(cl[0][0] - p[0]) < 10 and abs(cl[0][1] - p[1]) < 10:
                cl.append(p)
                break
        else:
            clusters.append([p])

    legend = []
    seen: Dict[str, Tuple[str, str]] = {}
    labels: List[Tuple[float, float, str, str]] = []
    for cl in clusters:
        k = len(cl)
        for i, (px, py, stage, mode, color, shape) in enumerate(cl):
            dx = (i - (k - 1) / 2.0) * 13.0
            svg.marker(px + dx, py, shape, color, 5.2)
        cx = cl[0][0]
        modes = sorted({p[3] for p in cl})
        label = "%s/%s" % (",".join(p[2] for p in cl), ",".join(modes))
        if k == 1:
            labels.append((cx + 10, cl[0][1] + 4, label, "start"))
        else:
            labels.append((cx, cl[0][1] + 20, label, "middle"))
    for lx, ly_, txt, anchor in resolve_labels(labels):
        svg.text(lx, ly_, txt, size=10, fill=INK_2, anchor=anchor)
    for _, _, stage, _, color, shape in placed:
        if stage not in seen:
            seen[stage] = (color, shape)
            legend.append((stage, color, shape))
    ly = draw_legend(svg, legend, x1 + 18, y0 + 12, "Этап")
    ly = draw_note(svg, x1 + 18, ly + 10, "Потолок FLOPS: " + flops_src, width=30)
    draw_note(svg, x1 + 18, ly + 8, "ПСП: " + peak.source, width=30, max_lines=8)
    write_svg(path, svg)
    return True


# --------------------------------------------------------------------------
# График 4 — speedup по этапам, сгруппированные столбцы
# --------------------------------------------------------------------------


def plot_speedup(data: Data, args, path: str) -> bool:
    stages = [s for s in data.stages if any(data.best(stage=s, n=n) for n in data.ns)]
    if not stages or not data.ns:
        warn("нет данных для speedup.svg")
        return False
    vals: Dict[Tuple[str, int], float] = {}
    for n in data.ns:
        base = data.get(args.baseline, "sep", n)
        if base is None:
            warn("speedup.svg: нет baseline %s/sep при N = %d — столбцы пропущены"
                 % (args.baseline, n))
            continue
        for s in stages:
            cfg = data.best(stage=s, n=n)
            if cfg is not None and cfg.tmin > 0:
                vals[(s, n)] = base.tmin / cfg.tmin
    if not vals:
        warn("speedup.svg: baseline %s не найден ни при одном N" % args.baseline)
        return False

    x0, y0, x1, y1 = PAD_L, PAD_T, W - PAD_R, H - PAD_B
    ys = LinScale(0.0, max(vals.values()) * 1.12, y1, y0)
    svg = Svg(title="Ускорение по этапам", subtitle="vs %s / sep" % args.baseline)
    draw_frame(svg, "Ускорение лучшего варианта этапа",
               "за 1.00× принят %s в режиме sep; подписано только максимальное в группе"
               % args.baseline,
               "N (матрица N×N, fp32)", "ускорение, ×", (x0, y0, x1, y1))
    for v, lab, _ in ys.ticks():
        py = ys(v)
        svg.line(x0, py, x1, py, GRID, 1.0)
        svg.text(x0 - 9, py + 4, lab + "×", size=11, fill=INK_3, anchor="end")
    svg.line(x0, y1, x1, y1, AXIS, 1.2)
    svg.line(x0, y0, x0, y1, AXIS, 1.2)

    gw = (x1 - x0) / max(len(data.ns), 1)
    bw_ = max((gw - 16) / max(len(stages), 1), 3.0)
    for gi, n in enumerate(data.ns):
        gx = x0 + gi * gw + 8
        best_v, best_i = -1.0, -1
        for si, s in enumerate(stages):
            v = vals.get((s, n))
            if v is not None and v > best_v:
                best_v, best_i = v, si
        for si, s in enumerate(stages):
            v = vals.get((s, n))
            if v is None:
                continue
            color, _ = stage_style(si)
            bx = gx + si * bw_
            by = ys(v)
            # 2 px зазор между соседними столбцами, скруглённый верх
            svg.add('<rect x="%s" y="%s" width="%s" height="%s" fill="%s" rx="3"/>'
                    % (nz(bx + 1), nz(by), nz(max(bw_ - 2, 1)), nz(y1 - by), color))
            if si == best_i:
                svg.text(bx + bw_ / 2, by - 6, "%.1f×" % v, size=10.5, fill=INK_2,
                         anchor="middle", weight="600")
        svg.text(gx + (bw_ * len(stages)) / 2, y1 + 17, str(n), size=11, fill=INK_3,
                 anchor="middle")
    legend = [(s, stage_style(i)[0], None) for i, s in enumerate(stages)]
    draw_legend(svg, legend, x1 + 18, y0 + 12, "Этап")
    write_svg(path, svg)
    return True


# --------------------------------------------------------------------------
# main
# --------------------------------------------------------------------------


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        prog="plot_svg.py",
        description="Графики отчёта (§8.2 ТЗ) в виде самодостаточных SVG. "
                    "Только стандартная библиотека Python, matplotlib не нужен.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    p.add_argument("--raw", default="results/raw.csv", help="сырые замеры, CSV со схемой §6.4 ТЗ")
    p.add_argument("--outdir", default="report/img", help="куда класть *.svg")
    p.add_argument("--bandwidth", default=None,
                   help="results/bandwidth.csv от bw_probe: пик ПСП из тестов add / add_nt")
    p.add_argument("--peak-gib", type=float, default=DEFAULT_PEAK_GIB,
                   help="запасной пик ПСП, ГиБ/с (если нет --bandwidth); по умолчанию — "
                        "оценка ~%.0f ГБ/с из спецификации, НЕ замер" % DEFAULT_PEAK_GB)
    p.add_argument("--baseline", default="e0_O0", help="вариант-baseline для speedup (режим sep)")
    p.add_argument("--dtype", default=None, help="какой dtype рисовать (по умолчанию f32)")
    p.add_argument("--bw-rfo-correct", action=argparse.BooleanOptionalAction, default=True,
                   help="пересчитывать тест add из bandwidth.csv в модель без RFO (×12/16)")
    p.add_argument("--l1d", type=int, default=DEFAULT_L1D, help="размер L1d P-ядра, байт")
    p.add_argument("--l2", type=int, default=DEFAULT_L2, help="размер L2 кластера P-ядер, байт")
    p.add_argument("--spread-limit", type=float, default=DEFAULT_SPREAD_LIMIT,
                   help="порог (max−min)/min для пометки зашумлённых прогонов")
    p.add_argument("--roofline-n", type=int, default=None,
                   help="N для roofline (по умолчанию — максимальное из имеющихся)")
    p.add_argument("--peak-gflops", type=float, default=None,
                   help="потолок FLOPS, GFLOP/s; если не задан — считается как "
                        "lanes × pipes × частота")
    p.add_argument("--simd-lanes", type=int, default=4, help="лейнов fp32 в векторе (NEON 128 бит)")
    p.add_argument("--fp-pipes", type=int, default=4, help="портов FADD на ядро (оценка)")
    p.add_argument("--clock-ghz", type=float, default=4.4, help="частота P-ядра, ГГц (оценка)")
    return p


def main(argv: Optional[Sequence[str]] = None) -> int:
    args = build_parser().parse_args(argv)
    if not os.path.exists(args.raw):
        print("error: нет файла с сырыми данными: %s" % args.raw, file=sys.stderr)
        return 1
    dtype = args.dtype or pick_dtype(args.raw)
    configs = load_raw(args.raw, dtype, args.l1d, args.l2, args.spread_limit)
    data = Data(configs)
    peak = load_bandwidth(args.bandwidth, args.peak_gib, args.bw_rfo_correct)
    esz = elem_size(dtype or (configs[0].dtype or "f32"))

    os.makedirs(args.outdir, exist_ok=True)
    ok = 0
    ok += plot_time_vs_n(data, args, esz, os.path.join(args.outdir, "time_vs_n.svg"))
    ok += plot_bandwidth_vs_n(data, peak, args, esz,
                              os.path.join(args.outdir, "bandwidth_vs_n.svg"))
    ok += plot_roofline(data, peak, args, os.path.join(args.outdir, "roofline.svg"))
    ok += plot_speedup(data, args, os.path.join(args.outdir, "speedup.svg"))
    if ok == 0:
        print("error: не удалось построить ни одного графика", file=sys.stderr)
        return 1
    print("plot_svg: готово %d из 4 графиков в %s" % (ok, args.outdir), file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
