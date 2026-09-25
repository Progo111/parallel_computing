/* timer.c — реализация §6.1 ТЗ. */
#include "timer.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

double dz_wtime(void)
{
    struct timespec ts;
    /* CLOCK_MONOTONIC_RAW: не подстраивается NTP/adjtime, отсчёт от загрузки,
     * поэтому tv_sec невелик и double не теряет наносекундное разрешение. */
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &ts) != 0) {
        fprintf(stderr, "timer: clock_gettime(CLOCK_MONOTONIC_RAW) failed\n");
        exit(1);
    }
    return (double)ts.tv_sec + 1e-9 * (double)ts.tv_nsec;
}

double dz_timer_resolution(void)
{
    enum { PROBES = 1000 };
    double best = 0.0;
    double prev = dz_wtime();

    for (int i = 0; i < PROBES; ++i) {
        double cur = dz_wtime();
        double d = cur - prev;
        prev = cur;
        if (d > 0.0 && (best == 0.0 || d < best)) {
            best = d;
        }
    }
    return best;
}

static int dz_cmp_double(const void *a, const void *b)
{
    double x = *(const double *)a;
    double y = *(const double *)b;
    if (x < y) return -1;
    if (x > y) return 1;
    return 0;
}

dz_stats_t dz_stats(const double *v, size_t n)
{
    dz_stats_t s;
    memset(&s, 0, sizeof s);

    if (v == NULL || n == 0) {
        return s;
    }

    /* Копия: входной массив не портим (его ещё пишут в CSV построчно). */
    double *tmp = (double *)malloc(n * sizeof *tmp);
    if (tmp == NULL) {
        fprintf(stderr, "timer: нет памяти под %zu отсчётов статистики\n", n);
        exit(1);
    }
    memcpy(tmp, v, n * sizeof *tmp);
    qsort(tmp, n, sizeof *tmp, dz_cmp_double);

    s.min = tmp[0];
    s.max = tmp[n - 1u];
    s.median = (n % 2u == 1u) ? tmp[n / 2u]
                              : 0.5 * (tmp[n / 2u - 1u] + tmp[n / 2u]);

    double sum = 0.0;
    for (size_t i = 0; i < n; ++i) {
        sum += tmp[i];
    }
    s.mean = sum / (double)n;

    if (n > 1u) {
        double acc = 0.0;
        for (size_t i = 0; i < n; ++i) {
            double d = tmp[i] - s.mean;
            acc += d * d;
        }
        s.sd = sqrt(acc / (double)(n - 1u));
    }

    s.spread = (s.min > 0.0) ? (s.max - s.min) / s.min : 0.0;

    free(tmp);
    return s;
}
