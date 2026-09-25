/* common.h — единый контракт проекта ДЗ №1 (вариант 3: C = A+B, D = A-B).
 *
 * Здесь и только здесь:
 *   - тип данных real_t (смена точности в одном месте);
 *   - параметры платформы (Apple M4, измерены sysctl);
 *   - прототипы ядер всех этапов E0..E4;
 *   - X-macro список вариантов VARIANT_LIST — единственный источник правды
 *     для bench.c (реестр) и Makefile (набор целей).
 *
 * Все ядра имеют одну и ту же сигнатуру. Параметр n — РАЗМЕРНОСТЬ матрицы N,
 * ядро обрабатывает n*n элементов.
 */
#ifndef DZ1_COMMON_H
#define DZ1_COMMON_H

#include <stddef.h>
#include <stdint.h>

/* ---------------------------------------------------------------------------
 * Параметры вычислительной системы (sysctl, Apple M4, macOS 24.6.0).
 *
 * ВНИМАНИЕ: глобальные hw.l1dcachesize = 65536 и hw.l2cachesize = 4194304 —
 * это значения E-ядер (efficiency). Бенчмарк принудительно исполняется на
 * P-ядре (QOS_CLASS_USER_INTERACTIVE), поэтому границы свипа по N считаются
 * по hw.perflevel0.*:
 *     P-ядро: L1d = 131072 Б, L2 = 16777216 Б (общий на кластер из 4 P-ядер)
 *     E-ядро: L1d =  65536 Б, L2 =  4194304 Б
 * Ошибка здесь сдвигает границы кэш-режимов примерно вдвое и ставит
 * вертикальные линии главного графика не туда.
 * ------------------------------------------------------------------------- */
#define CACHELINE      128u
#define DZ_PAGESIZE    16384u
#define L1D_SIZE       131072u   /* hw.perflevel0.l1dcachesize (P-ядро)   */
#define L2_SIZE        16777216u /* hw.perflevel0.l2cachesize  (P-кластер) */
#define L1D_SIZE_ECORE 65536u
#define L2_SIZE_ECORE  4194304u

#define RESTRICT __restrict__
#define ASSUME_ALIGNED(p) __builtin_assume_aligned((p), CACHELINE)

/* ---------------------------------------------------------------------------
 * Тип данных
 * ------------------------------------------------------------------------- */
typedef float real_t;
#define DTYPE_NAME  "float"
#define REAL_BYTES  ((size_t)sizeof(real_t))
#define REAL_LANES  4 /* fp32-лейнов в 128-битном NEON-регистре */

/* ---------------------------------------------------------------------------
 * Именование ядер: имя собирается из тега, тег задаётся флагом -DKERNEL_TAG=...
 * Один и тот же исходник компилируется несколько раз с разными флагами и
 * разными тегами -> разные символы в одном бинаре (требование E0 vs E1).
 * ------------------------------------------------------------------------- */
#define KCAT_(a, b) a##b
#define KCAT(a, b)  KCAT_(a, b)

#define MAT_ADD_FN(tag)    KCAT(mat_add_, tag)
#define MAT_SUB_FN(tag)    KCAT(mat_sub_, tag)
#define MAT_ADDSUB_FN(tag) KCAT(mat_addsub_, tag)

/* Сигнатуры ядер (едины для всех этапов).
 * ВАЖНО: в объявлениях RESTRICT НЕ ставится. Квалификаторы верхнего уровня у
 * параметров игнорируются при проверке совместимости типов (C11 6.7.6.3p15),
 * поэтому каждое ядро решает само: E0 определяется БЕЗ restrict (это предмет
 * измерения на E2), E2/E3/E4 — С restrict. Если бы restrict стоял в общем
 * прототипе, компилятор получил бы информацию об отсутствии алиасинга уже на
 * E0/E1 и эффект от E2.1 нельзя было бы измерить. */
typedef void (*mat_binop_fn)(const real_t *a, const real_t *b,
                             real_t *c, size_t n);
typedef void (*mat_fused_fn)(const real_t *a, const real_t *b,
                             real_t *c, real_t *d, size_t n);

/* ---------------------------------------------------------------------------
 * Реестр вариантов.
 *   X(stage, tag, flags_id, desc, nt)
 *     stage    — этап отчёта ("E0", "E0'", "E1", "E2", "E3", "E4");
 *     tag      — суффикс символов и имя объектника (совпадает с Makefile);
 *     flags_id — строка ключей компиляции (идёт в CSV);
 *     desc     — человекочитаемое описание для таблицы;
 *     nt       — 1, если вариант использует non-temporal store (модель трафика).
 * ------------------------------------------------------------------------- */
#define VARIANT_LIST(X)                                                                        \
  X("E0",  e0_O0,    "-O0 -fno-vectorize -fno-slp-vectorize -fno-unroll-loops",                \
       "naive scalar 2D, no optimization",                              0)                     \
  X("E0'", e0_O1,    "-O1 -fno-vectorize -fno-slp-vectorize -fno-unroll-loops",                \
       "naive scalar 2D, honest scalar baseline",                       0)                     \
  X("E1",  e1_O2,    "-O2 -mcpu=apple-m4",                                                     \
       "same source, compiler keys only (-O2)",                         0)                     \
  X("E1",  e1_O3,    "-O3 -mcpu=apple-m4 -funroll-loops",                                      \
       "same source, compiler keys only (-O3)",                         0)                     \
  X("E1",  e1_Ofast, "-Ofast -mcpu=apple-m4 -funroll-loops",                                   \
       "same source, compiler keys only (-Ofast)",                      0)                     \
  X("E1",  e1_ffm,   "-O3 -ffast-math -mcpu=apple-m4 -funroll-loops",                          \
       "same source, compiler keys only (-O3 -ffast-math)",             0)                     \
  X("E2",  e2_w4i1,  "-O3 -mcpu=apple-m4 -DVEC_WIDTH=4 -DINTERLEAVE=1",                        \
       "restrict+align+pragma, width 4 x interleave 1",                 0)                     \
  X("E2",  e2_w4i2,  "-O3 -mcpu=apple-m4 -DVEC_WIDTH=4 -DINTERLEAVE=2",                        \
       "restrict+align+pragma, width 4 x interleave 2",                 0)                     \
  X("E2",  e2_w4i4,  "-O3 -mcpu=apple-m4 -DVEC_WIDTH=4 -DINTERLEAVE=4",                        \
       "restrict+align+pragma, width 4 x interleave 4",                 0)                     \
  X("E2",  e2_w4i8,  "-O3 -mcpu=apple-m4 -DVEC_WIDTH=4 -DINTERLEAVE=8",                        \
       "restrict+align+pragma, width 4 x interleave 8",                 0)                     \
  X("E3",  e3_u1,    "-O3 -mcpu=apple-m4 -DUNROLL=1",                                          \
       "NEON intrinsics, unroll x1 (4 elem/iter)",                      0)                     \
  X("E3",  e3_u2,    "-O3 -mcpu=apple-m4 -DUNROLL=2",                                          \
       "NEON intrinsics, unroll x2 (8 elem/iter)",                      0)                     \
  X("E3",  e3_u4,    "-O3 -mcpu=apple-m4 -DUNROLL=4",                                          \
       "NEON intrinsics, unroll x4 (16 elem/iter)",                     0)                     \
  X("E3",  e3_u8,    "-O3 -mcpu=apple-m4 -DUNROLL=8",                                          \
       "NEON intrinsics, unroll x8 (32 elem/iter)",                     0)                     \
  X("E4",  e4_d0,    "-O3 -mcpu=apple-m4 -DPF_DIST=0",                                         \
       "NEON + non-temporal store, no sw prefetch",                     1)                     \
  X("E4",  e4_d64,   "-O3 -mcpu=apple-m4 -DPF_DIST=64",                                        \
       "NEON + NT-store + prefetch dist 64 elem",                       1)                     \
  X("E4",  e4_d128,  "-O3 -mcpu=apple-m4 -DPF_DIST=128",                                       \
       "NEON + NT-store + prefetch dist 128 elem",                      1)                     \
  X("E4",  e4_d256,  "-O3 -mcpu=apple-m4 -DPF_DIST=256",                                       \
       "NEON + NT-store + prefetch dist 256 elem",                      1)                     \
  X("E4",  e4_d512,  "-O3 -mcpu=apple-m4 -DPF_DIST=512",                                       \
       "NEON + NT-store + prefetch dist 512 elem",                      1)                     \
  X("E4",  e4_d1024, "-O3 -mcpu=apple-m4 -DPF_DIST=1024",                                      \
       "NEON + NT-store + prefetch dist 1024 elem",                     1)                     \
  X("E4",  e4_d2048, "-O3 -mcpu=apple-m4 -DPF_DIST=2048",                                      \
       "NEON + NT-store + prefetch dist 2048 elem",                     1)

/* Объявления всех ядер, порождённые из того же списка. */
#define DZ_DECLARE_VARIANT(stage, tag, flags, desc, nt)                                        \
  void MAT_ADD_FN(tag)(const real_t *a, const real_t *b, real_t *c, size_t n);                 \
  void MAT_SUB_FN(tag)(const real_t *a, const real_t *b, real_t *c, size_t n);                 \
  void MAT_ADDSUB_FN(tag)(const real_t *a, const real_t *b,                                    \
                          real_t *c, real_t *d, size_t n);

VARIANT_LIST(DZ_DECLARE_VARIANT)
#undef DZ_DECLARE_VARIANT

/* ---------------------------------------------------------------------------
 * Модель трафика памяти (байт на один ВЫХОДНОЙ элемент).
 *
 * Учебная модель предполагает write-allocate / RFO: строка приёмника перед
 * полной перезаписью сначала читается из DRAM, давая +4 Б на элемент
 * (16 вместо 12 для раздельных проходов), а non-temporal store этот трафик
 * убирает и даёт ожидаемые +33 %.
 *
 * НА ЭТОЙ МАШИНЕ ЭТО НЕ ТАК — проверено двумя независимыми замерами:
 *   1) STREAM-пробник (results/bandwidth.txt): add = 13.22 мс против
 *      add_nt = 13.13 мс на одинаковом объёме — разница 0.7 %, в пределах шума,
 *      хотя `stnp` в коде реально присутствует (подтверждено по asm);
 *   2) чистая потоковая запись 64 МиБ идёт 1.03 мс и обычным `stp`, и `stnp`;
 *      при наличии RFO фактический трафик DRAM составил бы 130 ГБ/с, что выше
 *      теоретического пика LPDDR5X у M4 (~120 ГБ/с) — физически невозможно.
 * Вывод: RFO на полностью перезаписываемой строке M4 устраняет аппаратно,
 * посылка E4.1 на Apple Silicon не работает. Подробный разбор — §8 отчёта.
 *
 * Поэтому модель едина для всех этапов, параметр nt на неё не влияет
 * (оставлен в сигнатуре: он документирует, что вариант реально использует
 * NT-store, и по нему строится сравнение «NT против обычной записи»):
 *   раздельные проходы : 4(a) + 4(b) + 4(store c)              = 12 Б/вых.элем
 *   слитый проход      : (4(a) + 4(b) + 4(c) + 4(d)) / 2 вых.  =  8 Б/вых.элем
 * Выходных элементов 2*N*N в обоих режимах (считаются и C, и D), поэтому
 * времена и достигнутая ПСП напрямую сравнимы между режимами и этапами.
 *
 * ВАЖНО: знаменатель roofline обязан быть в той же модели. Измеренная ПСП
 * одного P-ядра по этой модели ≈ 61 ГБ/с (results/bandwidth.txt).
 * ------------------------------------------------------------------------- */
static inline double dz_bytes_per_elem(int fused, int nt)
{
    (void)nt; /* см. комментарий выше: на M4 NT-store трафик не меняет */
    return fused ? 8.0 : 12.0;
}

#endif /* DZ1_COMMON_H */
