#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Генерация markdown-таблиц отчёта из results/raw.csv.

ДЗ №1 «Освоение векторизации», вариант 3 (C = A+B, D = A-B), fp32, Apple M4 / NEON.
Покрывает §10 ТЗ (таблицы результатов), §6.2.3 (min/медиана/σ/разброс),
§8.1.3 (roofline-предел), §7 (контроль корректности).

Пример:
    python3 scripts/report_table.py --raw results/raw.csv --out results/table.md \\
            --bandwidth results/bandwidth.csv

Зависимости: только стандартная библиотека (csv, statistics, argparse, math).
Модуль импортируется из scripts/plot_svg.py — весь разбор CSV живёт здесь.
"""

from __future__ import annotations

import argparse
import csv
import math
import os
import statistics
import sys
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Sequence, Set, Tuple

# --------------------------------------------------------------------------
# Схема данных (зафиксирована в src/common.h и src/bw_probe.c)
# --------------------------------------------------------------------------

RAW_COLUMNS = [
    "stage", "variant", "flags_id", "dtype", "N", "mode", "bytes_per_elem",
    "reps", "run_idx", "time_sec", "gflops", "gib_per_sec", "checksum", "bitexact",
]
BW_COLUMNS = ["test", "mib", "bytes_per_elem", "run_idx", "time_sec", "gb_per_sec", "gib_per_sec"]

STAGE_ORDER = ["E0", "E0'", "E1", "E2", "E3", "E4"]
# Цепочка «предыдущего этапа» для столбца «Speedup vs пред.» (§10).
PREV_STAGE = {"E0'": "E0", "E1": "E0'", "E2": "E1", "E3": "E2", "E4": "E3"}
MODE_ORDER = ["sep", "fused"]
MODE_RU = {"sep": "разд.", "fused": "слит."}

VARIANT_ORDER = [
    "e0_O0", "e0_O1",
    "e1_O2", "e1_O3", "e1_Ofast", "e1_ffm",
    "e2_w4i1", "e2_w4i2", "e2_w4i4", "e2_w4i8",
    "e3_u1", "e3_u2", "e3_u4", "e3_u8",
    "e4_d0", "e4_d64", "e4_d128", "e4_d256", "e4_d512", "e4_d1024", "e4_d2048",
]

# Человекочитаемые описания (соответствуют VARIANT_LIST в src/common.h).
VARIANT_DESC = {
    "e0_O0": "наивный скаляр 2D, без оптимизации",
    "e0_O1": "наивный скаляр 2D, честный скалярный baseline",
    "e1_O2": "те же исходники, только ключи (-O2)",
    "e1_O3": "те же исходники, только ключи (-O3)",
    "e1_Ofast": "те же исходники, только ключи (-Ofast)",
    "e1_ffm": "те же исходники, только ключи (-O3 -ffast-math)",
    "e2_w4i1": "restrict+align+pragma, width 4 × interleave 1",
    "e2_w4i2": "restrict+align+pragma, width 4 × interleave 2",
    "e2_w4i4": "restrict+align+pragma, width 4 × interleave 4",
    "e2_w4i8": "restrict+align+pragma, width 4 × interleave 8",
    "e3_u1": "NEON intrinsics, разворот ×1 (4 элем./итер.)",
    "e3_u2": "NEON intrinsics, разворот ×2 (8 элем./итер.)",
    "e3_u4": "NEON intrinsics, разворот ×4 (16 элем./итер.)",
    "e3_u8": "NEON intrinsics, разворот ×8 (32 элем./итер.)",
    "e4_d0": "NEON + NT-store, без программного prefetch",
    "e4_d64": "NEON + NT-store + prefetch, dist 64 элем.",
    "e4_d128": "NEON + NT-store + prefetch, dist 128 элем.",
    "e4_d256": "NEON + NT-store + prefetch, dist 256 элем.",
    "e4_d512": "NEON + NT-store + prefetch, dist 512 элем.",
    "e4_d1024": "NEON + NT-store + prefetch, dist 1024 элем.",
    "e4_d2048": "NEON + NT-store + prefetch, dist 2048 элем.",
}

STAGE_DESC = {
    "E0": "наивный скаляр, без оптимизации",
    "E0'": "наивный скаляр, честный скалярный baseline",
    "E1": "авто-векторизация ключами компилятора",
    "E2": "полуавтоматическая векторизация (директивы)",
    "E3": "ручная векторизация, NEON intrinsics",
    "E4": "оптимизация трафика памяти (NT-store + prefetch)",
}

# Модель трафика на один ВЫХОДНОЙ элемент (байт), единая для всех этапов.
# Замерено: на M4 обычный store НЕ платит за write-allocate/RFO, а NT-store
# ничего не добавляет, поэтому модель зависит только от режима:
#   sep   = 4(a) + 4(b) + 4(store c)              = 12 Б / вых. элем.
#   fused = (4+4 + 4 + 4) / 2 вых. элем.          =  8 Б / вых. элем.
# Старая модель ТЗ с RFO (16/12) распознаётся и помечается предупреждением.
TRAFFIC_EXPECTED = {"sep": 12.0, "fused": 8.0}
TRAFFIC_RFO_LEGACY = {"sep": 16.0, "fused": 12.0}

GIB = float(1 << 30)
# Кэши P-ядра Apple M4 (бенчмарк принудительно идёт на P-ядре через
# QOS_CLASS_USER_INTERACTIVE). У E-ядер вдвое-вчетверо меньше: 65536 / 4194304 —
# режимы кэша считаем по P-ядру, иначе границы на графиках врут.
DEFAULT_L1D = 131072
DEFAULT_L2 = 16777216
E_CORE_L1D, E_CORE_L2 = 65536, 4194304
# §2.2.2 ТЗ: если ПСП не измерена — ≈120 ГБ/с из спецификации (это НЕ замер и
# это пик всей SoC; одно P-ядро вытягивает заметно меньше — нужен bw_probe).
DEFAULT_PEAK_GB = 120.0
DEFAULT_PEAK_GIB = round(DEFAULT_PEAK_GB * 1e9 / GIB, 2)
DEFAULT_SPREAD_LIMIT = 0.05  # §6.2.3 ТЗ

_WARNED: Set[str] = set()


def warn(msg: str, once_key: Optional[str] = None) -> None:
    """Предупреждение в stderr. once_key — чтобы не спамить одинаковыми строками."""
    if once_key is not None:
        if once_key in _WARNED:
            return
        _WARNED.add(once_key)
    print("warning: " + msg, file=sys.stderr)


# --------------------------------------------------------------------------
# Разбор raw.csv
# --------------------------------------------------------------------------


def elem_size(dtype: str) -> int:
    d = (dtype or "").strip().lower()
    if d in ("f32", "fp32", "float", "float32", "single"):
        return 4
    if d in ("f64", "fp64", "double", "float64"):
        return 8
    warn("неизвестный dtype %r, считаю 4 байта на элемент" % dtype, "dtype:" + d)
    return 4


def cache_level(ws_bytes: float, l1d: int, l2: int) -> str:
    """Уровень, в котором помещается рабочий набор (строго меньше размера кэша:
    набор ровно в размер кэша на практике уже не помещается)."""
    if ws_bytes < l1d:
        return "L1d"
    if ws_bytes < l2:
        return "L2"
    return "DRAM"


def check_traffic_model(mode: str, bpe: float) -> None:
    """Сверяет bytes_per_elem из CSV с ожидаемой моделью. Значение всегда берётся
    из CSV как есть — это только предупреждение о смене модели."""
    if abs(bpe - TRAFFIC_EXPECTED.get(mode, -1)) < 1e-9:
        return
    if abs(bpe - TRAFFIC_RFO_LEGACY.get(mode, -1)) < 1e-9:
        warn("mode=%s: bytes_per_elem=%g — это старая модель ТЗ с write-allocate/RFO; "
             "на M4 RFO не наблюдается, ожидается %g" % (mode, bpe, TRAFFIC_EXPECTED[mode]),
             "traffic:%s:%g" % (mode, bpe))
        return
    warn("mode=%s: неожиданный bytes_per_elem=%g (ожидается %g); считаю по значению из CSV"
         % (mode, bpe, TRAFFIC_EXPECTED.get(mode, float("nan"))), "traffic:%s:%g" % (mode, bpe))


@dataclass
class Config:
    """Агрегат по одной конфигурации: (dtype, variant, mode, N) из 11 прогонов."""

    stage: str
    variant: str
    flags_id: str
    dtype: str
    mode: str
    n: int
    bpe: float
    reps: int = 0
    times: List[float] = field(default_factory=list)
    checksums: Set[str] = field(default_factory=set)
    bitexact: Set[int] = field(default_factory=set)
    n_bitexact0: int = 0
    spread_limit: float = DEFAULT_SPREAD_LIMIT

    # производные, заполняются в finalize()
    nruns: int = 0
    tmin: float = math.inf
    tmed: float = math.inf
    tsd: float = 0.0
    spread: float = math.inf
    gflops: float = 0.0
    gibs: float = 0.0
    ws_bytes: int = 0
    cache: str = "?"

    @property
    def key(self) -> Tuple[str, str, str, int]:
        return (self.dtype, self.variant, self.mode, self.n)

    @property
    def desc(self) -> str:
        return VARIANT_DESC.get(self.variant, self.variant)

    @property
    def noisy(self) -> bool:
        return math.isfinite(self.spread) and self.spread > self.spread_limit

    def finalize(self, l1d: int, l2: int, spread_limit: float) -> None:
        self.spread_limit = spread_limit
        self.nruns = len(self.times)
        if not self.times:
            return
        self.tmin = min(self.times)
        self.tmed = statistics.median(self.times)
        self.tsd = statistics.stdev(self.times) if len(self.times) >= 2 else 0.0
        self.spread = ((max(self.times) - self.tmin) / self.tmin) if self.tmin > 0 else math.inf
        out_elems = 2.0 * self.n * self.n
        if self.tmin > 0:
            self.gflops = out_elems / self.tmin / 1e9          # FLOP = 2*N*N
            self.gibs = self.bpe * out_elems / self.tmin / GIB  # фактический трафик
        check_traffic_model(self.mode, self.bpe)
        mats = 4 if self.mode == "fused" else 3  # A,B,C(,D)
        self.ws_bytes = mats * self.n * self.n * elem_size(self.dtype)
        self.cache = cache_level(self.ws_bytes, l1d, l2)

    def roofline_sec(self, peak_gib: float) -> float:
        """t_min = bytes_per_elem * 2*N*N / Peak_BW (§8.1.3 ТЗ)."""
        if peak_gib <= 0:
            return math.inf
        return self.bpe * 2.0 * self.n * self.n / (peak_gib * GIB)


def load_raw(path: str, dtype_filter: Optional[str], l1d: int, l2: int,
             spread_limit: float) -> List[Config]:
    """Читает raw.csv и сворачивает прогоны в конфигурации. Fatal -> SystemExit(1)."""
    if not os.path.exists(path):
        print("error: нет файла с сырыми данными: %s" % path, file=sys.stderr)
        raise SystemExit(1)

    configs: Dict[Tuple[str, str, str, int], Config] = {}
    bad = 0
    total = 0
    with open(path, newline="", encoding="utf-8") as fh:
        reader = csv.DictReader(fh)
        header = reader.fieldnames or []
        missing = [c for c in RAW_COLUMNS if c not in header]
        if missing:
            print("error: в %s нет обязательных столбцов: %s\n"
                  "       ожидаемая шапка: %s" % (path, ", ".join(missing), ",".join(RAW_COLUMNS)),
                  file=sys.stderr)
            raise SystemExit(1)
        extra = [c for c in header if c not in RAW_COLUMNS]
        if extra:
            warn("в %s есть неизвестные столбцы (игнорирую): %s" % (path, ", ".join(extra)))

        for row in reader:
            total += 1
            try:
                n = int(row["N"])
                bpe = float(row["bytes_per_elem"])
                tsec = float(row["time_sec"])
                mode = (row["mode"] or "").strip()
                variant = (row["variant"] or "").strip()
                stage = (row["stage"] or "").strip()
                dtype = (row["dtype"] or "").strip()
                reps = int(float(row["reps"] or 0))
                bitexact = int(float(row["bitexact"] or 0))
            except (TypeError, ValueError):
                bad += 1
                continue
            if not variant or not mode or n <= 0 or not math.isfinite(tsec) or tsec <= 0:
                bad += 1
                continue
            if dtype_filter and dtype != dtype_filter:
                continue
            if mode not in MODE_ORDER:
                warn("неизвестный mode=%r (жду sep/fused), строка пропущена" % mode, "mode:" + mode)
                bad += 1
                continue
            key = (dtype, variant, mode, n)
            cfg = configs.get(key)
            if cfg is None:
                cfg = Config(stage=stage, variant=variant, flags_id=(row["flags_id"] or "").strip(),
                             dtype=dtype, mode=mode, n=n, bpe=bpe, reps=reps)
                configs[key] = cfg
            cfg.times.append(tsec)
            cfg.checksums.add((row["checksum"] or "").strip())
            cfg.bitexact.add(bitexact)
            if bitexact == 0:
                cfg.n_bitexact0 += 1
            if abs(cfg.bpe - bpe) > 1e-9:
                warn("конфигурация %s/%s/N=%d: разные bytes_per_elem (%g и %g)"
                     % (variant, mode, n, cfg.bpe, bpe), "bpe:%s%s%d" % (variant, mode, n))

    if bad:
        warn("пропущено некорректных строк: %d из %d" % (bad, total))
    if not configs:
        print("error: в %s нет пригодных строк%s" % (
            path, (" для dtype=%s" % dtype_filter) if dtype_filter else ""), file=sys.stderr)
        raise SystemExit(1)

    out = list(configs.values())
    for cfg in out:
        cfg.finalize(l1d, l2, spread_limit)
    return out


def pick_dtype(path: str) -> Optional[str]:
    """Определяет dtype по умолчанию: f32, если он есть, иначе единственный присутствующий."""
    seen: Dict[str, int] = {}
    try:
        with open(path, newline="", encoding="utf-8") as fh:
            for row in csv.DictReader(fh):
                d = (row.get("dtype") or "").strip()
                if d:
                    seen[d] = seen.get(d, 0) + 1
    except OSError:
        return None
    if not seen:
        return None
    for cand in ("f32", "fp32", "float", "float32"):
        if cand in seen:
            return cand
    return max(seen.items(), key=lambda kv: kv[1])[0]


# --------------------------------------------------------------------------
# Пиковая ПСП
# --------------------------------------------------------------------------


@dataclass
class Peak:
    """Пиковая ПСП одного P-ядра в той же модели трафика, что и замеры (без RFO)."""

    gib: float
    source: str           # откуда взято — печатается в отчёте
    measured: bool

    @property
    def normal_gib(self) -> float:
        return self.gib

    @property
    def nt_gib(self) -> float:
        return self.gib

    def for_cfg(self, nt: bool = False) -> float:
        # Модель трафика единая для всех этапов, поэтому знаменатель один.
        return self.gib


def load_bandwidth(path: Optional[str], fallback_gib: float, rfo_correct: bool = True) -> Peak:
    """Пик ПСП из results/bandwidth.csv (тесты add / add_nt) либо из аргумента.

    Пик берётся по МИНИМАЛЬНОМУ времени прогона (= максимум ГиБ/с). Тест `add`
    в bw_probe посчитан по модели ТЗ с write-allocate (16 Б/элем.); замеры ядер
    считаются по модели без RFO (12 Б/элем.), поэтому `add` пересчитывается
    ×12/16 — иначе `% от ПСП` и `t_min` не сойдутся. `add_nt` уже без RFO.
    """
    if abs(fallback_gib - DEFAULT_PEAK_GIB) < 1e-6:
        src = ("оценка из спецификации Apple M4 (~%.0f ГБ/с = %.2f ГиБ/с), ЭТО НЕ ЗАМЕР "
               "и это пик всей SoC, а не одного P-ядра — запустите bw_probe и передайте "
               "--bandwidth" % (DEFAULT_PEAK_GB, fallback_gib))
    else:
        src = "значение ключа --peak-gib = %.2f ГиБ/с (задано вручную, не замер)" % fallback_gib
    spec = Peak(fallback_gib, src, False)
    if not path:
        return spec
    if not os.path.exists(path):
        warn("нет файла ПСП %s — беру %s" % (path, spec.source))
        return spec
    best: Dict[str, float] = {}
    bpe_of: Dict[str, float] = {}
    try:
        with open(path, newline="", encoding="utf-8") as fh:
            reader = csv.DictReader(fh)
            header = reader.fieldnames or []
            if "test" not in header or "gib_per_sec" not in header:
                warn("в %s нет столбцов test/gib_per_sec (жду шапку: %s) — беру %s"
                     % (path, ",".join(BW_COLUMNS), spec.source))
                return spec
            for row in reader:
                try:
                    val = float(row["gib_per_sec"])
                except (TypeError, ValueError):
                    continue
                t = (row["test"] or "").strip()
                if not (math.isfinite(val) and val > 0):
                    continue
                best[t] = max(best.get(t, 0.0), val)  # max ГиБ/с == min времени
                try:
                    bpe_of[t] = float(row["bytes_per_elem"])
                except (TypeError, ValueError, KeyError):
                    pass
    except OSError as exc:
        warn("не читается %s (%s) — беру %s" % (path, exc, spec.source))
        return spec

    cand: List[Tuple[float, str]] = []
    if "add" in best:
        raw = best["add"]
        bpe = bpe_of.get("add", 16.0)
        if rfo_correct and abs(bpe - TRAFFIC_EXPECTED["sep"]) > 1e-9 and bpe > 0:
            adj = raw * TRAFFIC_EXPECTED["sep"] / bpe
            cand.append((adj, "add %.2f→%.2f ГиБ/с (пересчёт %g→%g Б/элем., без RFO)"
                         % (raw, adj, bpe, TRAFFIC_EXPECTED["sep"])))
        else:
            cand.append((raw, "add %.2f ГиБ/с" % raw))
    if "add_nt" in best:
        cand.append((best["add_nt"], "add_nt %.2f ГиБ/с (уже без RFO)" % best["add_nt"]))
    if not cand:
        warn("в %s нет тестов 'add'/'add_nt' — беру %s" % (path, spec.source))
        return spec
    cand.sort(key=lambda c: -c[0])
    peak = cand[0][0]
    src = ("измерено (%s), по минимальному времени: Peak_BW = %.2f ГиБ/с = %.1f ГБ/с; "
           "кандидаты: %s" % (os.path.basename(path), peak, peak * GIB / 1e9,
                              "; ".join(c[1] for c in cand)))
    return Peak(peak, src, True)


# --------------------------------------------------------------------------
# Форматирование
# --------------------------------------------------------------------------

DASH = "—"


def fmt_ms(t: Optional[float]) -> str:
    if t is None or not math.isfinite(t):
        return DASH
    ms = t * 1000.0
    if ms >= 1000:
        return "%.0f" % ms
    if ms >= 100:
        return "%.1f" % ms
    if ms >= 10:
        return "%.2f" % ms
    if ms >= 1:
        return "%.3f" % ms
    return "%.4g" % ms  # доли микросекунды — держим 4 значащие цифры


def fmt_x(v: Optional[float]) -> str:
    if v is None or not math.isfinite(v):
        return DASH
    return "%.2f×" % v


def fmt_f(v: Optional[float], digits: int = 2) -> str:
    if v is None or not math.isfinite(v):
        return DASH
    return ("%." + str(digits) + "f") % v


def fmt_bytes(b: float) -> str:
    if b >= 1024 ** 3:
        return "%.2f ГиБ" % (b / 1024 ** 3)
    if b >= 1024 ** 2:
        return "%.2f МиБ" % (b / 1024 ** 2)
    if b >= 1024:
        return "%.1f КиБ" % (b / 1024)
    return "%d Б" % int(b)


def md_cell(s: str) -> str:
    return str(s).replace("|", "\\|").replace("\n", " ")


def md_code(s: str) -> str:
    s = (s or "").strip()
    if not s:
        return DASH
    return "`%s`" % md_cell(s)


def md_table(headers: Sequence[str], rows: Sequence[Sequence[str]]) -> List[str]:
    out = ["| " + " | ".join(md_cell(h) for h in headers) + " |",
           "|" + "|".join("---" for _ in headers) + "|"]
    for r in rows:
        cells = list(r) + [""] * (len(headers) - len(r))
        out.append("| " + " | ".join(md_cell(c) for c in cells[:len(headers)]) + " |")
    return out


def variant_sort_key(v: str) -> Tuple[int, str]:
    try:
        return (VARIANT_ORDER.index(v), v)
    except ValueError:
        return (len(VARIANT_ORDER), v)


# --------------------------------------------------------------------------
# Выборки
# --------------------------------------------------------------------------


class Data:
    """Индексы по списку конфигураций."""

    def __init__(self, configs: List[Config]):
        self.configs = configs
        self.by_key: Dict[Tuple[str, str, int], Config] = {}
        for c in configs:
            self.by_key[(c.variant, c.mode, c.n)] = c
        self.ns: List[int] = sorted({c.n for c in configs})
        self.stages: List[str] = [s for s in STAGE_ORDER if any(c.stage == s for c in configs)]
        unknown = sorted({c.stage for c in configs} - set(STAGE_ORDER))
        if unknown:
            warn("неизвестные этапы в CSV (в порядок таблиц не попадут): %s" % ", ".join(unknown))
        self.stages += unknown
        self.modes: List[str] = [m for m in MODE_ORDER if any(c.mode == m for c in configs)]

    def get(self, variant: str, mode: str, n: int) -> Optional[Config]:
        return self.by_key.get((variant, mode, n))

    def select(self, stage: Optional[str] = None, mode: Optional[str] = None,
               n: Optional[int] = None, variant: Optional[str] = None) -> List[Config]:
        out = self.configs
        if stage is not None:
            out = [c for c in out if c.stage == stage]
        if mode is not None:
            out = [c for c in out if c.mode == mode]
        if n is not None:
            out = [c for c in out if c.n == n]
        if variant is not None:
            out = [c for c in out if c.variant == variant]
        return out

    def best(self, stage: Optional[str] = None, mode: Optional[str] = None,
             n: Optional[int] = None) -> Optional[Config]:
        """Лучшая (по минимальному времени) конфигурация в выборке."""
        cand = [c for c in self.select(stage=stage, mode=mode, n=n) if math.isfinite(c.tmin)]
        return min(cand, key=lambda c: c.tmin) if cand else None

    def variants_of(self, stage: str) -> List[str]:
        return sorted({c.variant for c in self.configs if c.stage == stage}, key=variant_sort_key)


# --------------------------------------------------------------------------
# Таблицы
# --------------------------------------------------------------------------


def section_preamble(out: List[str], data: Data, peak: Peak, args, dtype: str) -> None:
    out.append("# Результаты ДЗ №1 «Освоение векторизации», вариант 3 (C = A+B, D = A−B)")
    out.append("")
    out.append("Файл сгенерирован `scripts/report_table.py` из `%s`. Руками не править." % args.raw)
    out.append("")
    nruns = sorted({c.nruns for c in data.configs})
    out.append("- Тип данных: `%s` (%d Б/элем.); конфигураций: %d; прогонов на конфигурацию: %s."
               % (dtype, elem_size(dtype), len(data.configs),
                  ", ".join(str(x) for x in nruns)))
    out.append("- Основная цифра — **минимум** из прогонов (§6.2.3 ТЗ); рядом медиана, σ "
               "(в % от минимума) и разброс `спред = (max−min)/min`.")
    out.append("- Конфигурации с разбросом > %.0f %% помечены `*` — по §6.2.3 прогон считается "
               "зашумлённым и его следует повторить." % (args.spread_limit * 100))
    out.append("- Пиковая ПСП: **%s**." % peak.source)
    out.append("- FLOP на прогон = `2·N²` (считаются обе матрицы C и D в обоих режимах); "
               "трафик = `bytes_per_elem · 2·N²`, ГиБ/с считаются по фактическому трафику (§6.4.1). "
               "`bytes_per_elem` берётся из CSV как есть.")
    out.append("- Режимы: `sep` — два раздельных прохода, `fused` — один слитый проход; "
               "в обоих случаях считаются обе матрицы, времена сравнимы напрямую.")
    out.append("- Модель трафика единая для всех этапов: **%g Б/вых. элем. для `sep` и %g для "
               "`fused`**, независимо от NT-store. На M4 обычный store не платит за "
               "write-allocate/RFO, а `stnp` не даёт выигрыша, поэтому старая модель ТЗ "
               "(16 / 12 Б с RFO) не применяется."
               % (TRAFFIC_EXPECTED["sep"], TRAFFIC_EXPECTED["fused"]))
    out.append("")
    out.append("**Границы кэша** (P-ядро: L1d = %s, L2 = %s — бенчмарк принудительно идёт на "
               "P-ядре, у E-ядер меньше: %s / %s; рабочий набор `3·N²·%d` Б для `sep` и "
               "`4·N²·%d` Б для `fused`, «помещается» = строго меньше размера кэша):"
               % (fmt_bytes(args.l1d), fmt_bytes(args.l2), fmt_bytes(E_CORE_L1D),
                  fmt_bytes(E_CORE_L2), elem_size(dtype), elem_size(dtype)))
    out.append("")
    rows = []
    esz = elem_size(dtype)
    for n in data.ns:
        ws_sep = 3 * n * n * esz
        ws_fused = 4 * n * n * esz
        rows.append([str(n), fmt_bytes(ws_sep), cache_level(ws_sep, args.l1d, args.l2),
                     fmt_bytes(ws_fused), cache_level(ws_fused, args.l1d, args.l2)])
    out.extend(md_table(["N", "Раб. набор `sep`", "Уровень", "Раб. набор `fused`", "Уровень"], rows))
    out.append("")


def render_main_row(cfg: Config, base: Optional[Config], prev_best: Optional[Config],
                    peak: Peak) -> List[str]:
    star = "*" if cfg.noisy else ""
    sp_base = (base.tmin / cfg.tmin) if base and math.isfinite(base.tmin) else None
    sp_prev = (prev_best.tmin / cfg.tmin) if prev_best and math.isfinite(prev_best.tmin) else None
    pk = peak.for_cfg()
    pct = (cfg.gibs / pk * 100.0) if pk > 0 else None
    sd_pct = (cfg.tsd / cfg.tmin * 100.0) if cfg.tmin > 0 else float("nan")
    noise = "%s / %.1f %% / %.1f %%" % (fmt_ms(cfg.tmed), sd_pct, cfg.spread * 100.0) \
        if math.isfinite(cfg.spread) else DASH
    return [
        cfg.stage,
        "%s (`%s`)" % (cfg.desc, cfg.variant),
        md_code(cfg.flags_id),
        cfg.mode,
        fmt_ms(cfg.tmin) + star,
        noise,
        fmt_x(sp_base),
        fmt_x(sp_prev),
        fmt_f(cfg.gibs, 2),
        fmt_f(pct, 1) + " %" if pct is not None else DASH,
        cfg.cache,
    ]


MAIN_HEADERS = ["Этап", "Описание / вариант", "Ключи", "Режим", "Время, мс",
                "медиана, мс / σ / спред", "Speedup vs E0", "Speedup vs пред.", "ГиБ/с",
                "% от ПСП", "Кэш"]


def section_main(out: List[str], data: Data, peak: Peak, args, dtype: str) -> None:
    out.append("## 1. Основные таблицы по N (§10 ТЗ)")
    out.append("")
    out.append("За 1.00× принят вариант `%s`, режим `sep` — это baseline E0 (§7.1 ТЗ). "
               "Столбец «Speedup vs пред.» — относительно ЛУЧШЕГО варианта предыдущего этапа "
               "по цепочке E0 → E0' → E1 → E2 → E3 → E4, В ТОМ ЖЕ РЕЖИМЕ (если предыдущего "
               "этапа в этом режиме нет — берётся его лучший вариант по обоим режимам)."
               % args.baseline)
    out.append("")
    esz = elem_size(dtype)
    for n in data.ns:
        ws_sep, ws_fused = 3 * n * n * esz, 4 * n * n * esz
        lvl_sep = cache_level(ws_sep, args.l1d, args.l2)
        lvl_fused = cache_level(ws_fused, args.l1d, args.l2)
        out.append("### N = %d  (раб. набор: `sep` %s → %s, `fused` %s → %s)"
                   % (n, fmt_bytes(ws_sep), lvl_sep, fmt_bytes(ws_fused), lvl_fused))
        out.append("")
        base = data.get(args.baseline, "sep", n)
        if base is None:
            warn("N=%d: нет baseline %s/sep — колонка «Speedup vs E0» пуста" % (n, args.baseline))
        rows: List[List[str]] = []
        models: List[Tuple[str, float]] = []
        for stage in data.stages:
            prev = PREV_STAGE.get(stage)
            for mode in data.modes:
                cfg = data.best(stage=stage, mode=mode, n=n)
                if cfg is None:
                    continue
                # Сравниваем с тем же режимом — иначе sep мерился бы против fused.
                prev_best = (data.best(stage=prev, mode=mode, n=n)
                             or data.best(stage=prev, n=n)) if prev else None
                rows.append(render_main_row(cfg, base, prev_best, peak))
                key = (cfg.mode, cfg.bpe)
                if key not in models:
                    models.append(key)
        if not rows:
            out.append("_нет данных для N = %d_" % n)
            out.append("")
            continue
        # Строки теоретического предела (§8.1.3): по одной на каждую модель трафика в блоке.
        for mode, bpe in sorted(models, key=lambda m: (MODE_ORDER.index(m[0]), -m[1])):
            pk = peak.for_cfg()
            t = bpe * 2.0 * n * n / (pk * GIB) if pk > 0 else math.inf
            sp_base = (base.tmin / t) if base and math.isfinite(base.tmin) and t > 0 else None
            rows.append([DASH, "**теор. предел (roofline)**, %g Б/вых. элем." % bpe, DASH, mode,
                         "**%s**" % fmt_ms(t), DASH, fmt_x(sp_base), DASH, fmt_f(pk, 2),
                         "100 %", lvl_fused if mode == "fused" else lvl_sep])
        out.extend(md_table(MAIN_HEADERS, rows))
        out.append("")
    out.append("`*` — разброс прогонов > %.0f %% (§6.2.3 ТЗ), цифра ненадёжна. "
               "Теор. предел считается как `t = bytes_per_elem · 2·N² / Peak_BW`. Знаменатель "
               "один для всех этапов: модель трафика на M4 не зависит от NT-store (обычный "
               "`stp` не платит за write-allocate), поэтому сравнение E3 и E4 в ГиБ/с честное."
               % (args.spread_limit * 100))
    out.append("")
    out.append("**Про «% от ПСП» > 100 %:** знаменатель — пик ПСП **DRAM**. Для `N`, чей рабочий "
               "набор помещается в L1d/L2 (столбец «Кэш»), данные идут из кэша, полоса которого "
               "выше DRAM-овой, поэтому доля закономерно превышает 100 % и строка «теор. предел» "
               "для таких `N` — не предел, а справка. Нижняя граница времени в кэше определяется "
               "не ПСП, а шириной вектора (§8.1.4 ТЗ).")
    out.append("")


def tuning_table(out: List[str], data: Data, stage: str, title: str, hint: str, args) -> None:
    out.append(title)
    out.append("")
    variants = data.variants_of(stage)
    if not variants:
        out.append("_нет данных для этапа %s_" % stage)
        out.append("")
        warn("нет данных для этапа %s — таблица подбора пропущена" % stage)
        return
    out.append(hint)
    out.append("")
    for mode in data.modes:
        present = [v for v in variants if any(data.get(v, mode, n) for n in data.ns)]
        if not present:
            continue
        out.append("**Режим `%s`**, время лучшего прогона, мс (жирным — победитель в столбце):" % mode)
        out.append("")
        headers = ["Вариант", "Ключи"] + ["N = %d" % n for n in data.ns]
        best_in_col: Dict[int, float] = {}
        for n in data.ns:
            cand = [data.get(v, mode, n) for v in present]
            cand = [c for c in cand if c is not None and math.isfinite(c.tmin)]
            if cand:
                best_in_col[n] = min(c.tmin for c in cand)
        rows = []
        for v in present:
            flags = ""
            cells = []
            for n in data.ns:
                cfg = data.get(v, mode, n)
                if cfg is None or not math.isfinite(cfg.tmin):
                    cells.append(DASH)
                    continue
                flags = flags or cfg.flags_id
                txt = fmt_ms(cfg.tmin) + ("*" if cfg.noisy else "")
                if n in best_in_col and cfg.tmin <= best_in_col[n] * (1 + 1e-12):
                    txt = "**%s**" % txt
                cells.append(txt)
            rows.append([("`%s`" % v), md_code(flags)] + cells)
        out.extend(md_table(headers, rows))
        out.append("")
        winners = []
        for n in data.ns:
            cfg = data.best(stage=stage, mode=mode, n=n)
            if cfg is not None:
                winners.append("N = %d → `%s`" % (n, cfg.variant))
        if winners:
            out.append("Победители: " + "; ".join(winners) + ".")
            out.append("")


def section_sep_vs_fused(out: List[str], data: Data, args) -> None:
    out.append("## 5. Раздельные проходы против слитого")
    out.append("")
    out.append("Для лучшего варианта этапа в каждом режиме. «Модель» — отношение "
               "`bytes_per_elem(sep) / bytes_per_elem(fused)`: во столько раз слияние сокращает "
               "трафик, это и есть ожидаемый выигрыш для memory-bound задачи (§11.2 ТЗ).")
    out.append("")
    rows = []
    any_row = False
    for stage in [s for s in ("E2", "E3", "E4") if s in data.stages]:
        for n in data.ns:
            s_cfg = data.best(stage=stage, mode="sep", n=n)
            f_cfg = data.best(stage=stage, mode="fused", n=n)
            if s_cfg is None or f_cfg is None:
                continue
            any_row = True
            gain = s_cfg.tmin / f_cfg.tmin if f_cfg.tmin > 0 else math.inf
            model = s_cfg.bpe / f_cfg.bpe if f_cfg.bpe > 0 else math.inf
            rows.append([stage, str(n), "`%s`" % s_cfg.variant, fmt_ms(s_cfg.tmin),
                         "`%s`" % f_cfg.variant, fmt_ms(f_cfg.tmin), fmt_x(gain), fmt_x(model),
                         s_cfg.cache])
    if not any_row:
        out.append("_нет пар sep/fused — таблица пуста_")
        out.append("")
        warn("нет ни одной пары sep/fused для этапов E2/E3/E4")
        return
    out.extend(md_table(["Этап", "N", "Вариант sep", "t(sep), мс", "Вариант fused", "t(fused), мс",
                         "Выигрыш", "Модель", "Кэш (sep)"], rows))
    out.append("")


def section_summary(out: List[str], data: Data, peak: Peak, args) -> None:
    out.append("## 6. Сводка: лучший вариант этапа × N")
    out.append("")
    out.append("В ячейке: **время, мс** / ГиБ/с / вариант и режим. Это главная таблица для выводов.")
    out.append("")
    headers = ["Этап"] + ["N = %d" % n for n in data.ns]
    rows = []
    for stage in data.stages:
        cells = []
        for n in data.ns:
            cfg = data.best(stage=stage, n=n)
            if cfg is None:
                cells.append(DASH)
                continue
            cells.append("**%s** / %s<br>`%s` %s%s" % (fmt_ms(cfg.tmin), fmt_f(cfg.gibs, 1),
                                                      cfg.variant, cfg.mode,
                                                      "*" if cfg.noisy else ""))
        rows.append(["**%s** — %s" % (stage, STAGE_DESC.get(stage, ""))] + cells)
    # Теоретический предел по модели трафика лучшей конфигурации в каждой клетке.
    cells = []
    for n in data.ns:
        cfg = data.best(n=n)
        if cfg is None:
            cells.append(DASH)
            continue
        pk = peak.for_cfg()
        t = cfg.roofline_sec(pk)
        cells.append("**%s** / %s<br>%g Б/эл, %s" % (fmt_ms(t), fmt_f(pk, 1), cfg.bpe, cfg.mode))
    rows.append(["**теор. предел (roofline)** — по модели трафика лучшей клетки"] + cells)
    out.extend(md_table(headers, rows))
    out.append("")
    out.append("Ускорение лучшего варианта этапа относительно baseline `%s`/`sep`:" % args.baseline)
    out.append("")
    rows = []
    for stage in data.stages:
        cells = []
        for n in data.ns:
            base = data.get(args.baseline, "sep", n)
            cfg = data.best(stage=stage, n=n)
            if base is None or cfg is None:
                cells.append(DASH)
            else:
                cells.append(fmt_x(base.tmin / cfg.tmin))
        rows.append(["**%s**" % stage] + cells)
    out.extend(md_table(headers, rows))
    out.append("")


def section_correctness(out: List[str], data: Data, args) -> None:
    out.append("## 7. Контроль корректности (§7 ТЗ)")
    out.append("")
    # 7.1 bitexact
    broken = [c for c in data.configs if 0 in c.bitexact]
    out.append("### 7.1 Побитовое совпадение с эталоном E0")
    out.append("")
    checks = sum(c.nruns for c in data.configs)
    if not broken:
        out.append("Все %d конфигураций (%d проверок построчно) дают `bitexact = 1` — "
                   "расхождений с эталоном E0 нет (§7.2 ТЗ)." % (len(data.configs), checks))
        out.append("")
        rows = []
        for stage in data.stages:
            cfgs = data.select(stage=stage)
            rows.append([stage, str(len(cfgs)), str(sum(c.nruns for c in cfgs)),
                         str(sum(c.n_bitexact0 for c in cfgs))])
        out.extend(md_table(["Этап", "Конфигураций", "Проверок", "`bitexact = 0`"], rows))
        out.append("")
        tail = [n for n in data.ns if n % 4 != 0]
        if tail:
            out.append("Размеры с непустым хвостом (N не кратно ширине вектора 4 × fp32), "
                       "§7.5 ТЗ: %s — корректность хвоста подтверждена."
                       % ", ".join(str(n) for n in tail))
        else:
            out.append("Все N в этом файле кратны 4 (ширине вектора fp32), поэтому случай "
                       "непустого хвоста здесь не представлен — он проверяется отдельным "
                       "прогоном на N = 1023 (§7.5 ТЗ).")
        out.append("")
    else:
        out.append("**НАЙДЕНЫ расхождения — это баг, а не погрешность fp (§7.2 ТЗ):**")
        out.append("")
        rows = [[c.stage, "`%s`" % c.variant, c.mode, str(c.n), md_code(c.flags_id),
                 "%d из %d прогонов" % (c.n_bitexact0, c.nruns)]
                for c in sorted(broken, key=lambda c: (variant_sort_key(c.variant), c.mode, c.n))]
        out.extend(md_table(["Этап", "Вариант", "Режим", "N", "Ключи", "bitexact = 0"], rows))
        out.append("")
    # 7.2 checksum
    out.append("### 7.2 Контрольная сумма по N")
    out.append("")
    rows = []
    mismatched = []
    for n in data.ns:
        sums: Dict[str, List[Config]] = {}
        for c in data.select(n=n):
            for s in c.checksums:
                sums.setdefault(s, []).append(c)
        if len(sums) == 1:
            rows.append([str(n), "1", "`%s`" % next(iter(sums)), "OK"])
        else:
            mismatched.append(n)
            rows.append([str(n), str(len(sums)),
                         ", ".join("`%s`" % s for s in sorted(sums)), "**РАСХОЖДЕНИЕ**"])
    out.extend(md_table(["N", "Разных сумм", "Значение(я)", "Вердикт"], rows))
    out.append("")
    if mismatched:
        out.append("Разбор расхождений (какая сумма от каких конфигураций):")
        out.append("")
        rows = []
        for n in mismatched:
            groups: Dict[str, List[str]] = {}
            for c in sorted(data.select(n=n), key=lambda c: (variant_sort_key(c.variant), c.mode)):
                for s in c.checksums:
                    groups.setdefault(s, []).append("%s/%s" % (c.variant, c.mode))
            for s in sorted(groups, key=lambda s: -len(groups[s])):
                names = groups[s]
                shown = ", ".join("`%s`" % x for x in names[:8])
                if len(names) > 8:
                    shown += " … (всего %d)" % len(names)
                rows.append([str(n), "`%s`" % s, str(len(names)), shown])
        out.extend(md_table(["N", "checksum", "Конфигураций", "Кто"], rows))
        out.append("")
        warn("контрольные суммы расходятся при N = %s" % ", ".join(str(n) for n in mismatched))
    # 7.3 шум
    out.append("### 7.3 Зашумлённые конфигурации (разброс > %.0f %%)" % (args.spread_limit * 100))
    out.append("")
    noisy = [c for c in data.configs if c.noisy]
    if not noisy:
        out.append("Нет: у всех конфигураций `(max−min)/min` ≤ %.0f %%." % (args.spread_limit * 100))
        out.append("")
    else:
        worst = max(noisy, key=lambda c: c.spread)
        out.append("По §6.2.3 ТЗ такой прогон невалиден и его надо повторить. "
                   "Зашумлено **%d из %d** конфигураций (%.0f %%), худший разброс %.0f %% "
                   "(`%s`/%s, N = %d)."
                   % (len(noisy), len(data.configs), 100.0 * len(noisy) / len(data.configs),
                      worst.spread * 100.0, worst.variant, worst.mode, worst.n))
        out.append("")
        ordered = sorted(noisy, key=lambda c: -c.spread)
        shown = ordered[:args.noisy_rows] if args.noisy_rows > 0 else ordered
        rows = [["`%s`" % c.variant, c.mode, str(c.n), fmt_ms(c.tmin), fmt_ms(c.tmed),
                 "%.1f %%" % (c.tsd / c.tmin * 100.0 if c.tmin > 0 else float("nan")),
                 "%.1f %%" % (c.spread * 100.0), str(c.nruns)]
                for c in shown]
        out.extend(md_table(["Вариант", "Режим", "N", "min, мс", "медиана, мс", "σ, % от min",
                             "спред", "прогонов"], rows))
        out.append("")
        if len(shown) < len(ordered):
            out.append("Показаны %d худших из %d (ключ `--noisy-rows`, 0 — показать все)."
                       % (len(shown), len(ordered)))
            out.append("")
        out.append("Разбивка по N (сколько конфигураций зашумлено):")
        out.append("")
        rows = []
        for n in data.ns:
            tot = len(data.select(n=n))
            bad_n = sum(1 for c in data.select(n=n) if c.noisy)
            med = statistics.median([c.spread for c in data.select(n=n)]) * 100.0 \
                if tot else float("nan")
            rows.append([str(n), "%d / %d" % (bad_n, tot), "%.1f %%" % med])
        out.extend(md_table(["N", "Зашумлено", "Медианный спред"], rows))
        out.append("")
        warn("зашумлённых конфигураций (спред > %.0f %%): %d"
             % (args.spread_limit * 100, len(noisy)))
    # 7.4 полнота
    expected = {c.nruns for c in data.configs}
    if len(expected) > 1:
        out.append("Разное число прогонов по конфигурациям: %s — часть замеров не добрана."
                   % ", ".join(str(x) for x in sorted(expected)))
        out.append("")
        warn("разное число прогонов по конфигурациям: %s"
             % ", ".join(str(x) for x in sorted(expected)))


# --------------------------------------------------------------------------
# main
# --------------------------------------------------------------------------


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        prog="report_table.py",
        description="Генерация markdown-таблиц отчёта (§10 ТЗ) из results/raw.csv. "
                    "Только стандартная библиотека Python.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    p.add_argument("--raw", default="results/raw.csv", help="сырые замеры, CSV со схемой §6.4 ТЗ")
    p.add_argument("--out", default="results/table.md", help="куда писать markdown")
    p.add_argument("--bandwidth", default=None,
                   help="results/bandwidth.csv от bw_probe: пик ПСП берётся из тестов add / add_nt")
    p.add_argument("--peak-gib", type=float, default=DEFAULT_PEAK_GIB,
                   help="запасной пик ПСП, ГиБ/с (используется, если нет --bandwidth); "
                        "по умолчанию — оценка ~%.0f ГБ/с из спецификации, НЕ замер" % DEFAULT_PEAK_GB)
    p.add_argument("--baseline", default="e0_O0",
                   help="вариант-baseline для столбца «Speedup vs E0» (режим sep)")
    p.add_argument("--dtype", default=None,
                   help="какой dtype разбирать (по умолчанию f32, если он есть в CSV)")
    p.add_argument("--bw-rfo-correct", action=argparse.BooleanOptionalAction, default=True,
                   help="пересчитывать тест add из bandwidth.csv в модель без RFO (×12/16)")
    p.add_argument("--l1d", type=int, default=DEFAULT_L1D,
                   help="размер L1d P-ядра, байт (у E-ядра %d)" % E_CORE_L1D)
    p.add_argument("--l2", type=int, default=DEFAULT_L2,
                   help="размер L2 кластера P-ядер, байт (у E-кластера %d)" % E_CORE_L2)
    p.add_argument("--spread-limit", type=float, default=DEFAULT_SPREAD_LIMIT,
                   help="порог (max−min)/min, выше которого прогон помечается как зашумлённый")
    p.add_argument("--noisy-rows", type=int, default=30,
                   help="сколько худших зашумлённых конфигураций выписывать в §7.3 (0 — все)")
    return p


def main(argv: Optional[Sequence[str]] = None) -> int:
    args = build_parser().parse_args(argv)
    if not os.path.exists(args.raw):
        print("error: нет файла с сырыми данными: %s" % args.raw, file=sys.stderr)
        return 1
    dtype = args.dtype or pick_dtype(args.raw)
    if dtype is None:
        warn("не удалось определить dtype — беру все строки")
    configs = load_raw(args.raw, dtype, args.l1d, args.l2, args.spread_limit)
    dtype = dtype or (configs[0].dtype or "f32")
    data = Data(configs)
    peak = load_bandwidth(args.bandwidth, args.peak_gib, args.bw_rfo_correct)

    out: List[str] = []
    section_preamble(out, data, peak, args, dtype)
    section_main(out, data, peak, args, dtype)
    tuning_table(out, data, "E1", "## 2. Подбор ключей компилятора, этап E1 (§10.1 ТЗ)",
                 "Исходник один и тот же, меняются только ключи.", args)
    tuning_table(out, data, "E2",
                 "## 3. Подбор `vectorize_width × interleave_count`, этап E2 (§10.2 ТЗ)",
                 "Ширина вектора фиксирована (NEON 128 бит = 4 × fp32), варьируется "
                 "`interleave_count` в `#pragma clang loop`.", args)
    out.append("## 4. Подбор глубины разворота (E3) и дистанции префетча (E4) (§10.3 ТЗ)")
    out.append("")
    tuning_table(out, data, "E3", "### 4.1 Глубина разворота, этап E3",
                 "NEON intrinsics; `UNROLL=k` даёт `4k` элементов за итерацию.", args)
    tuning_table(out, data, "E4", "### 4.2 Дистанция программного префетча, этап E4",
                 "NEON + non-temporal store; `PF_DIST=0` — без программного префетча. "
                 "Большие дистанции — содержательный отрицательный результат (префетч уезжает "
                 "за пределы полезного окна и мешает аппаратному), а не выброс: строки "
                 "приводятся полностью.", args)
    section_sep_vs_fused(out, data, args)
    section_summary(out, data, peak, args)
    section_correctness(out, data, args)

    text = "\n".join(out).rstrip() + "\n"
    out_dir = os.path.dirname(os.path.abspath(args.out))
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)
    try:
        with open(args.out, "w", encoding="utf-8") as fh:
            fh.write(text)
    except OSError as exc:
        print("error: не могу записать %s: %s" % (args.out, exc), file=sys.stderr)
        return 1
    print("report_table: %s — %d конфигураций, N = %s, этапы: %s"
          % (args.out, len(configs), ", ".join(str(n) for n in data.ns), ", ".join(data.stages)),
          file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
