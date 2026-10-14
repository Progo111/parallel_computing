/* E2 — полуавтоматическая векторизация. Интринсиков нет, вектор строит
   компилятор, ему помогают четыре правки относительно E1:
     - restrict: нет проверки перекрытия указателей перед циклом;
     - assume_aligned: разрешены выровненные обращения;
     - один линейный цикл по n*n вместо двойного i/j;
     - #pragma clang loop с явной шириной вектора и интерливингом.
   Глубина интерливинга задаётся -DINTERLEAVE, хвост считается скалярно. */
#include "common.h"

#ifndef INTERLEAVE
#define INTERLEAVE 4
#endif

#define BLOCK ((size_t)4 * INTERLEAVE)

void MAT_ADD(const real_t *restrict a, const real_t *restrict b,
             real_t *restrict c, size_t n)
{
    const real_t *restrict pa = ALIGNED(a);
    const real_t *restrict pb = ALIGNED(b);
    real_t *restrict pc = ALIGNED(c);

    const size_t total = n * n;
    const size_t body = total - total % BLOCK;

#pragma clang loop vectorize(enable) vectorize_width(4) interleave_count(INTERLEAVE)
    for (size_t k = 0; k < body; ++k)
        pc[k] = pa[k] + pb[k];

#pragma clang loop vectorize(disable)
    for (size_t k = body; k < total; ++k)
        pc[k] = pa[k] + pb[k];
}

void MAT_SUB(const real_t *restrict a, const real_t *restrict b,
             real_t *restrict c, size_t n)
{
    const real_t *restrict pa = ALIGNED(a);
    const real_t *restrict pb = ALIGNED(b);
    real_t *restrict pc = ALIGNED(c);

    const size_t total = n * n;
    const size_t body = total - total % BLOCK;

#pragma clang loop vectorize(enable) vectorize_width(4) interleave_count(INTERLEAVE)
    for (size_t k = 0; k < body; ++k)
        pc[k] = pa[k] - pb[k];

#pragma clang loop vectorize(disable)
    for (size_t k = body; k < total; ++k)
        pc[k] = pa[k] - pb[k];
}

void MAT_ADDSUB(const real_t *restrict a, const real_t *restrict b,
                real_t *restrict c, real_t *restrict d, size_t n)
{
    const real_t *restrict pa = ALIGNED(a);
    const real_t *restrict pb = ALIGNED(b);
    real_t *restrict pc = ALIGNED(c);
    real_t *restrict pd = ALIGNED(d);

    const size_t total = n * n;
    const size_t body = total - total % BLOCK;

#pragma clang loop vectorize(enable) vectorize_width(4) interleave_count(INTERLEAVE)
    for (size_t k = 0; k < body; ++k) {
        real_t x = pa[k];
        real_t y = pb[k];
        pc[k] = x + y;
        pd[k] = x - y;
    }

#pragma clang loop vectorize(disable)
    for (size_t k = body; k < total; ++k) {
        real_t x = pa[k];
        real_t y = pb[k];
        pc[k] = x + y;
        pd[k] = x - y;
    }
}
