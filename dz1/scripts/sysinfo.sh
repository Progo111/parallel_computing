#!/usr/bin/env bash
# scripts/sysinfo.sh — снимок вычислительной системы и компилятора (§2.2 ТЗ).
#
# Пишет results/env.txt. Идемпотентен, sudo не требуется.
# Запуск:  bash scripts/sysinfo.sh
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="$ROOT/results"
OUT="$OUT_DIR/env.txt"
mkdir -p "$OUT_DIR"

hr() { printf '%s\n' "------------------------------------------------------------------------"; }
sec() { hr; printf '== %s\n' "$1"; hr; }

# Безопасный sysctl: не падает под set -e, если ключа нет.
sv() { sysctl -n "$1" 2>/dev/null || echo "<absent>"; }

# Всё тело отчёта — в функции; она печатает в stdout, а вызов ниже
# дублирует вывод в results/env.txt через tee.
main() {
# ---------------------------------------------------------------- шапка -----
printf '========================================================================\n'
printf ' ДЗ №1 «Освоение векторизации», вариант 3 (C = A+B, D = A-B)\n'
printf ' Снимок вычислительной системы — results/env.txt\n'
printf ' Дата прогона : %s\n' "$(date '+%Y-%m-%d %H:%M:%S %Z')"
printf ' Хост         : %s\n' "$(hostname -s 2>/dev/null || echo '<unknown>')"
printf ' Пользователь  : %s\n' "${USER:-$(id -un)}"
printf ' Скрипт       : scripts/sysinfo.sh\n'
printf '========================================================================\n'

# ------------------------------------------------------------- ОС / ядро ----
sec "1. Операционная система (sw_vers)"
sw_vers

sec "2. Ядро (uname -a)"
uname -a

# -------------------------------------------------------------- компилятор --
sec "3. Компилятор (clang --version)"
clang --version

printf '\nclang       : %s\n' "$(command -v clang || echo '<not found>')"
printf 'xcode-select: %s\n' "$(xcode-select -p 2>/dev/null || echo '<not configured>')"
printf 'SDK path    : %s\n' "$(xcrun --show-sdk-path 2>/dev/null || echo '<n/a>')"
printf 'SDK version : %s\n' "$(xcrun --show-sdk-version 2>/dev/null || echo '<n/a>')"
printf 'make        : %s\n' "$(make --version 2>/dev/null | head -1 || echo '<n/a>')"

# ------------------------------------------------------------------- CPU ----
sec "4. Процессор (machdep.cpu)"
printf 'machdep.cpu.brand_string : %s\n' "$(sv machdep.cpu.brand_string)"
printf 'machdep.cpu.core_count   : %s\n' "$(sv machdep.cpu.core_count)"
printf 'machdep.cpu.thread_count : %s\n' "$(sv machdep.cpu.thread_count)"

sec "5. Топология, кэши, память (sysctl hw.*, отсортировано)"
sysctl -a 2>/dev/null \
  | grep -E 'hw\.(perflevel[0-9]*\.|l1[di]cachesize|l2cachesize|l3cachesize|cachelinesize|pagesize|memsize|ncpu|physicalcpu|logicalcpu|cpufrequency|byteorder)' \
  | sort

sec "6. Сводка ключевых параметров (для §2 отчёта)"
CL="$(sv hw.cachelinesize)"
PG="$(sv hw.pagesize)"
MEM="$(sv hw.memsize)"
NCPU="$(sv hw.ncpu)"
printf '%-34s %s\n' "hw.cachelinesize (Б)"      "$CL"
printf '%-34s %s\n' "hw.pagesize (Б)"           "$PG"
printf '%-34s %s\n' "hw.memsize (Б)"            "$MEM"
if [ "$MEM" != "<absent>" ]; then
  printf '%-34s %s\n' "  то же в ГиБ" "$(awk -v m="$MEM" 'BEGIN{printf "%.2f", m/1073741824}')"
fi
printf '%-34s %s\n' "hw.ncpu (всего логических)" "$NCPU"
printf '%-34s %s\n' "hw.byteorder"               "$(sv hw.byteorder)  (1234 = little-endian)"
printf '\n'
printf '%-34s %s\n' "GLOBAL hw.l1dcachesize (Б)" "$(sv hw.l1dcachesize)"
printf '%-34s %s\n' "GLOBAL hw.l1icachesize (Б)" "$(sv hw.l1icachesize)"
printf '%-34s %s\n' "GLOBAL hw.l2cachesize  (Б)" "$(sv hw.l2cachesize)"
printf '%-34s %s\n' "GLOBAL hw.l3cachesize  (Б)" "$(sv hw.l3cachesize)"
printf '\n'
# Per-perflevel: на Apple Silicon глобальные hw.l*cachesize соответствуют
# E-кластеру, а P-ядра имеют СВОИ (большие) кэши — это важно для §5.2 (свип по N).
for lvl in 0 1 2 3; do
  name="$(sysctl -n "hw.perflevel${lvl}.name" 2>/dev/null || true)"
  [ -n "$name" ] || continue
  printf 'perflevel%-2s (%s)\n' "$lvl" "$name"
  printf '    %-28s %s\n' "physicalcpu"   "$(sv "hw.perflevel${lvl}.physicalcpu")"
  printf '    %-28s %s\n' "logicalcpu"    "$(sv "hw.perflevel${lvl}.logicalcpu")"
  printf '    %-28s %s\n' "l1dcachesize"  "$(sv "hw.perflevel${lvl}.l1dcachesize")"
  printf '    %-28s %s\n' "l1icachesize"  "$(sv "hw.perflevel${lvl}.l1icachesize")"
  printf '    %-28s %s\n' "l2cachesize"   "$(sv "hw.perflevel${lvl}.l2cachesize")"
  printf '    %-28s %s\n' "cpusperl2"     "$(sv "hw.perflevel${lvl}.cpusperl2")"
done
printf '\n'
FREQ="$(sv hw.cpufrequency)"
if [ "$FREQ" = "<absent>" ]; then
  printf 'hw.cpufrequency            : <absent>\n'
  printf '  ПРИМЕЧАНИЕ: на Apple Silicon sysctl НЕ публикует частоту ядер\n'
  printf '  (DVFS управляется прошивкой). Частоту P-ядер M4 (~4.4 ГГц турбо)\n'
  printf '  в отчёте следует брать из спецификации и помечать как справочную.\n'
else
  printf 'hw.cpufrequency            : %s\n' "$FREQ"
fi

# ------------------------------------------------------------- ISA-фичи -----
sec "7. ISA-расширения (hw.optional.*)"
sysctl -a 2>/dev/null | grep -E '^hw\.optional\.' | sort

sec "8. Вердикт по векторным расширениям (главный факт §2.1 ТЗ)"
has() { [ "$(sysctl -n "$1" 2>/dev/null || echo 0)" = "1" ] && echo present || echo absent; }

NEON_ST="$(has hw.optional.neon)"
# SVE в non-streaming режиме: Apple его не экспортирует ни одним ключом.
SVE_KEYS="$(sysctl -a 2>/dev/null | grep -E '^hw\.optional\..*(SVE|sve)' || true)"
if [ -n "$SVE_KEYS" ]; then SVE_ST=present; else SVE_ST=absent; fi

printf 'NEON / ASIMD (128 бит)     : %s\n' "$NEON_ST"
printf 'NEON fp16                  : %s\n' "$(has hw.optional.neon_fp16)"
printf 'NEON half-precision fp     : %s\n' "$(has hw.optional.neon_hpfp)"
printf 'SVE: %s\n' "$SVE_ST"
printf 'SME  (FEAT_SME)            : %s\n' "$(has hw.optional.arm.FEAT_SME)"
printf 'SME2 (FEAT_SME2)           : %s\n' "$(has hw.optional.arm.FEAT_SME2)"
printf 'SME max SVL (бит)          : %s\n' "$(awk -v b="$(sv hw.optional.arm.sme_max_svl_b)" 'BEGIN{if(b=="<absent>") print "<absent>"; else print b*8}')"
printf 'AVX / AVX-512 / SSE        : absent (x86-only, платформа AArch64)\n'
printf '\n'
printf 'ВЫВОД: наиболее позднее доступное ПОЛЬЗОВАТЕЛЬСКОЕ векторное расширение\n'
printf 'на этой машине — NEON/ASIMD, 128 бит (4 x fp32). SVE в обычном\n'
printf '(не-streaming) режиме отсутствует; SME2 доступен только в streaming-\n'
printf 'режиме и для поэлементного C=A+B / D=A-B нерелевантен.\n'

# ----------------------------------------------- поддержка ключей clang -----
sec "9. Поддержка ключей компилятора (компиляция 'int main(){}')"
TMPBIN="$(mktemp -t dz1_flagprobe)"
# shellcheck disable=SC2064
trap "rm -f '$TMPBIN'" EXIT

probe_flag() {
  local flag="$1" out
  if out="$(echo 'int main(){}' | clang "$flag" -x c - -o "$TMPBIN" 2>&1)"; then
    printf '  %-26s OK\n' "$flag"
  else
    printf '  %-26s FAIL\n' "$flag"
    printf '%s\n' "$out" | sed 's/^/        | /' | head -4
  fi
}

for flag in "-mcpu=apple-m4" "-mcpu=apple-m3" "-mcpu=native" "-march=armv9-a+sve"; do
  probe_flag "$flag"
done
printf '\n'
printf 'ВНИМАНИЕ: OK у "-march=armv9-a+sve" означает лишь, что ФРОНТЕНД clang\n'
printf 'принимает ключ. Железо SVE не имеет (см. §8), поэтому SVE-код на этой\n'
printf 'машине выполнить нельзя — для ДЗ используется -mcpu=apple-m4.\n'

# ------------------------------------------------------------- питание ------
sec "10. Питание и тепловой режим (влияет на частоты, §6.3 ТЗ)"
printf '%s\n' '--- pmset -g ps ---'
pmset -g ps 2>&1 || printf '<pmset -g ps недоступен>\n'
printf '\n--- pmset -g therm ---\n'
pmset -g therm 2>&1 || printf '<pmset -g therm недоступен>\n'
printf '\n--- pmset -g (lowpowermode / highpowermode) ---\n'
pmset -g 2>/dev/null | grep -Ei 'lowpowermode|highpowermode|powernap' || printf '<нет ключей power mode>\n'
printf '\n'
if pmset -g ps 2>/dev/null | grep -q "Battery Power"; then
  printf 'ПРЕДУПРЕЖДЕНИЕ: машина работает ОТ БАТАРЕИ. P-ядра могут быть\n'
  printf 'ограничены по частоте -> заниженные и нестабильные замеры.\n'
  printf 'Для финального прогона подключить питание и повторить §6.2.\n'
else
  printf 'Питание от сети (AC) — режим, пригодный для финальных замеров.\n'
fi

sec "11. Фоновая нагрузка на момент снимка"
printf 'uptime: %s\n' "$(uptime)"
printf '\nТоп-5 процессов по CPU:\n'
ps -Ao %cpu,comm -r 2>/dev/null | head -6 || true

hr
printf 'Готово. Файл: %s\n' "$OUT"
hr
}

main 2>&1 | tee "$OUT"
