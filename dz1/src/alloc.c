/* alloc.c — реализация §3.1.3 ТЗ. */
#include "alloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Максимум одновременно живых буферов (bench держит 6 штук на каждое N). */
#define DZ_ALLOC_SLOTS 64

/* Реестр: возвращённый (сдвинутый) указатель -> базовый указатель malloc'а.
 * Без него сдвиг shift_bytes невозможно было бы освободить. */
typedef struct {
    void *user;  /* то, что отдали наружу */
    void *base;  /* то, что вернул posix_memalign */
    size_t bytes;
} dz_alloc_slot_t;

static dz_alloc_slot_t g_slots[DZ_ALLOC_SLOTS];
static size_t g_slots_used;

static void dz_die(const char *what, size_t bytes)
{
    fprintf(stderr,
            "alloc: %s (запрошено %zu байт, %.2f МиБ). "
            "Уменьшите максимальный N (--sizes) и повторите.\n",
            what, bytes, (double)bytes / (1024.0 * 1024.0));
    exit(1);
}

real_t *dz_alloc_matrix(size_t nelem, size_t shift_bytes)
{
    if (nelem == 0) {
        fprintf(stderr, "alloc: dz_alloc_matrix(nelem=0) — некорректный аргумент\n");
        exit(1);
    }
    if (shift_bytes % CACHELINE != 0u) {
        fprintf(stderr,
                "alloc: shift_bytes=%zu не кратен CACHELINE=%u\n",
                shift_bytes, (unsigned)CACHELINE);
        exit(1);
    }
    if (nelem > (SIZE_MAX - shift_bytes - DZ_PAGESIZE) / sizeof(real_t)) {
        fprintf(stderr, "alloc: переполнение размера при nelem=%zu\n", nelem);
        exit(1);
    }

    size_t payload = nelem * sizeof(real_t);
    size_t total = payload + shift_bytes;

    /* Округляем вверх до целого числа страниц: и prefault проще, и аллокатор
     * отдаёт ровно столько страниц, сколько мы собираемся трогать. */
    total = (total + (DZ_PAGESIZE - 1u)) & ~((size_t)DZ_PAGESIZE - 1u);

    if (g_slots_used >= DZ_ALLOC_SLOTS) {
        fprintf(stderr, "alloc: исчерпан реестр буферов (%d слотов)\n", DZ_ALLOC_SLOTS);
        exit(1);
    }

    void *base = NULL;
    int rc = posix_memalign(&base, (size_t)DZ_PAGESIZE, total);
    if (rc != 0 || base == NULL) {
        dz_die("не удалось выделить выровненный буфер", total);
    }

    void *user = (void *)((unsigned char *)base + shift_bytes);

    g_slots[g_slots_used].user = user;
    g_slots[g_slots_used].base = base;
    g_slots[g_slots_used].bytes = payload;
    g_slots_used++;

    return (real_t *)user;
}

void dz_free_matrix(real_t *p)
{
    if (p == NULL) {
        return;
    }
    for (size_t i = 0; i < g_slots_used; ++i) {
        if (g_slots[i].user == (void *)p) {
            free(g_slots[i].base);
            g_slots[i] = g_slots[g_slots_used - 1u];
            g_slots_used--;
            return;
        }
    }
    fprintf(stderr, "alloc: dz_free_matrix(%p): указатель не найден в реестре\n", (void *)p);
    exit(1);
}

void dz_prefault(void *p, size_t bytes)
{
    if (p == NULL || bytes == 0) {
        return;
    }
    volatile unsigned char *q = (volatile unsigned char *)p;
    for (size_t off = 0; off < bytes; off += (size_t)DZ_PAGESIZE) {
        q[off] = 0u; /* именно ЗАПИСЬ: чтение не снимет copy-on-write нулевой страницы */
    }
    q[bytes - 1u] = 0u;
}

/* xorshift64* — детерминированный, быстрый, достаточного качества для данных. */
static inline uint64_t dz_xorshift64star(uint64_t *state)
{
    uint64_t x = *state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    return x * 0x2545F4914F6CDD1DULL;
}

void dz_fill_random(real_t *p, size_t nelem, uint64_t seed)
{
    /* Нулевое состояние — вырожденная точка xorshift, подменяем константой. */
    uint64_t state = (seed != 0u) ? seed : 0x9E3779B97F4A7C15ULL;

    for (size_t i = 0; i < nelem; ++i) {
        uint64_t r = dz_xorshift64star(&state);
        /* Экспонента 127 + случайная мантисса => ровно [1.0f, 2.0f):
         * ни денормалов, ни нулей, ни inf/NaN (§5.3.2). */
        uint32_t bits = 0x3F800000u | (uint32_t)((r >> 41) & 0x007FFFFFu);
        float f;
        memcpy(&f, &bits, sizeof f);
        p[i] = (real_t)f;
    }
}
