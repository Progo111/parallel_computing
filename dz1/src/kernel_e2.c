/* kernel_e2.c — ЭТАП E2: полуавтоматическая векторизация (ТЗ §4, E2.1–E2.6).
 *
 * Собирается 4 раза (см. Makefile / VARIANT_LIST в common.h):
 *   e2_w4i1  -O3 -mcpu=apple-m4 -DVEC_WIDTH=4 -DINTERLEAVE=1
 *   e2_w4i2  -O3 -mcpu=apple-m4 -DVEC_WIDTH=4 -DINTERLEAVE=2
 *   e2_w4i4  -O3 -mcpu=apple-m4 -DVEC_WIDTH=4 -DINTERLEAVE=4
 *   e2_w4i8  -O3 -mcpu=apple-m4 -DVEC_WIDTH=4 -DINTERLEAVE=8
 *
 * Что проверяется этим файлом (четыре независимые правки относительно E1,
 * вся векторизация по-прежнему — работа компилятора, интринсиков тут нет):
 *   E2.1  RESTRICT на всех указателях  -> снимает runtime-проверку перекрытия
 *         (memcheck), которую clang вставляет на E1 перед векторным циклом.
 *   E2.2  ОДИН линейный цикл по total = n*n вместо двойного i/j: матрица
 *         плотная и непрерывная, пересчёт i*n не нужен, анализ векторизатору
 *         проще, индукционная переменная одна.
 *   E2.3  ASSUME_ALIGNED на входе -> компилятор вправе использовать
 *         выровненные векторные обращения (пары ldp/stp q) вместо ldur/stur
 *         и не строить пролог довыравнивания.
 *   E2.4  #pragma clang loop vectorize(enable) vectorize_width(VEC_WIDTH)
 *                             interleave_count(INTERLEAVE)
 *         — ширина вектора и глубина интерливинга задаются ключами -D,
 *         чтобы замерить сетку {4} x {1,2,4,8} (ТЗ §10.2).
 *   E2.5  Горячий цикл идёт строго по блоку, кратному VEC_WIDTH*INTERLEAVE;
 *         хвост (n*n % блок) обрабатывается отдельным скалярным циклом ВНЕ
 *         горячего цикла. Корректность сохраняется для любых n, включая
 *         n = 1023 (n*n = 1046529 — нечётное, не кратно ширине вектора).
 *   E2.6  Слитый mat_addsub: a[k] и b[k] читаются ОДИН раз, пишутся c[k] и
 *         d[k] — трафик падает с (3+3)*N^2*4 до 4*N^2*4 байт (ТЗ §8.1.1).
 *
 * Арифметика поэлементная, одна операция на элемент, переассоциации нет =>
 * результат побитово совпадает с эталоном E0.
 */

#include "common.h"

/* Дефолты, чтобы файл собирался и без -D (значения по умолчанию из ТЗ §4 E2.4). */
#ifndef VEC_WIDTH
#define VEC_WIDTH 4
#endif
#ifndef INTERLEAVE
#define INTERLEAVE 4
#endif

/* Число элементов, обрабатываемых за одну итерацию развёрнутого векторного
 * цикла. Главный цикл идёт только по кратному этому числу префиксу. */
#define DZ_BLOCK ((size_t)(VEC_WIDTH) * (size_t)(INTERLEAVE))

/* C = A + B */
void MAT_ADD_FN(KERNEL_TAG)(const real_t *RESTRICT a, const real_t *RESTRICT b,
                            real_t *RESTRICT c, size_t n)
{
    /* E2.3: __builtin_assume_aligned возвращает void*, тип восстанавливаем
     * явным приведением, const не теряем. */
    const real_t *RESTRICT pa = (const real_t *RESTRICT)ASSUME_ALIGNED(a);
    const real_t *RESTRICT pb = (const real_t *RESTRICT)ASSUME_ALIGNED(b);
    real_t *RESTRICT       pc = (real_t *RESTRICT)ASSUME_ALIGNED(c);

    const size_t total = n * n;              /* E2.2: один линейный проход */
    const size_t body  = total - total % DZ_BLOCK;   /* E2.5: кратный блок */

#pragma clang loop vectorize(enable) vectorize_width(VEC_WIDTH) interleave_count(INTERLEAVE)
    for (size_t k = 0; k < body; ++k) {
        pc[k] = pa[k] + pb[k];
    }

    /* Скалярный хвост — вне горячего цикла, не более VEC_WIDTH*INTERLEAVE-1
     * итераций. Тройка vectorize/interleave/unroll(disable) нужна, чтобы clang
     * не строил здесь ещё один (заведомо недостижимый) векторный эпилог и его
     * SLP-версию: горячий цикл + ровно один скалярный хвост, листинг читаемый. */
    #pragma clang loop vectorize(disable) interleave(disable) unroll(disable)
    for (size_t k = body; k < total; ++k) {
        pc[k] = pa[k] + pb[k];
    }
}

/* D = A - B */
void MAT_SUB_FN(KERNEL_TAG)(const real_t *RESTRICT a, const real_t *RESTRICT b,
                            real_t *RESTRICT c, size_t n)
{
    const real_t *RESTRICT pa = (const real_t *RESTRICT)ASSUME_ALIGNED(a);
    const real_t *RESTRICT pb = (const real_t *RESTRICT)ASSUME_ALIGNED(b);
    real_t *RESTRICT       pc = (real_t *RESTRICT)ASSUME_ALIGNED(c);

    const size_t total = n * n;
    const size_t body  = total - total % DZ_BLOCK;

#pragma clang loop vectorize(enable) vectorize_width(VEC_WIDTH) interleave_count(INTERLEAVE)
    for (size_t k = 0; k < body; ++k) {
        pc[k] = pa[k] - pb[k];
    }

    /* Скалярный хвост — вне горячего цикла (см. комментарий в mat_add). */
    #pragma clang loop vectorize(disable) interleave(disable) unroll(disable)
    for (size_t k = body; k < total; ++k) {
        pc[k] = pa[k] - pb[k];
    }
}

/* Слитый проход: C = A + B и D = A - B за ОДИН обход A и B.
 * Ключевой пункт по трафику: a[k] и b[k] читаются ровно один раз. */
void MAT_ADDSUB_FN(KERNEL_TAG)(const real_t *RESTRICT a, const real_t *RESTRICT b,
                               real_t *RESTRICT c, real_t *RESTRICT d, size_t n)
{
    const real_t *RESTRICT pa = (const real_t *RESTRICT)ASSUME_ALIGNED(a);
    const real_t *RESTRICT pb = (const real_t *RESTRICT)ASSUME_ALIGNED(b);
    real_t *RESTRICT       pc = (real_t *RESTRICT)ASSUME_ALIGNED(c);
    real_t *RESTRICT       pd = (real_t *RESTRICT)ASSUME_ALIGNED(d);

    const size_t total = n * n;
    const size_t body  = total - total % DZ_BLOCK;

#pragma clang loop vectorize(enable) vectorize_width(VEC_WIDTH) interleave_count(INTERLEAVE)
    for (size_t k = 0; k < body; ++k) {
        const real_t va = pa[k];
        const real_t vb = pb[k];
        pc[k] = va + vb;
        pd[k] = va - vb;
    }

    /* Скалярный хвост — вне горячего цикла (см. комментарий в mat_add). */
    #pragma clang loop vectorize(disable) interleave(disable) unroll(disable)
    for (size_t k = body; k < total; ++k) {
        const real_t va = pa[k];
        const real_t vb = pb[k];
        pc[k] = va + vb;
        pd[k] = va - vb;
    }
}
