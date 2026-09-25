#!/usr/bin/env bash
# asm_dump.sh — генерация ассемблерных листингов по всем вариантам и
# автоматическая вырезка ГОРЯЧЕГО ЦИКЛА (§9.2 ТЗ: в отчёт идёт тело цикла,
# а не весь файл).
#
#   ./scripts/asm_dump.sh [tag ...]     # по умолчанию — все теги из Makefile
#
# Результат:
#   asm/<tag>.s        — полный листинг (make asm)
#   asm/<tag>.hot.txt  — горячие циклы трёх ядер (add / sub / addsub)
#                        с подсчётом инструкций и элементов за итерацию
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
mkdir -p asm

TAGS=("$@")
if [ ${#TAGS[@]} -eq 0 ]; then
    # shellcheck disable=SC2207
    TAGS=($(make -s print-TAGS))
fi

for t in "${TAGS[@]}"; do
    make -s "asm/$t.s"
done

python3 - "${TAGS[@]}" <<'PY'
import re, sys, pathlib

# Хвостовой комментарий `; @name` / `; %for.body` в листингах Mach-O
# обязателен к учёту: без него ни одна метка не распознаётся.
TRAIL = r'\s*(?:;.*)?$'
FUNC_RE  = re.compile(r'^_(mat_(?:add|sub|addsub)_[A-Za-z0-9_]+):' + TRAIL)
LABEL_RE = re.compile(r'^(L[A-Za-z0-9_]+):' + TRAIL)
BRANCH_RE = re.compile(r'^\s+(b|b\.[a-z]+|cbn?z|tbn?z)\s+(?:[^,]+,\s*)*(L[A-Za-z0-9_]+)' + TRAIL)

def mnemonic(line):
    """Мнемоника инструкции или None для метки/директивы/комментария."""
    s = line.split(';')[0].strip()
    if not s or s.startswith('.') or s.endswith(':'):
        return None
    return s.split(None, 1)[0]

def loops(body):
    """Все циклы функции: (метка, начало, конец) по обратным переходам."""
    labels = {}
    for i, ln in enumerate(body):
        m = LABEL_RE.match(ln)
        if m:
            labels[m.group(1)] = i
    out = []
    for i, ln in enumerate(body):
        m = BRANCH_RE.match(ln)
        if not m:
            continue
        j = labels.get(m.group(2))
        if j is not None and j < i:
            out.append((j, i))
    return out

def hot_loop(body):
    """Горячий цикл = САМЫЙ ВНУТРЕННИЙ (важно для -O0, где вложены i и j).

    Среди внутренних предпочтение ВЕКТОРНОМУ циклу: на E1 clang оставляет
    рядом скалярную запасную ветвь (её берёт memcheck при перекрытии
    указателей), и после -funroll-loops она длиннее векторного цикла —
    выбор «просто самого длинного» вырезал в отчёт не тот цикл."""
    ls = loops(body)
    if not ls:
        return None, 0
    inner = [(j, i) for (j, i) in ls
             if not any(j < a and b < i for (a, b) in ls if (a, b) != (j, i))]
    cand = inner or ls

    def n_insn(t):
        return sum(1 for k in range(t[0], t[1] + 1) if mnemonic(body[k]))

    def n_vec(t):
        return sum(1 for k in range(t[0], t[1] + 1)
                   if (m := mnemonic(body[k])) and m.startswith(('fadd.',
                                                                'fsub.')))
    vec_loops = [t for t in cand if n_vec(t) > 0]
    best = max(vec_loops or cand, key=n_insn)
    return body[best[0]:best[1] + 1], n_insn(best)

def count(insns, pred):
    return sum(1 for ln in insns
               if (m := mnemonic(ln)) is not None and pred(m))

for tag in sys.argv[1:]:
    src = pathlib.Path(f'asm/{tag}.s')
    if not src.exists():
        print(f'!! нет {src}', file=sys.stderr)
        continue
    lines = src.read_text().splitlines()

    funcs, cur, name = {}, None, None
    for ln in lines:
        m = FUNC_RE.match(ln)
        if m:
            name, cur = m.group(1), []
            funcs[name] = cur
            continue
        if cur is not None:
            if '.cfi_endproc' in ln:
                cur = None
                continue
            cur.append(ln)

    out = [f'==== {tag} : горячие циклы ====', '']
    for name in sorted(funcs):
        loop, n = hot_loop(funcs[name])
        out.append(f'---- {name} ----')
        if loop is None:
            out.append('  (цикл не найден: либо развёрнут полностью, либо '
                       'инлайнен)')
            out.append('')
            continue
        ld  = count(loop, lambda m: m.startswith(('ldp', 'ldr', 'ldur', 'ld1')))
        st  = count(loop, lambda m: m.startswith(('stp', 'str', 'stur', 'st1',
                                                  'stnp')))
        nt  = count(loop, lambda m: m.startswith('stnp'))
        pf  = count(loop, lambda m: m.startswith('prfm'))
        # Apple-синтаксис: векторная форма — `fadd.4s v0, v1, v2`,
        # скалярная — `fadd s0, s1, s2`. Различать по точке в мнемонике.
        vadd = count(loop, lambda m: m.startswith('fadd.'))
        vsub = count(loop, lambda m: m.startswith('fsub.'))
        sadd = count(loop, lambda m: m == 'fadd')
        ssub = count(loop, lambda m: m == 'fsub')
        vec, scl = vadd + vsub, sadd + ssub
        # В слитом ядре fadd и fsub обрабатывают ОДНИ И ТЕ ЖЕ элементы
        # (C = A+B и D = A-B), поэтому элементы считаются по максимуму,
        # а не по сумме — иначе слитый вариант завышается вдвое.
        elems = max(vadd, vsub) * 4 + max(sadd, ssub)
        out.append(f'  инструкций в теле: {n}; загрузок: {ld}; записей: {st} '
                   f'(из них non-temporal stnp: {nt}); prfm: {pf}')
        out.append(f'  векторных fadd/fsub: {vadd}/{vsub}; '
                   f'скалярных: {sadd}/{ssub}')
        if elems:
            out.append(f'  элементов за итерацию: {elems}; '
                       f'инструкций на элемент: {n / elems:.2f}')
        out.append('')
        out.extend('    ' + l.rstrip() for l in loop)
        out.append('')
    dst = pathlib.Path(f'asm/{tag}.hot.txt')
    dst.write_text('\n'.join(out) + '\n')
    print(f'{dst}: ' + ', '.join(sorted(funcs)))
PY
