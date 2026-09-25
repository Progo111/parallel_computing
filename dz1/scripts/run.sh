#!/usr/bin/env bash
# run.sh — полный воспроизводимый прогон: окружение -> ПСП -> свип по N ->
# таблицы -> графики. Все числа отчёта порождаются отсюда (§13.8 ТЗ).
#
#   ./scripts/build.sh && ./scripts/run.sh
#
# Переменные окружения:
#   SIZES  — список N через запятую (по умолчанию свип L1/L2/DRAM)
#   RUNS   — число замеров на конфигурацию (по умолчанию 11)
#   QUICK  — 1: укороченный прогон для отладки (мало N, 3 замера)
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

SIZES="${SIZES:-64,96,256,512,1024,1536,2048,4096,8192}"
RUNS="${RUNS:-11}"
if [ "${QUICK:-0}" = "1" ]; then
    SIZES="${SIZES_QUICK:-64,512,4096}"
    RUNS=3
fi

RAW=results/raw.csv
mkdir -p results report/img

if [ ! -x build/bench ]; then
    echo "!! build/bench нет — сначала ./scripts/build.sh" >&2
    exit 1
fi

echo "==> 1/5 окружение -> results/env.txt"
bash scripts/sysinfo.sh >/dev/null

echo "==> 2/5 пиковая ПСП -> results/bandwidth.{txt,csv}"
if [ ! -f results/bandwidth.csv ] || [ "${FORCE_BW:-0}" = "1" ]; then
    bash scripts/bw_probe.sh >/dev/null
else
    echo "    results/bandwidth.csv уже есть (FORCE_BW=1 чтобы перемерить)"
fi

echo "==> 3/5 свип по N: $SIZES, замеров: $RUNS -> $RAW"
echo "    (прогон длительный; питание — от сети, фоновые задачи закрыть)"
./build/bench --sizes "$SIZES" --runs "$RUNS" --out "$RAW"

echo "==> 4/5 таблицы -> results/table.md"
python3 scripts/report_table.py --raw "$RAW" --out results/table.md \
        --bandwidth results/bandwidth.csv

echo "==> 5/5 графики -> report/img/*.svg"
python3 scripts/plot_svg.py --raw "$RAW" --outdir report/img \
        --bandwidth results/bandwidth.csv

echo "==> готово"
wc -l "$RAW"
ls -1 report/img
