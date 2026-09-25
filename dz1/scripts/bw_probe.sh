#!/usr/bin/env bash
# scripts/bw_probe.sh — сборка и запуск микробенчмарка ПСП (§2.2.2 ТЗ).
#
# Собирает src/bw_probe.c ОТДЕЛЬНО от основного Makefile (bw_probe не входит
# в набор ядер E0..E4 и не линкуется с bench.c).
#
# Результаты:
#   results/bandwidth.txt — человекочитаемая таблица
#   results/bandwidth.csv — сырые замеры, 11 строк на тест
#
# Запуск:  bash scripts/bw_probe.sh [--mib N] [--runs N]
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="$ROOT/src/bw_probe.c"
BUILD_DIR="$ROOT/build"
BIN="$BUILD_DIR/bw_probe"
OUT_DIR="$ROOT/results"
TXT="$OUT_DIR/bandwidth.txt"
CSV="$OUT_DIR/bandwidth.csv"

mkdir -p "$BUILD_DIR" "$OUT_DIR"

# -mcpu=apple-m4 — основной ключ проекта; если Apple clang его не знает,
# откатываемся по цепочке (см. §2.2.4 ТЗ) и сообщаем об этом.
MCPU=""
for cand in "-mcpu=apple-m4" "-mcpu=apple-m3" "-mcpu=native"; do
  if echo 'int main(){}' | clang "$cand" -x c - -o /dev/null 2>/dev/null; then
    MCPU="$cand"
    break
  fi
done
if [ -z "$MCPU" ]; then
  echo "ERROR: clang не принял ни один из -mcpu=apple-m4/apple-m3/native" >&2
  exit 1
fi
[ "$MCPU" = "-mcpu=apple-m4" ] || echo "NOTE: -mcpu=apple-m4 недоступен, используется $MCPU" >&2

CFLAGS=(-std=gnu11 -Wall -Wextra -I"$ROOT/src" -D_DARWIN_C_SOURCE -O3 "$MCPU")

echo "[bw_probe] compile: clang ${CFLAGS[*]} src/bw_probe.c -o build/bw_probe -lm"
clang "${CFLAGS[@]}" "$SRC" -o "$BIN" -lm

echo "[bw_probe] run: $BIN $* --csv results/bandwidth.csv"
echo "[bw_probe] (768 МиБ рабочего множества, ~1 мин — не трогайте машину)"

# Прогон: stdout -> и на экран, и в results/bandwidth.txt; CSV пишет сама
# программа. `nice -n -0` не используем — отрицательный nice требует sudo.
{
  echo "# scripts/bw_probe.sh — $(date '+%Y-%m-%d %H:%M:%S %Z')"
  echo "# compile: clang ${CFLAGS[*]} src/bw_probe.c -o build/bw_probe -lm"
  echo "# clang: $(clang --version | head -1)"
  echo "# power: $(pmset -g ps 2>/dev/null | head -1)"
  echo "# lowpowermode: $(pmset -g 2>/dev/null | awk '/lowpowermode/{print $2}')  (1 = ВКЛЮЧЕН, частоты урезаны)"
  echo "#"
  echo "# Методика: замеры ЧЕРЕДУЮТСЯ между тестами (внешний цикл — повтор)."
  echo "# Устойчивая величина — МИНИМУМ (воспроизводится между прогонами в пределах"
  echo "# ~1 %); медиана и разброс зависят от DVFS/троттлинга и нестабильны."
  echo "#"
} > "$TXT"

"$BIN" "$@" --csv "$CSV" | tee -a "$TXT"

echo
echo "[bw_probe] OK"
echo "  текст : $TXT"
echo "  CSV   : $CSV ($(($(wc -l < "$CSV") - 1)) строк данных)"
