/* E3 — ручная векторизация на NEON-интринсиках (128 бит = 4 float).
   Разворот на UNROLL векторов: сначала все загрузки, потом вся арифметика,
   потом все записи — соседние обращения склеиваются в ldp/stp q, зависимости
   внутри итерации нет. Хвост n*n % (4*UNROLL) считается скалярно. */
#include <arm_neon.h>

#include "common.h"

#ifndef UNROLL
#define UNROLL 4
#endif

#define BLOCK ((size_t)4 * UNROLL)

/* Директива нужна только на хвосте: иначе clang векторизует и его,
   и в листинге вместо скалярного остатка появляется второй векторный цикл. */
#define TAIL _Pragma("clang loop vectorize(disable) unroll(disable)")

void MAT_ADD(const real_t *restrict a, const real_t *restrict b,
             real_t *restrict c, size_t n)
{
    const real_t *restrict pa = ALIGNED(a);
    const real_t *restrict pb = ALIGNED(b);
    real_t *restrict pc = ALIGNED(c);

    const size_t total = n * n;
    size_t k = 0;

    for (; k + BLOCK <= total; k += BLOCK) {
        float32x4_t va[UNROLL], vb[UNROLL], vr[UNROLL];

        for (int u = 0; u < UNROLL; ++u) va[u] = vld1q_f32(pa + k + 4u * u);
        for (int u = 0; u < UNROLL; ++u) vb[u] = vld1q_f32(pb + k + 4u * u);
        for (int u = 0; u < UNROLL; ++u) vr[u] = vaddq_f32(va[u], vb[u]);
        for (int u = 0; u < UNROLL; ++u) vst1q_f32(pc + k + 4u * u, vr[u]);
    }

    TAIL
    for (; k < total; ++k) pc[k] = pa[k] + pb[k];
}

void MAT_SUB(const real_t *restrict a, const real_t *restrict b,
             real_t *restrict c, size_t n)
{
    const real_t *restrict pa = ALIGNED(a);
    const real_t *restrict pb = ALIGNED(b);
    real_t *restrict pc = ALIGNED(c);

    const size_t total = n * n;
    size_t k = 0;

    for (; k + BLOCK <= total; k += BLOCK) {
        float32x4_t va[UNROLL], vb[UNROLL], vr[UNROLL];

        for (int u = 0; u < UNROLL; ++u) va[u] = vld1q_f32(pa + k + 4u * u);
        for (int u = 0; u < UNROLL; ++u) vb[u] = vld1q_f32(pb + k + 4u * u);
        for (int u = 0; u < UNROLL; ++u) vr[u] = vsubq_f32(va[u], vb[u]);
        for (int u = 0; u < UNROLL; ++u) vst1q_f32(pc + k + 4u * u, vr[u]);
    }

    TAIL
    for (; k < total; ++k) pc[k] = pa[k] - pb[k];
}

/* Слитый проход: каждый вектор A и B загружается один раз и идёт и в fadd,
   и в fsub. Чтений вдвое меньше, чем у пары отдельных вызовов add + sub. */
void MAT_ADDSUB(const real_t *restrict a, const real_t *restrict b,
                real_t *restrict c, real_t *restrict d, size_t n)
{
    const real_t *restrict pa = ALIGNED(a);
    const real_t *restrict pb = ALIGNED(b);
    real_t *restrict pc = ALIGNED(c);
    real_t *restrict pd = ALIGNED(d);

    const size_t total = n * n;
    size_t k = 0;

    for (; k + BLOCK <= total; k += BLOCK) {
        float32x4_t va[UNROLL], vb[UNROLL], vs[UNROLL], vd[UNROLL];

        for (int u = 0; u < UNROLL; ++u) va[u] = vld1q_f32(pa + k + 4u * u);
        for (int u = 0; u < UNROLL; ++u) vb[u] = vld1q_f32(pb + k + 4u * u);
        for (int u = 0; u < UNROLL; ++u) vs[u] = vaddq_f32(va[u], vb[u]);
        for (int u = 0; u < UNROLL; ++u) vd[u] = vsubq_f32(va[u], vb[u]);
        for (int u = 0; u < UNROLL; ++u) vst1q_f32(pc + k + 4u * u, vs[u]);
        for (int u = 0; u < UNROLL; ++u) vst1q_f32(pd + k + 4u * u, vd[u]);
    }

    TAIL
    for (; k < total; ++k) {
        real_t x = pa[k];
        real_t y = pb[k];
        pc[k] = x + y;
        pd[k] = x - y;
    }
}
