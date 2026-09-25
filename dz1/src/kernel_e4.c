/* kernel_e4.c — ЭТАП E4: оптимизация трафика памяти (ТЗ §4, E4.1–E4.2).
 *
 * Собирается 5 раз (см. Makefile / VARIANT_LIST в common.h):
 *   e4_d0      -O3 -mcpu=apple-m4 -DPF_DIST=0      только NT-store, без префетча
 *   e4_d256    -O3 -mcpu=apple-m4 -DPF_DIST=256    NT-store + префетч на 256 элем.
 *   e4_d512    -O3 -mcpu=apple-m4 -DPF_DIST=512    NT-store + префетч на 512 элем.
 *   e4_d1024   -O3 -mcpu=apple-m4 -DPF_DIST=1024   NT-store + префетч на 1024 элем.
 *   e4_d2048   -O3 -mcpu=apple-m4 -DPF_DIST=2048   NT-store + префетч на 2048 элем.
 *
 * База — лучший вариант E3: ручные NEON-интринсики, разворот на 16 элементов
 * (64 Б) за итерацию, слитый addsub. Сверху добавлены два независимых приёма.
 *
 *   E4.1 NON-TEMPORAL STORE. Обычная запись в C идёт по политике
 *        write-allocate: строка назначения сначала ЧИТАЕТСЯ из DRAM, хотя
 *        будет перезаписана целиком. Это лишние 4 Б на элемент:
 *          раздельный проход: 4(a)+4(b)+4(RFO c)+4(store c) = 16 Б/элем
 *          то же с NT-store:  4(a)+4(b)+4(store c)          = 12 Б/элем
 *          слитый проход:     (4+4 + 4+4 + 4+4)/2           = 12 Б/вых.элем
 *          слитый + NT-store: (4+4 + 4 + 4)/2               =  8 Б/вых.элем
 *        Теоретический выигрыш на DRAM-размерах: +33%.
 *        Реализация: __builtin_nontemporal_store. ВАЖНАЯ ДЕТАЛЬ КОДОГЕНЕРАЦИИ —
 *        на одиночном 128-битном значении clang порождает НЕ stnp q, а пару
 *        stnp d (расщепляет вектор пополам и добавляет лишние mov/ext).
 *        Инструкция STNP на AArch64 работает с ПАРОЙ регистров, поэтому
 *        NT-запись здесь делается сразу для ДВУХ соседних векторов через
 *        32-байтный векторный тип — тогда clang выдаёт ровно `stnp q0, q1`.
 *        См. nt_store_pair() ниже.
 *
 *   E4.2 ПРОГРАММНЫЙ ПРЕФЕТЧ. __builtin_prefetch(p, 0, 0): rw=0 — чтение,
 *        locality=0 — не оставлять строку в кэше (на AArch64 -> PRFM PLDL1STRM).
 *        Включается только при PF_DIST > 0, чтобы вариант e4_d0 был честным
 *        «только NT-store, без префетча». На Apple Silicon аппаратный
 *        префетчер сильный и выигрыша может не быть — отрицательный результат
 *        тоже идёт в таблицу отчёта (ТЗ §4, E4.2).
 *
 * Численно NT-store НИЧЕГО не меняет: это только политика кэширования,
 * записываются те же самые биты. Побитовое совпадение с эталоном сохраняется.
 *
 * Хвост (n*n % 16) — скалярно, после горячего цикла, обычной записью
 * (ради ≤15 элементов NT-store бессмысленен).
 *
 * Параметр n — размерность матрицы N; обрабатывается n*n элементов, row-major.
 */

#include <arm_neon.h>

#include "common.h"

#ifndef PF_DIST
#define PF_DIST 0
#endif

/* Разворот фиксирован: 4 вектора = 16 элементов = 64 Б за итерацию
 * (лучший вариант по итогам E3). */
#define UNROLL_VEC 4
#define BLOCK ((size_t)(REAL_LANES * UNROLL_VEC))

_Static_assert(sizeof(real_t) == 4, "kernel_e4: NT-store pair assumes fp32 lanes");

/* 32-байтный (два NEON-регистра) векторный тип для non-temporal записи.
 * aligned(4) — намеренно: тип НЕ должен навязывать выравнивание 32 Б,
 * иначе приведение указателя было бы UB на произвольном смещении.
 * На кодогенерацию это не влияет, stnp выдаётся в обоих случаях. */
typedef real_t ntpair_t __attribute__((vector_size(32), aligned(4)));

/* E4.1: non-temporal запись двух соседних векторов -> одна инструкция stnp q,q. */
static inline void nt_store_pair(real_t *p, float32x4_t lo, float32x4_t hi)
{
    const ntpair_t v = __builtin_shufflevector(lo, hi, 0, 1, 2, 3, 4, 5, 6, 7);
    __builtin_nontemporal_store(v, (ntpair_t *)p);
}

/* ---------------------------------------------------------------------------
 * Директива для ХВОСТОВЫХ циклов (и только для них).
 *
 * Хвост — это n*n % 16 элементов, заведомо меньше одного блока, вне
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

/* E4.2: пара префетчей потоков A и B; при PF_DIST == 0 раскрывается в ничто. */
#if PF_DIST > 0
#define PREFETCH_AB(ap, bp, k)                                  \
    do {                                                        \
        __builtin_prefetch((ap) + (k) + (PF_DIST), 0, 0);       \
        __builtin_prefetch((bp) + (k) + (PF_DIST), 0, 0);       \
    } while (0)
#else
#define PREFETCH_AB(ap, bp, k) ((void)0)
#endif

/* C = A + B, NEON + non-temporal store (+ опциональный префетч). */
void MAT_ADD_FN(KERNEL_TAG)(const real_t *RESTRICT a, const real_t *RESTRICT b,
                            real_t *RESTRICT c, size_t n)
{
    const real_t *RESTRICT ap = ASSUME_ALIGNED(a);
    const real_t *RESTRICT bp = ASSUME_ALIGNED(b);
    real_t *RESTRICT cp = ASSUME_ALIGNED(c);

    const size_t total = n * n;
    size_t k = 0;

    for (; k + BLOCK <= total; k += BLOCK) {
        PREFETCH_AB(ap, bp, k);

        /* все загрузки */
        const float32x4_t a0 = vld1q_f32(ap + k + 0);
        const float32x4_t a1 = vld1q_f32(ap + k + 4);
        const float32x4_t a2 = vld1q_f32(ap + k + 8);
        const float32x4_t a3 = vld1q_f32(ap + k + 12);
        const float32x4_t b0 = vld1q_f32(bp + k + 0);
        const float32x4_t b1 = vld1q_f32(bp + k + 4);
        const float32x4_t b2 = vld1q_f32(bp + k + 8);
        const float32x4_t b3 = vld1q_f32(bp + k + 12);
        /* вся арифметика */
        const float32x4_t r0 = vaddq_f32(a0, b0);
        const float32x4_t r1 = vaddq_f32(a1, b1);
        const float32x4_t r2 = vaddq_f32(a2, b2);
        const float32x4_t r3 = vaddq_f32(a3, b3);
        /* все записи, non-temporal, парами */
        nt_store_pair(cp + k + 0, r0, r1);
        nt_store_pair(cp + k + 8, r2, r3);
    }

    TAIL_SCALAR
    for (; k < total; ++k) cp[k] = ap[k] + bp[k];
}

/* D = A - B, NEON + non-temporal store (+ опциональный префетч). */
void MAT_SUB_FN(KERNEL_TAG)(const real_t *RESTRICT a, const real_t *RESTRICT b,
                            real_t *RESTRICT c, size_t n)
{
    const real_t *RESTRICT ap = ASSUME_ALIGNED(a);
    const real_t *RESTRICT bp = ASSUME_ALIGNED(b);
    real_t *RESTRICT cp = ASSUME_ALIGNED(c);

    const size_t total = n * n;
    size_t k = 0;

    for (; k + BLOCK <= total; k += BLOCK) {
        PREFETCH_AB(ap, bp, k);

        const float32x4_t a0 = vld1q_f32(ap + k + 0);
        const float32x4_t a1 = vld1q_f32(ap + k + 4);
        const float32x4_t a2 = vld1q_f32(ap + k + 8);
        const float32x4_t a3 = vld1q_f32(ap + k + 12);
        const float32x4_t b0 = vld1q_f32(bp + k + 0);
        const float32x4_t b1 = vld1q_f32(bp + k + 4);
        const float32x4_t b2 = vld1q_f32(bp + k + 8);
        const float32x4_t b3 = vld1q_f32(bp + k + 12);

        const float32x4_t r0 = vsubq_f32(a0, b0);
        const float32x4_t r1 = vsubq_f32(a1, b1);
        const float32x4_t r2 = vsubq_f32(a2, b2);
        const float32x4_t r3 = vsubq_f32(a3, b3);

        nt_store_pair(cp + k + 0, r0, r1);
        nt_store_pair(cp + k + 8, r2, r3);
    }

    TAIL_SCALAR
    for (; k < total; ++k) cp[k] = ap[k] - bp[k];
}

/* Слитый проход C = A+B, D = A-B + non-temporal store.
 * Самая выгодная комбинация по модели трафика: 8 Б на выходной элемент
 * против 16 Б у раздельных проходов с обычной записью (E0..E2). */
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
        PREFETCH_AB(ap, bp, k);

        /* A и B читаются здесь ровно один раз на оба результата */
        const float32x4_t a0 = vld1q_f32(ap + k + 0);
        const float32x4_t a1 = vld1q_f32(ap + k + 4);
        const float32x4_t a2 = vld1q_f32(ap + k + 8);
        const float32x4_t a3 = vld1q_f32(ap + k + 12);
        const float32x4_t b0 = vld1q_f32(bp + k + 0);
        const float32x4_t b1 = vld1q_f32(bp + k + 4);
        const float32x4_t b2 = vld1q_f32(bp + k + 8);
        const float32x4_t b3 = vld1q_f32(bp + k + 12);

        const float32x4_t s0 = vaddq_f32(a0, b0);
        const float32x4_t s1 = vaddq_f32(a1, b1);
        const float32x4_t s2 = vaddq_f32(a2, b2);
        const float32x4_t s3 = vaddq_f32(a3, b3);
        const float32x4_t t0 = vsubq_f32(a0, b0);
        const float32x4_t t1 = vsubq_f32(a1, b1);
        const float32x4_t t2 = vsubq_f32(a2, b2);
        const float32x4_t t3 = vsubq_f32(a3, b3);

        nt_store_pair(cp + k + 0, s0, s1);
        nt_store_pair(cp + k + 8, s2, s3);
        nt_store_pair(dp + k + 0, t0, t1);
        nt_store_pair(dp + k + 8, t2, t3);
    }

    TAIL_SCALAR
    for (; k < total; ++k) {
        const real_t va = ap[k];
        const real_t vb = bp[k];
        cp[k] = va + vb;
        dp[k] = va - vb;
    }
}
