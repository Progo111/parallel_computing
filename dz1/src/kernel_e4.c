/* E4 — оптимизация трафика памяти поверх лучшего варианта E3 (разворот на
   4 вектора = 16 элементов за итерацию). Добавлены два приёма:
     - non-temporal store: запись мимо кэша, строка приёмника не читается
       под write-allocate и не вытесняет из кэша полезные данные;
     - программный префетч потоков A и B на PF_DIST элементов вперёд
       (при PF_DIST = 0 не подставляется вообще — это вариант «только NT»).
   Численно результат тот же: NT-store меняет только политику кэширования. */
#include <arm_neon.h>

#include "common.h"

#ifndef PF_DIST
#define PF_DIST 0
#endif

#define BLOCK 16u

/* Инструкция stnp на AArch64 пишет пару регистров, поэтому NT-запись делается
   сразу для двух соседних векторов: на одиночном 128-битном значении clang
   расщепляет вектор и выдаёт stnp d вместо stnp q. */
typedef real_t ntpair_t __attribute__((vector_size(32), aligned(4)));

static inline void nt_store_pair(real_t *p, float32x4_t lo, float32x4_t hi)
{
    ntpair_t v = __builtin_shufflevector(lo, hi, 0, 1, 2, 3, 4, 5, 6, 7);
    __builtin_nontemporal_store(v, (ntpair_t *)p);
}

#if PF_DIST > 0
#define PREFETCH(pa, pb, k)                            \
    do {                                               \
        __builtin_prefetch((pa) + (k) + PF_DIST, 0, 0); \
        __builtin_prefetch((pb) + (k) + PF_DIST, 0, 0); \
    } while (0)
#else
#define PREFETCH(pa, pb, k) ((void)0)
#endif

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
        PREFETCH(pa, pb, k);

        float32x4_t a0 = vld1q_f32(pa + k), a1 = vld1q_f32(pa + k + 4);
        float32x4_t a2 = vld1q_f32(pa + k + 8), a3 = vld1q_f32(pa + k + 12);
        float32x4_t b0 = vld1q_f32(pb + k), b1 = vld1q_f32(pb + k + 4);
        float32x4_t b2 = vld1q_f32(pb + k + 8), b3 = vld1q_f32(pb + k + 12);

        nt_store_pair(pc + k, vaddq_f32(a0, b0), vaddq_f32(a1, b1));
        nt_store_pair(pc + k + 8, vaddq_f32(a2, b2), vaddq_f32(a3, b3));
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
        PREFETCH(pa, pb, k);

        float32x4_t a0 = vld1q_f32(pa + k), a1 = vld1q_f32(pa + k + 4);
        float32x4_t a2 = vld1q_f32(pa + k + 8), a3 = vld1q_f32(pa + k + 12);
        float32x4_t b0 = vld1q_f32(pb + k), b1 = vld1q_f32(pb + k + 4);
        float32x4_t b2 = vld1q_f32(pb + k + 8), b3 = vld1q_f32(pb + k + 12);

        nt_store_pair(pc + k, vsubq_f32(a0, b0), vsubq_f32(a1, b1));
        nt_store_pair(pc + k + 8, vsubq_f32(a2, b2), vsubq_f32(a3, b3));
    }

    TAIL
    for (; k < total; ++k) pc[k] = pa[k] - pb[k];
}

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
        PREFETCH(pa, pb, k);

        float32x4_t a0 = vld1q_f32(pa + k), a1 = vld1q_f32(pa + k + 4);
        float32x4_t a2 = vld1q_f32(pa + k + 8), a3 = vld1q_f32(pa + k + 12);
        float32x4_t b0 = vld1q_f32(pb + k), b1 = vld1q_f32(pb + k + 4);
        float32x4_t b2 = vld1q_f32(pb + k + 8), b3 = vld1q_f32(pb + k + 12);

        nt_store_pair(pc + k, vaddq_f32(a0, b0), vaddq_f32(a1, b1));
        nt_store_pair(pc + k + 8, vaddq_f32(a2, b2), vaddq_f32(a3, b3));
        nt_store_pair(pd + k, vsubq_f32(a0, b0), vsubq_f32(a1, b1));
        nt_store_pair(pd + k + 8, vsubq_f32(a2, b2), vsubq_f32(a3, b3));
    }

    TAIL
    for (; k < total; ++k) {
        real_t x = pa[k];
        real_t y = pb[k];
        pc[k] = x + y;
        pd[k] = x - y;
    }
}
