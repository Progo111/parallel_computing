/* E0 и E1 — один и тот же код. E0 собирается с -O0 и запретом векторизации,
   E1 тем же текстом, но с -O2/-O3/-Ofast: вся разница в ключах компилятора.
   Индексация двумерная, restrict нет, выравнивание не заявлено. */
#include "common.h"

void MAT_ADD(const real_t *a, const real_t *b, real_t *c, size_t n)
{
    for (size_t i = 0; i < n; ++i)
        for (size_t j = 0; j < n; ++j)
            c[i * n + j] = a[i * n + j] + b[i * n + j];
}

void MAT_SUB(const real_t *a, const real_t *b, real_t *c, size_t n)
{
    for (size_t i = 0; i < n; ++i)
        for (size_t j = 0; j < n; ++j)
            c[i * n + j] = a[i * n + j] - b[i * n + j];
}

/* Слитый проход: A и B читаются один раз на оба результата. */
void MAT_ADDSUB(const real_t *a, const real_t *b, real_t *c, real_t *d, size_t n)
{
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            real_t x = a[i * n + j];
            real_t y = b[i * n + j];
            c[i * n + j] = x + y;
            d[i * n + j] = x - y;
        }
    }
}
