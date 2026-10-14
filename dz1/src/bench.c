/* Прогон всех вариантов ядра E0..E4 по сетке N: проверка корректности,
   замер времени, запись CSV для отчёта.

   Использование: ./build/bench [--sizes 64,128,...] [--runs 7] [--out файл] */
#define _DARWIN_C_SOURCE

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/qos.h>
#include <time.h>

#include "common.h"

typedef void (*binop_t)(const real_t *, const real_t *, real_t *, size_t);
typedef void (*fused_t)(const real_t *, const real_t *, real_t *, real_t *, size_t);

#define DECL(tag)                                                              \
    void mat_add_##tag(const real_t *, const real_t *, real_t *, size_t);      \
    void mat_sub_##tag(const real_t *, const real_t *, real_t *, size_t);      \
    void mat_addsub_##tag(const real_t *, const real_t *, real_t *, real_t *, size_t);

DECL(e0_O0)
DECL(e1_O2) DECL(e1_O3) DECL(e1_Ofast)
DECL(e2_i1) DECL(e2_i2) DECL(e2_i4) DECL(e2_i8)
DECL(e3_u1) DECL(e3_u2) DECL(e3_u4) DECL(e3_u8)
DECL(e4_p0) DECL(e4_p256) DECL(e4_p1024)

typedef struct {
    const char *stage;
    const char *tag;
    const char *flags;
    binop_t add;
    binop_t sub;
    fused_t addsub;
} variant_t;

#define VAR(stage, tag, flags) \
    { stage, #tag, flags, mat_add_##tag, mat_sub_##tag, mat_addsub_##tag }

static const variant_t variants[] = {
    VAR("E0", e0_O0,    "-O0 -fno-vectorize -fno-slp-vectorize"),
    VAR("E1", e1_O2,    "-O2"),
    VAR("E1", e1_O3,    "-O3 -funroll-loops"),
    VAR("E1", e1_Ofast, "-Ofast -funroll-loops"),
    VAR("E2", e2_i1,    "-O3 -DINTERLEAVE=1"),
    VAR("E2", e2_i2,    "-O3 -DINTERLEAVE=2"),
    VAR("E2", e2_i4,    "-O3 -DINTERLEAVE=4"),
    VAR("E2", e2_i8,    "-O3 -DINTERLEAVE=8"),
    VAR("E3", e3_u1,    "-O3 -DUNROLL=1"),
    VAR("E3", e3_u2,    "-O3 -DUNROLL=2"),
    VAR("E3", e3_u4,    "-O3 -DUNROLL=4"),
    VAR("E3", e3_u8,    "-O3 -DUNROLL=8"),
    VAR("E4", e4_p0,    "-O3 -DPF_DIST=0"),
    VAR("E4", e4_p256,  "-O3 -DPF_DIST=256"),
    VAR("E4", e4_p1024, "-O3 -DPF_DIST=1024"),
};

#define NVAR (sizeof variants / sizeof variants[0])

static const size_t default_sizes[] = { 64, 128, 256, 512, 1024, 2048, 4096 };

static volatile double sink; /* чтобы компилятор не выбросил вызовы ядер */

static double now_s(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (double)ts.tv_sec + 1e-9 * (double)ts.tv_nsec;
}

static int cmp_double(const void *p, const void *q)
{
    double x = *(const double *)p, y = *(const double *)q;
    return (x > y) - (x < y);
}

static real_t *alloc_matrix(size_t nelem)
{
    void *p = NULL;
    if (posix_memalign(&p, 16384, nelem * sizeof(real_t)) != 0 || p == NULL) {
        fprintf(stderr, "не хватило памяти под %zu элементов\n", nelem);
        exit(1);
    }
    return p;
}

/* Значения из [1, 2): нет ни нулей, ни денормалов, сложение и вычитание
   не теряют точность на ровном месте. */
static void fill(real_t *p, size_t nelem, unsigned seed)
{
    unsigned s = seed;
    for (size_t i = 0; i < nelem; ++i) {
        s = s * 1664525u + 1013904223u;
        p[i] = 1.0f + (real_t)(s >> 8) / 16777216.0f;
    }
}

/* Замеры одного варианта: возвращает минимум и медиану времени, мс. */
static void measure(const variant_t *v, int fused, const real_t *a, const real_t *b,
                    real_t *c, real_t *d, size_t n, size_t reps, int runs,
                    double *out_min, double *out_med)
{
    double *t = malloc((size_t)runs * sizeof *t);

    for (int run = -1; run < runs; ++run) { /* run = -1 — прогрев */
        double t0 = now_s();
        for (size_t r = 0; r < reps; ++r) {
            if (fused) {
                v->addsub(a, b, c, d, n);
            } else {
                v->add(a, b, c, n);
                v->sub(a, b, d, n);
            }
        }
        double dt = (now_s() - t0) / (double)reps;
        if (run >= 0)
            t[run] = dt * 1e3;
        sink += (double)c[0] + (double)d[n * n - 1];
    }

    qsort(t, (size_t)runs, sizeof *t, cmp_double);
    *out_min = t[0];
    *out_med = t[runs / 2];
    free(t);
}

int main(int argc, char **argv)
{
    const size_t *sizes = default_sizes;
    size_t nsizes = sizeof default_sizes / sizeof default_sizes[0];
    size_t own_sizes[32];
    int runs = 7;
    const char *out_path = "results/raw.csv";

    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--runs") && i + 1 < argc) {
            runs = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--out") && i + 1 < argc) {
            out_path = argv[++i];
        } else if (!strcmp(argv[i], "--sizes") && i + 1 < argc) {
            nsizes = 0;
            for (char *tok = strtok(argv[++i], ","); tok; tok = strtok(NULL, ",")) {
                if (nsizes == sizeof own_sizes / sizeof own_sizes[0]) break;
                own_sizes[nsizes++] = (size_t)strtoull(tok, NULL, 10);
            }
            sizes = own_sizes;
        } else {
            fprintf(stderr, "использование: %s [--sizes 64,128] [--runs 7] [--out файл]\n",
                    argv[0]);
            return 1;
        }
    }
    if (runs < 1 || nsizes == 0) {
        fprintf(stderr, "некорректные параметры прогона\n");
        return 1;
    }

    /* На M4 поток может уехать на энергоэффективное ядро — тогда времена
       разъезжаются в разы. Просим планировщик держать нас на P-ядре. */
    if (pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0) != 0)
        fprintf(stderr, "предупреждение: не удалось задать QoS-класс\n");

    FILE *out = fopen(out_path, "w");
    if (!out) {
        perror(out_path);
        return 1;
    }
    fprintf(out, "N,stage,tag,flags,mode,reps,min_ms,med_ms,gbps\n");

    int bad = 0;

    for (size_t si = 0; si < nsizes; ++si) {
        size_t n = sizes[si];
        size_t nelem = n * n;

        real_t *a = alloc_matrix(nelem), *b = alloc_matrix(nelem);
        real_t *c = alloc_matrix(nelem), *d = alloc_matrix(nelem);
        real_t *cref = alloc_matrix(nelem), *dref = alloc_matrix(nelem);

        fill(a, nelem, 12345u);
        fill(b, nelem, 67890u);
        for (size_t i = 0; i < nelem; ++i) {
            cref[i] = a[i] + b[i];
            dref[i] = a[i] - b[i];
        }

        /* Число повторов на один замер: чем меньше матрица, тем больше
           повторов, чтобы замер был длиннее разрешения таймера. */
        size_t reps = 40000000u / nelem;
        if (reps == 0) reps = 1;

        printf("N = %zu (%.1f МиБ на матрицу), повторов на замер: %zu\n",
               n, (double)(nelem * sizeof(real_t)) / 1048576.0, reps);

        for (size_t vi = 0; vi < NVAR; ++vi) {
            const variant_t *v = &variants[vi];

            for (int fused = 0; fused < 2; ++fused) {
                memset(c, 0, nelem * sizeof *c);
                memset(d, 0, nelem * sizeof *d);

                if (fused)
                    v->addsub(a, b, c, d, n);
                else {
                    v->add(a, b, c, n);
                    v->sub(a, b, d, n);
                }
                int ok = memcmp(c, cref, nelem * sizeof *c) == 0 &&
                         memcmp(d, dref, nelem * sizeof *d) == 0;
                if (!ok) {
                    fprintf(stderr, "РАСХОЖДЕНИЕ: %s, режим %s, N = %zu\n",
                            v->tag, fused ? "fused" : "sep", n);
                    bad = 1;
                }

                double min_ms, med_ms;
                measure(v, fused, a, b, c, d, n, reps, runs, &min_ms, &med_ms);

                /* Полезный трафик за один проход: sep читает A и B дважды. */
                double bytes = (fused ? 16.0 : 24.0) * (double)nelem;
                double gbps = bytes / (min_ms * 1e-3) / 1e9;

                printf("  %-9s %-5s %9.5f мс  %6.1f ГБ/с%s\n",
                       v->tag, fused ? "fused" : "sep", min_ms, gbps,
                       ok ? "" : "  <-- ошибка");

                fprintf(out, "%zu,%s,%s,\"%s\",%s,%zu,%.8f,%.8f,%.2f\n",
                        n, v->stage, v->tag, v->flags, fused ? "fused" : "sep",
                        reps, min_ms, med_ms, gbps);
            }
        }
        fflush(out);

        free(a); free(b); free(c); free(d); free(cref); free(dref);
    }

    fclose(out);
    printf("\nрезультаты: %s\n", out_path);
    printf(bad ? "есть расхождения с эталоном\n"
               : "все варианты совпали с эталоном побитово\n");
    return bad;
}
