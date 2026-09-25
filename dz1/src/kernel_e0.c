/* kernel_e0.c — ЭТАПЫ E0 и E1 (ТЗ §4, E0.1–E0.3 и E1.1–E1.4).
 *
 * ОДИН исходник на оба этапа — в этом весь смысл E1: код не меняется,
 * меняются только ключи компилятора. Символы разводятся через -DKERNEL_TAG.
 *
 * Собирается 6 раз (см. Makefile / VARIANT_LIST в common.h):
 *   e0_O0     -O0 -fno-vectorize -fno-slp-vectorize -fno-unroll-loops
 *   e0_O1     -O1 -fno-vectorize -fno-slp-vectorize -fno-unroll-loops
 *   e1_O2     -O2    -mcpu=apple-m4
 *   e1_O3     -O3    -mcpu=apple-m4 -funroll-loops
 *   e1_Ofast  -Ofast -mcpu=apple-m4 -funroll-loops
 *   e1_ffm    -O3 -ffast-math -mcpu=apple-m4 -funroll-loops
 *
 * Что проверяется этим файлом:
 *   E0  — «честный ноль»: наивный двойной цикл с двумерной индексацией
 *         a[i*n + j], БЕЗ restrict, БЕЗ выравнивания, без разворота,
 *         без интринсиков и без единой #pragma. Ожидаемый asm — скалярные
 *         ldr s / fadd s / str s, 1 элемент за итерацию.
 *   E1  — тот же текст, поданный автовекторизатору clang. Здесь измеряется,
 *         сколько задачи компилятор решает сам: он обязан векторизовать
 *         внутренний цикл, но вынужден вставить runtime-проверку перекрытия
 *         указателей (memcheck), потому что restrict тут намеренно нет.
 *         Снятие этого memcheck — предмет этапа E2 (kernel_e2.c).
 *
 * ВАЖНО: файл не должен содержать ничего, зависящего от уровня оптимизации,
 * иначе сравнение E0 vs E1 перестаёт быть сравнением ключей.
 *
 * Параметр n — размерность матрицы N; обрабатывается n*n элементов, row-major.
 * Одна операция на элемент, переассоциации нет => результат всех вариантов
 * (включая -Ofast / -ffast-math) обязан быть побитово одинаковым.
 */

#include "common.h"

/* C = A + B */
void MAT_ADD_FN(KERNEL_TAG)(const real_t *a, const real_t *b,
                            real_t *c, size_t n)
{
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            c[i * n + j] = a[i * n + j] + b[i * n + j];
        }
    }
}

/* D = A - B */
void MAT_SUB_FN(KERNEL_TAG)(const real_t *a, const real_t *b,
                            real_t *c, size_t n)
{
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            c[i * n + j] = a[i * n + j] - b[i * n + j];
        }
    }
}

/* Слитый проход: C = A + B и D = A - B за один обход A и B.
 * Наивная форма (как и всё на E0): двойной цикл, каждый элемент a[i*n+j] и
 * b[i*n+j] читается один раз за итерацию. Без restrict компилятор не знает,
 * что c и d не перекрываются с a и b, — это и есть измеряемое ограничение. */
void MAT_ADDSUB_FN(KERNEL_TAG)(const real_t *a, const real_t *b,
                               real_t *c, real_t *d, size_t n)
{
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            const real_t va = a[i * n + j];
            const real_t vb = b[i * n + j];
            c[i * n + j] = va + vb;
            d[i * n + j] = va - vb;
        }
    }
}
