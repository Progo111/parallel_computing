#!/usr/bin/env bash
# Полный прогон: сборка, замеры, таблицы и графики для отчёта.
set -e
cd "$(dirname "$0")/.."

mkdir -p results report/img
make -j"$(sysctl -n hw.ncpu)" all listings

{
    sysctl -n machdep.cpu.brand_string
    sysctl hw.ncpu hw.memsize hw.cachelinesize \
           hw.perflevel0.l1dcachesize hw.perflevel0.l2cachesize
    sw_vers
    clang --version | head -1
} > results/env.txt

./build/bench --out results/raw.csv
python3 scripts/table.py results/raw.csv > results/tables.md
python3 scripts/plot.py results/raw.csv report/img
echo "таблицы: results/tables.md, графики: report/img"
