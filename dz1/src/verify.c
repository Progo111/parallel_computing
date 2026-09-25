/* verify.c — реализация §7 ТЗ. */
#include "verify.h"

#include <string.h>

#define DZ_FNV1A_BASIS 14695981039346656037ULL
#define DZ_FNV1A_PRIME 1099511628211ULL

uint64_t dz_fnv1a_cont(uint64_t h, const void *p, size_t bytes)
{
    const unsigned char *q = (const unsigned char *)p;
    for (size_t i = 0; i < bytes; ++i) {
        h ^= (uint64_t)q[i];
        h *= DZ_FNV1A_PRIME;
    }
    return h;
}

uint64_t dz_fnv1a(const void *p, size_t bytes)
{
    return dz_fnv1a_cont(DZ_FNV1A_BASIS, p, bytes);
}

int dz_bitcmp(const void *a, const void *b, size_t bytes, size_t *first_diff)
{
    if (bytes == 0) {
        return 0;
    }
    if (memcmp(a, b, bytes) == 0) {
        return 0;
    }

    /* Расхождение есть — локализуем его: сначала грубо по блокам (быстрый
     * memcmp), затем побайтно внутри найденного блока. */
    const unsigned char *pa = (const unsigned char *)a;
    const unsigned char *pb = (const unsigned char *)b;
    const size_t block = 4096u;

    size_t off = 0;
    while (off < bytes) {
        size_t chunk = (bytes - off < block) ? (bytes - off) : block;
        if (memcmp(pa + off, pb + off, chunk) != 0) {
            for (size_t i = 0; i < chunk; ++i) {
                if (pa[off + i] != pb[off + i]) {
                    if (first_diff != NULL) {
                        *first_diff = off + i;
                    }
                    return 1;
                }
            }
        }
        off += chunk;
    }

    /* Сюда не попадаем (memcmp уже сказал, что различие есть), но на всякий. */
    if (first_diff != NULL) {
        *first_diff = bytes;
    }
    return 1;
}
