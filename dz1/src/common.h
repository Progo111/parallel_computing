/* Общие определения для всех вариантов ядра.
   Вариант 3: C = A + B, D = A - B, матрицы float, хранение построчно. */
#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>

/* Apple M4, P-ядро (sysctl hw.perflevel0.*): L1d 128 КиБ, L2 16 МиБ. */
#define CACHELINE 128u
#define L1D_SIZE  (128u * 1024u)
#define L2_SIZE   (16u * 1024u * 1024u)

typedef float real_t;

#define ALIGNED(p) __builtin_assume_aligned((p), CACHELINE)

/* Один и тот же исходник собирается несколько раз с разными ключами,
   имена функций разводятся флагом -DKERNEL_TAG=<тег>. */
#define CAT_(a, b) a##b
#define CAT(a, b)  CAT_(a, b)
#define MAT_ADD    CAT(mat_add_, KERNEL_TAG)
#define MAT_SUB    CAT(mat_sub_, KERNEL_TAG)
#define MAT_ADDSUB CAT(mat_addsub_, KERNEL_TAG)

#endif
