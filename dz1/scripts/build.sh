#!/usr/bin/env bash
# build.sh — сборка всех вариантов E0..E4 (19 объектников из 4 исходников),
# ассемблерных листингов и отчётов векторизатора.
#
# Критерий приёмки §3 ТЗ: сборка проходит без единого предупреждения
# при -Wall -Wextra.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

JOBS="${JOBS:-$(sysctl -n hw.ncpu)}"
LOG=build/build.log

mkdir -p build asm results/opt-records

echo "==> clean"
make clean >/dev/null 2>&1 || true

echo "==> compile (make -j$JOBS), log: $LOG"
set +e
make -j"$JOBS" all 2>&1 | tee "$LOG"
status=${PIPESTATUS[0]}
set -e
if [ "$status" -ne 0 ]; then
    echo "!! СБОРКА УПАЛА, см. $LOG" >&2
    exit "$status"
fi

warn_count=$(grep -c -E 'warning:' "$LOG" || true)
if [ "$warn_count" -gt 0 ]; then
    echo "!! предупреждений компилятора: $warn_count (ТЗ требует 0)" >&2
    grep -E 'warning:' "$LOG" | head -40 >&2
    exit 1
fi
echo "    предупреждений: 0 — ок"

echo "==> ассемблерные листинги (make asm)"
make -j"$JOBS" asm >/dev/null

echo "==> отчёты векторизатора (make optrec)"
make optrec >/dev/null

echo "==> готово"
ls -la build/bench
echo "объектников ядер: $(ls build/e*.o | wc -l | tr -d ' ')"
echo "листингов asm:    $(ls asm/*.s | wc -l | tr -d ' ')"
