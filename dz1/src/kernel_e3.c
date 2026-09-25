/* kernel_e3.c — ЭТАП E3: ручная векторизация на NEON-интринсиках (ТЗ §4, E3.1–E3.5).
 *
 * Собирается 4 раза (см. Makefile / VARIANT_LIST в common.h):
 *   e3_u1   -O3 -mcpu=apple-m4 -DUNROLL=1   (4  элем./итер. =  16 Б)
 *   e3_u2   -O3 -mcpu=apple-m4 -DUNROLL=2   (8  элем./итер. =  32 Б)
 *   e3_u4   -O3 -mcpu=apple-m4 -DUNROLL=4   (16 элем./итер. =  64 Б)
 *   e3_u8   -O3 -mcpu=apple-m4 -DUNROLL=8   (32 элем./итер. = 128 Б = строка кэша)
 *
 * Что проверяется этим файлом:
 *   E3.1 — векторизация делается РУКАМИ: vld1q_f32 / vaddq_f32 / vsubq_f32 /
 *          vst1q_f32. Ни одной #pragma векторизатора здесь нет — компилятору
 *          не оставлено свободы выбора ширины вектора.
 *   E3.2 — разворот цикла по UNROLL ВЕКТОРОВ. Регистры в теле итерации
 *          независимы: сначала ВСЕ загрузки, затем ВСЕ арифметические
 *          операции, затем ВСЕ записи. Так нет цепочки зависимостей
 *          load->add->store длиной в итерацию, планировщик свободно
 *          склеивает соседние ldr q / str q в ldp q0,q1 / stp q0,q1
 *          и прячет латентность памяти. Измеряется, какой разворот лучше.
 *   E3.3 — ГЛАВНЫЙ пункт этапа: mat_addsub_* считает C = A+B и D = A-B
 *          за ОДИН проход. A и B читаются один раз вместо двух:
 *            два раздельных прохода: (4+4+4)*2 = 24 Б на пару вых. элементов;
 *            один слитый проход:      4+4+4+4  = 16 Б на пару вых. элементов.
 *          Трафик 6N^2*4 -> 4N^2*4 Б, теоретический выигрыш 1.5x.
 *   E3.4 — хвост (n*n % (4*UNROLL)) обрабатывается отдельным скалярным
 *          циклом ПОСЛЕ горячего цикла, внутри горячего цикла ветвлений нет.
 *   E3.5 — одна операция на элемент, никаких математических преобразований:
 *          результат обязан побитово совпадать со скалярным эталоном.
 *
 * RESTRICT на всех указателях (алиасинга нет -> нет runtime-memcheck),
 * ASSUME_ALIGNED сообщает о выравнивании на строку кэша (128 Б): буферы
 * аллоцированы на границе страницы 16 КиБ, значит и на 128 Б тоже.
 *
 * Параметр n — размерность матрицы N; обрабатывается n*n элементов, row-major.
 */

#include <arm_neon.h>

#include "common.h"

#ifndef UNROLL
#define UNROLL 4
#endif

/* Элементов за одну итерацию горячего цикла: UNROLL векторов по 4 fp32. */
#define BLOCK ((size_t)(REAL_LANES * UNROLL))

/* ---------------------------------------------------------------------------
 * Директива для ХВОСТОВЫХ циклов (и только для них).
 *
 * Хвост — это n*n % (4*UNROLL) элементов, заведомо меньше одного блока, вне
 * горячего цикла. ТЗ (E3.4, §9.3) требует, чтобы он обрабатывался СКАЛЯРНО,
 * и чтобы это было видно в листинге. Без директивы clang самостоятельно
 * векторизует хвостовой цикл: в asm появляются ld1/fadd v*.4s плюс ещё один
 * уровень проверок длины, и разбор листинга в отчёте («хвост скалярный»)
 * перестаёт соответствовать реальному коду.
 *
 * Почему этой директивы НЕТ на горячем цикле: там векторизовать нечего —
 * весь параллелизм задан вручную интринсиками vld1q/vaddq/vsubq/vst1q, и
 * автовекторизатору просто не над чем работать. Запрет автовекторизации
 * относится именно к горячему циклу (E3 — этап РУЧНОЙ векторизации,
 * компилятор не должен делать за нас основную работу); на хвосте директива
 * решает обратную задачу — не даёт компилятору добавить векторный код там,
 * где мы его не просили.
 * ------------------------------------------------------------------------- */
#define TAIL_SCALAR \
    _Pragma("clang loop vectorize(disable) interleave(disable) unroll(disable)")

/* C = A + B, ручная векторизация, разворот на UNROLL векторов. */
void MAT_ADD_FN(KERNEL_TAG)(const real_t *RESTRICT a, const real_t *RESTRICT b,
                            real_t *RESTRICT c, size_t n)
{
    const real_t *RESTRICT ap = ASSUME_ALIGNED(a);
    const real_t *RESTRICT bp = ASSUME_ALIGNED(b);
    real_t *RESTRICT cp = ASSUME_ALIGNED(c);

    const size_t total = n * n;
    size_t k = 0;

    for (; k + BLOCK <= total; k += BLOCK) {
        float32x4_t va[UNROLL], vb[UNROLL], vr[UNROLL];

        /* все загрузки */
        for (int u = 0; u < UNROLL; ++u) va[u] = vld1q_f32(ap + k + 4 * (size_t)u);
        for (int u = 0; u < UNROLL; ++u) vb[u] = vld1q_f32(bp + k + 4 * (size_t)u);
        /* вся арифметика (UNROLL независимых fadd v*.4s) */
        for (int u = 0; u < UNROLL; ++u) vr[u] = vaddq_f32(va[u], vb[u]);
        /* все записи */
        for (int u = 0; u < UNROLL; ++u) vst1q_f32(cp + k + 4 * (size_t)u, vr[u]);
    }

    TAIL_SCALAR
    for (; k < total; ++k) cp[k] = ap[k] + bp[k];
}

/* D = A - B, ручная векторизация, разворот на UNROLL векторов. */
void MAT_SUB_FN(KERNEL_TAG)(const real_t *RESTRICT a, const real_t *RESTRICT b,
                            real_t *RESTRICT c, size_t n)
{
    const real_t *RESTRICT ap = ASSUME_ALIGNED(a);
    const real_t *RESTRICT bp = ASSUME_ALIGNED(b);
    real_t *RESTRICT cp = ASSUME_ALIGNED(c);

    const size_t total = n * n;
    size_t k = 0;

    for (; k + BLOCK <= total; k += BLOCK) {
        float32x4_t va[UNROLL], vb[UNROLL], vr[UNROLL];

        for (int u = 0; u < UNROLL; ++u) va[u] = vld1q_f32(ap + k + 4 * (size_t)u);
        for (int u = 0; u < UNROLL; ++u) vb[u] = vld1q_f32(bp + k + 4 * (size_t)u);
        for (int u = 0; u < UNROLL; ++u) vr[u] = vsubq_f32(va[u], vb[u]);
        for (int u = 0; u < UNROLL; ++u) vst1q_f32(cp + k + 4 * (size_t)u, vr[u]);
    }

    TAIL_SCALAR
    for (; k < total; ++k) cp[k] = ap[k] - bp[k];
}

/* E3.3 — СЛИТЫЙ проход: C = A+B и D = A-B за один обход.
 * Каждый вектор A и B загружается РОВНО ОДИН раз и используется дважды
 * (fadd и fsub), поэтому трафик чтения вдвое меньше, чем у двух раздельных
 * вызовов add+sub: 4 потока по N^2*4 Б вместо 6. */
void MAT_ADDSUB_FN(KERNEL_TAG)(const real_t *RESTRICT a, const real_t *RESTRICT b,
                               real_t *RESTRICT c, real_t *RESTRICT d, size_t n)
{
    const real_t *RESTRICT ap = ASSUME_ALIGNED(a);
    const real_t *RESTRICT bp = ASSUME_ALIGNED(b);
    real_t *RESTRICT cp = ASSUME_ALIGNED(c);
    real_t *RESTRICT dp = ASSUME_ALIGNED(d);

    const size_t total = n * n;
    size_t k = 0;

    for (; k + BLOCK <= total; k += BLOCK) {
        float32x4_t va[UNROLL], vb[UNROLL], vsum[UNROLL], vdif[UNROLL];

        /* все загрузки: A и B читаются здесь единственный раз */
        for (int u = 0; u < UNROLL; ++u) va[u] = vld1q_f32(ap + k + 4 * (size_t)u);
        for (int u = 0; u < UNROLL; ++u) vb[u] = vld1q_f32(bp + k + 4 * (size_t)u);
        /* вся арифметика: по два независимых результата на пару регистров */
        for (int u = 0; u < UNROLL; ++u) vsum[u] = vaddq_f32(va[u], vb[u]);
        for (int u = 0; u < UNROLL; ++u) vdif[u] = vsubq_f32(va[u], vb[u]);
        /* все записи */
        for (int u = 0; u < UNROLL; ++u) vst1q_f32(cp + k + 4 * (size_t)u, vsum[u]);
        for (int u = 0; u < UNROLL; ++u) vst1q_f32(dp + k + 4 * (size_t)u, vdif[u]);
    }

    TAIL_SCALAR
    for (; k < total; ++k) {
        const real_t va = ap[k];
        const real_t vb = bp[k];
        cp[k] = va + vb;
        dp[k] = va - vb;
    }
}
