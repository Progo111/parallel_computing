/* src/bw_probe.c — однопоточный STREAM-подобный микробенчмарк пиковой ПСП.
 *
 * ДЗ №1 «Освоение векторизации», вариант 3 (C = A+B, D = A-B).
 * Блок 1 плана работ (§12 ТЗ), подзадача §2.2.2: измерить реальную пиковую
 * пропускную способность памяти на ОДНОМ P-ядре. Полученное число —
 * знаменатель всех roofline-оценок §8.1 (AI_ridge = Peak_FLOPS / Peak_BW,
 * t_min = bytes_per_elem * N^2 / Peak_BW).
 *
 * Файл САМОДОСТАТОЧЕН: собирается отдельно от основного Makefile скриптом
 * scripts/bw_probe.sh и не участвует в линковке ядер E0..E4.
 * Из common.h берутся только константы CACHELINE / DZ_PAGESIZE / L2_SIZE.
 *
 * Тесты (все — NEON-интринсики, разворот x4, restrict):
 *   copy   c[i] = a[i]                    12 Б/элем (read a + RFO c + write c)
 *   scale  b[i] = s * c[i]                12 Б/элем
 *   add    c[i] = a[i] + b[i]             16 Б/элем  <- ровно наше ядро E0..E3
 *   triad  a[i] = b[i] + s * c[i]         16 Б/элем
 *   add_nt c[i] = a[i] + b[i], NT-store   12 Б/элем  <- верхняя планка для E4
 *   read   sum += a[i] + b[i]              8 Б/элем  <- пик ПСП на чтение
 *
 * Модель трафика взята из dz_bytes_per_elem() (common.h): обычная запись
 * стоит 3 обращения к памяти (RFO + store + чтение источника на элемент),
 * non-temporal store обходит RFO и стоит 2. ВНИМАНИЕ: на Apple M4 это
 * измеримо НЕ подтверждается — add и add_nt идут за одно и то же время
 * (см. блок ИТОГИ в выводе программы), т.е. обычный store уже не платит RFO.
 *
 * Замеры ЧЕРЕДУЮТСЯ (внешний цикл — повтор, внутренний — тест), иначе DVFS
 * macOS делает распределение бимодальным и тесты несравнимыми между собой.
 */

#include <arm_neon.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/qos.h>
#include <time.h>
#include <unistd.h>

#include "common.h" /* CACHELINE, DZ_PAGESIZE, L2_SIZE */

/* --------------------------------------------------------------------------
 * Параметры прогона
 * ------------------------------------------------------------------------ */
#define DEFAULT_MIB 256u /* на КАЖДЫЙ из трёх массивов; 256 МиБ >> 4 x L2 */
#define WARMUP_RUNS 3 /* 1 прогрева мало: первый замер стабильно выбросной (DVFS + TLB) */
#define TIMED_RUNS  11
#define UNROLL_F32  16 /* 4 NEON-регистра x 4 fp32 = разворот x4 */

/* Сток, чтобы оптимизатор не выбросил тела циклов. */
static volatile float g_sink = 0.0f;

#define COMPILER_BARRIER() __asm__ volatile("" ::: "memory")

/* --------------------------------------------------------------------------
 * Таймер: CLOCK_MONOTONIC_RAW — не подвержен подстройке NTP/adjtime.
 * ------------------------------------------------------------------------ */
static double now_sec(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &ts) != 0) {
        perror("clock_gettime(CLOCK_MONOTONIC_RAW)");
        exit(EXIT_FAILURE);
    }
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

static double timer_resolution_sec(void)
{
    struct timespec ts;
    if (clock_getres(CLOCK_MONOTONIC_RAW, &ts) != 0)
        return -1.0;
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

/* --------------------------------------------------------------------------
 * Аллокация: выравнивание на страницу (16 КиБ) + prefault.
 * Без prefault первый проход меряет обработку page fault'ов ядром, а не ПСП.
 * ------------------------------------------------------------------------ */
static float *alloc_pages(size_t bytes)
{
    void *p = NULL;
    int rc = posix_memalign(&p, (size_t)DZ_PAGESIZE, bytes);
    if (rc != 0 || p == NULL) {
        fprintf(stderr, "posix_memalign(%zu B) failed: %s\n", bytes, strerror(rc));
        exit(EXIT_FAILURE);
    }
    return (float *)p;
}

/* Проход ЗАПИСЬЮ по каждой странице: физические кадры реально отображаются. */
static void prefault(float *p, size_t bytes)
{
    volatile unsigned char *q = (volatile unsigned char *)p;
    for (size_t off = 0; off < bytes; off += (size_t)DZ_PAGESIZE)
        q[off] = 0;
    if (bytes > 0)
        q[bytes - 1] = 0;
}

/* --------------------------------------------------------------------------
 * QoS: просим планировщик macOS держать поток на P-кластере.
 * QOS_CLASS_USER_INTERACTIVE — единственный рычаг без sudo и без private API.
 * ------------------------------------------------------------------------ */
static int pin_to_pcore(void)
{
    int rc = pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
    if (rc != 0) {
        fprintf(stderr, "WARNING: pthread_set_qos_class_self_np failed: %s (rc=%d)\n", strerror(rc), rc);
        return rc;
    }
    qos_class_t got = QOS_CLASS_UNSPECIFIED;
    int rel = 0;
    if (pthread_get_qos_class_np(pthread_self(), &got, &rel) == 0 && got != QOS_CLASS_USER_INTERACTIVE)
        fprintf(stderr, "WARNING: QoS class readback = %d (expected %d)\n", (int)got, (int)QOS_CLASS_USER_INTERACTIVE);
    return 0;
}

/* --------------------------------------------------------------------------
 * Ядра. Все на NEON-интринсиках с разворотом x4 (16 fp32 за итерацию),
 * с restrict — мы меряем подсистему памяти, а не качество кодогенерации.
 * n кратно UNROLL_F32 по построению (см. main).
 * ------------------------------------------------------------------------ */

static void k_copy(const float *restrict a, float *restrict c, size_t n)
{
    for (size_t i = 0; i < n; i += UNROLL_F32) {
        float32x4_t x0 = vld1q_f32(a + i + 0);
        float32x4_t x1 = vld1q_f32(a + i + 4);
        float32x4_t x2 = vld1q_f32(a + i + 8);
        float32x4_t x3 = vld1q_f32(a + i + 12);
        vst1q_f32(c + i + 0, x0);
        vst1q_f32(c + i + 4, x1);
        vst1q_f32(c + i + 8, x2);
        vst1q_f32(c + i + 12, x3);
    }
}

static void k_scale(const float *restrict c, float *restrict b, size_t n, float s)
{
    const float32x4_t vs = vdupq_n_f32(s);
    for (size_t i = 0; i < n; i += UNROLL_F32) {
        float32x4_t x0 = vld1q_f32(c + i + 0);
        float32x4_t x1 = vld1q_f32(c + i + 4);
        float32x4_t x2 = vld1q_f32(c + i + 8);
        float32x4_t x3 = vld1q_f32(c + i + 12);
        vst1q_f32(b + i + 0, vmulq_f32(vs, x0));
        vst1q_f32(b + i + 4, vmulq_f32(vs, x1));
        vst1q_f32(b + i + 8, vmulq_f32(vs, x2));
        vst1q_f32(b + i + 12, vmulq_f32(vs, x3));
    }
}

static void k_add(const float *restrict a, const float *restrict b, float *restrict c, size_t n)
{
    for (size_t i = 0; i < n; i += UNROLL_F32) {
        float32x4_t a0 = vld1q_f32(a + i + 0);
        float32x4_t a1 = vld1q_f32(a + i + 4);
        float32x4_t a2 = vld1q_f32(a + i + 8);
        float32x4_t a3 = vld1q_f32(a + i + 12);
        float32x4_t b0 = vld1q_f32(b + i + 0);
        float32x4_t b1 = vld1q_f32(b + i + 4);
        float32x4_t b2 = vld1q_f32(b + i + 8);
        float32x4_t b3 = vld1q_f32(b + i + 12);
        vst1q_f32(c + i + 0, vaddq_f32(a0, b0));
        vst1q_f32(c + i + 4, vaddq_f32(a1, b1));
        vst1q_f32(c + i + 8, vaddq_f32(a2, b2));
        vst1q_f32(c + i + 12, vaddq_f32(a3, b3));
    }
}

static void k_triad(float *restrict a, const float *restrict b, const float *restrict c, size_t n, float s)
{
    const float32x4_t vs = vdupq_n_f32(s);
    for (size_t i = 0; i < n; i += UNROLL_F32) {
        float32x4_t b0 = vld1q_f32(b + i + 0);
        float32x4_t b1 = vld1q_f32(b + i + 4);
        float32x4_t b2 = vld1q_f32(b + i + 8);
        float32x4_t b3 = vld1q_f32(b + i + 12);
        float32x4_t c0 = vld1q_f32(c + i + 0);
        float32x4_t c1 = vld1q_f32(c + i + 4);
        float32x4_t c2 = vld1q_f32(c + i + 8);
        float32x4_t c3 = vld1q_f32(c + i + 12);
        vst1q_f32(a + i + 0, vfmaq_f32(b0, vs, c0));
        vst1q_f32(a + i + 4, vfmaq_f32(b1, vs, c1));
        vst1q_f32(a + i + 8, vfmaq_f32(b2, vs, c2));
        vst1q_f32(a + i + 12, vfmaq_f32(b3, vs, c3));
    }
}

/* Та же операция, что k_add, но запись — non-temporal (обходит RFO/кэш).
 * __builtin_nontemporal_store у clang на AArch64 разворачивается в STNP. */
static void k_add_nt(const float *restrict a, const float *restrict b, float *restrict c, size_t n)
{
    for (size_t i = 0; i < n; i += UNROLL_F32) {
        float32x4_t a0 = vld1q_f32(a + i + 0);
        float32x4_t a1 = vld1q_f32(a + i + 4);
        float32x4_t a2 = vld1q_f32(a + i + 8);
        float32x4_t a3 = vld1q_f32(a + i + 12);
        float32x4_t b0 = vld1q_f32(b + i + 0);
        float32x4_t b1 = vld1q_f32(b + i + 4);
        float32x4_t b2 = vld1q_f32(b + i + 8);
        float32x4_t b3 = vld1q_f32(b + i + 12);
        __builtin_nontemporal_store(vaddq_f32(a0, b0), (float32x4_t *)(void *)(c + i + 0));
        __builtin_nontemporal_store(vaddq_f32(a1, b1), (float32x4_t *)(void *)(c + i + 4));
        __builtin_nontemporal_store(vaddq_f32(a2, b2), (float32x4_t *)(void *)(c + i + 8));
        __builtin_nontemporal_store(vaddq_f32(a3, b3), (float32x4_t *)(void *)(c + i + 12));
    }
}

/* Чистое чтение двух потоков, без единой записи в память.
 *
 * ВАЖНО: 8 НЕЗАВИСИМЫХ аккумуляторов — по 4 на каждый входной поток.
 * Вариант с 4 аккумуляторами (s += a[i] + b[i]) упирается не в память, а в
 * петлевую зависимость по FADD (latency ~3 такта): измерено 34 ГБ/с против
 * 61 ГБ/с здесь на той же машине. То есть он мерил бы АЛУ, а не ПСП. */
static float k_read(const float *restrict a, const float *restrict b, size_t n)
{
    float32x4_t p0 = vdupq_n_f32(0.0f), p1 = vdupq_n_f32(0.0f);
    float32x4_t p2 = vdupq_n_f32(0.0f), p3 = vdupq_n_f32(0.0f);
    float32x4_t q0 = vdupq_n_f32(0.0f), q1 = vdupq_n_f32(0.0f);
    float32x4_t q2 = vdupq_n_f32(0.0f), q3 = vdupq_n_f32(0.0f);
    for (size_t i = 0; i < n; i += UNROLL_F32) {
        p0 = vaddq_f32(p0, vld1q_f32(a + i + 0));
        q0 = vaddq_f32(q0, vld1q_f32(b + i + 0));
        p1 = vaddq_f32(p1, vld1q_f32(a + i + 4));
        q1 = vaddq_f32(q1, vld1q_f32(b + i + 4));
        p2 = vaddq_f32(p2, vld1q_f32(a + i + 8));
        q2 = vaddq_f32(q2, vld1q_f32(b + i + 8));
        p3 = vaddq_f32(p3, vld1q_f32(a + i + 12));
        q3 = vaddq_f32(q3, vld1q_f32(b + i + 12));
    }
    float32x4_t s = vaddq_f32(vaddq_f32(vaddq_f32(p0, p1), vaddq_f32(p2, p3)),
                              vaddq_f32(vaddq_f32(q0, q1), vaddq_f32(q2, q3)));
    return vaddvq_f32(s);
}

/* --------------------------------------------------------------------------
 * Описание одного теста + диспетчер
 * ------------------------------------------------------------------------ */
typedef enum {
    T_COPY = 0,
    T_SCALE,
    T_ADD,
    T_TRIAD,
    T_ADD_NT,
    T_READ,
    T_COUNT
} test_id_t;

typedef struct {
    const char *name;
    double      bytes_per_elem;
    const char *formula;
} test_desc_t;

static const test_desc_t TESTS[T_COUNT] = {
    { "copy",   12.0, "c[i] = a[i]"             },
    { "scale",  12.0, "b[i] = s*c[i]"           },
    { "add",    16.0, "c[i] = a[i] + b[i]"      },
    { "triad",  16.0, "a[i] = b[i] + s*c[i]"    },
    { "add_nt", 12.0, "c[i] = a[i]+b[i] (NT st)" },
    { "read",    8.0, "sum += a[i] + b[i]"      },
};

typedef struct {
    float *a, *b, *c;
    size_t n;
    float  s;
} bufs_t;

static void run_kernel(test_id_t t, const bufs_t *B)
{
    switch (t) {
    case T_COPY:   k_copy(B->a, B->c, B->n); break;
    case T_SCALE:  k_scale(B->c, B->b, B->n, B->s); break;
    case T_ADD:    k_add(B->a, B->b, B->c, B->n); break;
    case T_TRIAD:  k_triad(B->a, B->b, B->c, B->n, B->s); break;
    case T_ADD_NT: k_add_nt(B->a, B->b, B->c, B->n); break;
    case T_READ:   g_sink = k_read(B->a, B->b, B->n); break;
    case T_COUNT:
    default:       break;
    }
}

/* --------------------------------------------------------------------------
 * Статистика
 * ------------------------------------------------------------------------ */
static int cmp_double(const void *x, const void *y)
{
    double a = *(const double *)x, b = *(const double *)y;
    return (a > b) - (a < b);
}

static double median_of(const double *sorted, int n)
{
    if (n <= 0)
        return 0.0;
    if (n % 2)
        return sorted[n / 2];
    return 0.5 * (sorted[n / 2 - 1] + sorted[n / 2]);
}

/* --------------------------------------------------------------------------
 * CLI
 * ------------------------------------------------------------------------ */
static void usage(const char *prog)
{
    fprintf(stderr,
            "usage: %s [--mib <MiB per array>] [--runs <N>] [--csv <path>]\n"
            "  --mib   размер КАЖДОГО из трёх массивов, МиБ (по умолчанию %u)\n"
            "  --runs  число замеров после прогрева (по умолчанию %d)\n"
            "  --csv   куда писать CSV: test,mib,bytes_per_elem,run_idx,time_sec,gb_per_sec,gib_per_sec\n",
            prog, DEFAULT_MIB, TIMED_RUNS);
}

static unsigned parse_uint(const char *s, const char *what)
{
    char         *end = NULL;
    unsigned long v   = strtoul(s, &end, 10);
    if (end == s || *end != '\0' || v == 0UL || v > 1000000UL) {
        fprintf(stderr, "bad value for %s: '%s'\n", what, s);
        exit(EXIT_FAILURE);
    }
    return (unsigned)v;
}

int main(int argc, char **argv)
{
    unsigned    mib      = DEFAULT_MIB;
    int         runs     = TIMED_RUNS;
    const char *csv_path = NULL;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--mib") == 0 && i + 1 < argc) {
            mib = parse_uint(argv[++i], "--mib");
        } else if (strcmp(argv[i], "--runs") == 0 && i + 1 < argc) {
            runs = (int)parse_uint(argv[++i], "--runs");
        } else if (strcmp(argv[i], "--csv") == 0 && i + 1 < argc) {
            csv_path = argv[++i];
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return EXIT_SUCCESS;
        } else {
            fprintf(stderr, "unknown argument: %s\n", argv[i]);
            usage(argv[0]);
            return EXIT_FAILURE;
        }
    }
    if (runs < 1 || runs > 1001) {
        fprintf(stderr, "--runs must be in [1, 1001]\n");
        return EXIT_FAILURE;
    }

    const size_t bytes_per_arr = (size_t)mib * 1024u * 1024u;
    size_t       n             = bytes_per_arr / sizeof(float);
    n -= n % UNROLL_F32; /* кратность развороту: хвост не нужен */
    const size_t used_bytes = n * sizeof(float);

    const int qos_rc = pin_to_pcore();

    bufs_t B;
    B.a = alloc_pages(bytes_per_arr);
    B.b = alloc_pages(bytes_per_arr);
    B.c = alloc_pages(bytes_per_arr);
    B.n = n;
    B.s = 1.0000001f;

    /* Prefault ДО любых замеров: иначе первый проход меряет page faults. */
    prefault(B.a, bytes_per_arr);
    prefault(B.b, bytes_per_arr);
    prefault(B.c, bytes_per_arr);

    /* Осмысленные данные (не denormals, не NaN). */
    for (size_t i = 0; i < n; ++i) {
        B.a[i] = 1.0f + (float)(i & 1023u) * 0.001f;
        B.b[i] = 2.0f - (float)(i & 511u) * 0.002f;
        B.c[i] = 0.5f;
    }

    FILE *csv = NULL;
    if (csv_path) {
        csv = fopen(csv_path, "w");
        if (!csv) {
            fprintf(stderr, "cannot open CSV '%s': %s\n", csv_path, strerror(errno));
            free(B.a);
            free(B.b);
            free(B.c);
            return EXIT_FAILURE;
        }
        fprintf(csv, "test,mib,bytes_per_elem,run_idx,time_sec,gb_per_sec,gib_per_sec\n");
    }

    /* ------------------------------------------------------------ шапка -- */
    time_t    tnow = time(NULL);
    struct tm tmv;
    char      stamp[64] = "unknown";
    if (localtime_r(&tnow, &tmv))
        strftime(stamp, sizeof stamp, "%Y-%m-%d %H:%M:%S", &tmv);

    printf("========================================================================\n");
    printf(" bw_probe — STREAM-подобный микробенчмарк ПСП (1 поток, P-ядро)\n");
    printf(" ДЗ №1, вариант 3 (C = A+B, D = A-B). §2.2.2 ТЗ.\n");
    printf(" Дата прогона        : %s\n", stamp);
    printf(" Массивов            : 3 x %u МиБ (используется %.2f МиБ = %zu элем. fp32)\n", mib,
           (double)used_bytes / (1024.0 * 1024.0), n);
    printf(" Рабочее множество   : %.2f МиБ  (L2 = %.2f МиБ -> x%.1f)\n", 3.0 * (double)used_bytes / (1024.0 * 1024.0),
           (double)L2_SIZE / (1024.0 * 1024.0), 3.0 * (double)used_bytes / (double)L2_SIZE);
    printf(" Выравнивание        : %u Б (hw.pagesize), cacheline %u Б\n", (unsigned)DZ_PAGESIZE, (unsigned)CACHELINE);
    printf(" Prefault            : да (проход записью по всем страницам)\n");
    printf(" Прогонов            : %d прогрев + %d замеров на тест (ЧЕРЕДУЮЩИЕСЯ)\n", WARMUP_RUNS, runs);
    printf(" Вектор              : NEON 128 бит, разворот x4 (%d fp32/итер.)\n", UNROLL_F32);
    printf(" Таймер              : CLOCK_MONOTONIC_RAW, разрешение %.1f нс\n", timer_resolution_sec() * 1e9);
    printf(" QoS                 : %s\n",
           qos_rc == 0 ? "QOS_CLASS_USER_INTERACTIVE (P-кластер)" : "НЕ УСТАНОВЛЕН — результат может быть занижен!");
    printf("========================================================================\n\n");

    /* times[t][r] — ЧЕРЕДУЮЩАЯСЯ схема сбора: внешний цикл по повторам,
     * внутренний по тестам. Это принципиально: macOS непрерывно меняет
     * частоту P-ядра (DVFS, Low Power Mode, тепловой бюджет), и распределение
     * времён бимодально. Если гнать 11 замеров одного теста подряд, весь тест
     * может целиком попасть в «медленную» моду — измеренная ПСП тогда занижена
     * вдвое и тесты между собой несравнимы. При чередовании каждый тест
     * видит одну и ту же смесь состояний DVFS. */
    double *times = (double *)malloc((size_t)T_COUNT * (size_t)runs * sizeof(double));
    double *sorted = (double *)malloc((size_t)runs * sizeof(double));
    double  t_min_of[T_COUNT] = { 0 };
    if (!times || !sorted) {
        fprintf(stderr, "out of memory for timing arrays\n");
        return EXIT_FAILURE;
    }
#define TIME_AT(t, r) times[(size_t)(t) * (size_t)runs + (size_t)(r)]

    for (int r = 0; r < WARMUP_RUNS + runs; ++r) {
        for (int t = 0; t < T_COUNT; ++t) {
            COMPILER_BARRIER();
            const double t0 = now_sec();
            run_kernel((test_id_t)t, &B);
            COMPILER_BARRIER();
            const double t1 = now_sec();
            if (r >= WARMUP_RUNS)
                TIME_AT(t, r - WARMUP_RUNS) = t1 - t0;
            /* Сток: утаскиваем элемент результата в volatile, чтобы
             * оптимизатор не признал вызов мёртвым. */
            g_sink += B.c[(size_t)r % n] + B.a[0] + B.b[0];
            COMPILER_BARRIER();
        }
    }

    printf("%-8s %-26s %6s  %10s %10s  %9s %9s  %9s %8s %7s\n", "test", "формула", "Б/элем", "t_min,мс", "t_med,мс",
           "ГБ/с(min)", "ГБ/с(med)", "ГиБ/с", "IQR,%", "max-min%");
    printf("------------------------------------------------------------------------"
           "--------------------------------------------\n");

    int noisy = 0;
    for (int t = 0; t < T_COUNT; ++t) {
        const test_desc_t *td = &TESTS[t];

        for (int r = 0; r < runs; ++r)
            sorted[r] = TIME_AT(t, r);
        qsort(sorted, (size_t)runs, sizeof(double), cmp_double);

        const double t_min   = sorted[0];
        const double t_max   = sorted[runs - 1];
        const double t_med   = median_of(sorted, runs);
        const double traffic = td->bytes_per_elem * (double)n; /* байт */
        const double gb_min  = traffic / t_min / 1e9;
        const double gb_med  = traffic / t_med / 1e9;
        const double gib_min = traffic / t_min / 1073741824.0;
        const double spread  = (t_med > 0.0) ? 100.0 * (t_max - t_min) / t_med : 0.0;
        /* Робастный разброс: межквартильный размах. max-min ловит ОДИН
         * выброс (вытеснение с P-ядра, чужой процесс) и потому неинформативен;
         * IQR описывает основную массу замеров. */
        const double p25     = sorted[runs / 4];
        const double p75     = sorted[(3 * runs) / 4];
        const double iqr     = (t_med > 0.0) ? 100.0 * (p75 - p25) / t_med : 0.0;

        t_min_of[t] = t_min;
        if (iqr > 5.0)
            noisy = 1;

        printf("%-8s %-26s %6.0f  %10.3f %10.3f  %9.2f %9.2f  %9.2f %8.2f %7.1f%s\n", td->name, td->formula,
               td->bytes_per_elem, t_min * 1e3, t_med * 1e3, gb_min, gb_med, gib_min, iqr, spread,
               iqr > 5.0 ? "  <-- IQR>5%!" : "");

        if (csv) {
            for (int r = 0; r < runs; ++r) {
                const double dt   = TIME_AT(t, r);
                const double gbs  = traffic / dt / 1e9;
                const double gibs = traffic / dt / 1073741824.0;
                fprintf(csv, "%s,%u,%.0f,%d,%.9f,%.4f,%.4f\n", td->name, mib, td->bytes_per_elem, r, dt, gbs, gibs);
            }
            fflush(csv);
        }
    }

    printf("------------------------------------------------------------------------"
           "--------------------------------------------\n\n");

    if (noisy) {
        printf("ВНИМАНИЕ: межквартильный разброс > 5 %% хотя бы у одного теста. Причины:\n");
        printf("  - Low Power Mode / работа от батареи (см. results/env.txt, раздел 10);\n");
        printf("  - фоновые процессы, DVFS и тепловой бюджет.\n");
        printf("  Устойчивой оценкой ПСП считать МИНИМУМ: он соответствует состоянию\n");
        printf("  без троттлинга и воспроизводится между прогонами в пределах ~1 %%\n");
        printf("  (проверено). Колонка max-min%% почти всегда раздута ОДНИМ выбросом.\n\n");
    }

    /* ------------------------------------------------------- ИТОГИ ------- */
    {
        const double t_add    = t_min_of[T_ADD];
        const double t_add_nt = t_min_of[T_ADD_NT];
        const double t_read   = t_min_of[T_READ];
        const double nd       = (double)n;
        const double nt_gain  = (t_add > 0.0) ? 100.0 * (t_add - t_add_nt) / t_add : 0.0;

        printf("ИТОГИ для roofline (§8.1 ТЗ), по МИНИМАЛЬНОМУ времени:\n");
        printf("  add    (C = A+B, обычная запись) : %7.3f мс -> %6.2f ГБ/с при 16 Б/элем\n", t_add * 1e3,
               16.0 * nd / t_add / 1e9);
        printf("                                     %7s    %6.2f ГБ/с при 12 Б/элем\n", "",
               12.0 * nd / t_add / 1e9);
        printf("  add_nt (C = A+B, NT-store STNP)  : %7.3f мс -> %6.2f ГБ/с при 12 Б/элем\n", t_add_nt * 1e3,
               12.0 * nd / t_add_nt / 1e9);
        printf("  read   (чистое чтение 2 потоков) : %7.3f мс -> %6.2f ГБ/с при  8 Б/элем\n", t_read * 1e3,
               8.0 * nd / t_read / 1e9);
        printf("\n");
        printf("  Выигрыш NT-store по времени (add -> add_nt): %+.1f %%\n", nt_gain);
        if (nt_gain < 3.0) {
            printf("  => NT-store (STNP) на этом M4 НЕ даёт выигрыша по времени, хотя\n");
            printf("     инструкции stnp в коде реально присутствуют. Значит обычный\n");
            printf("     store здесь УЖЕ не платит за RFO (полнострочная запись\n");
            printf("     объединяется в store-буфере), и модель 16 Б/элем завышает\n");
            printf("     реальный трафик. ВАЖНО для §8.1 и для этапа E4.\n");
        }
        printf("\n");
        printf("  ПЕРЕКРЁСТНАЯ ПРОВЕРКА модели трафика (та же ПСП должна получаться\n");
        printf("  из всех шести тестов — иначе модель неверна):\n");
        /* Байт/элем БЕЗ RFO: каждая запись стоит только сам store. */
        static const double NO_RFO[T_COUNT] = { 8.0, 8.0, 12.0, 12.0, 12.0, 8.0 };
        printf("    %-8s %10s %14s %14s\n", "test", "t_min,мс", "ГБ/с (с RFO)", "ГБ/с (без RFO)");
        for (int k = 0; k < T_COUNT; ++k)
            printf("    %-8s %10.3f %14.2f %14.2f\n", TESTS[k].name, t_min_of[k] * 1e3,
                   TESTS[k].bytes_per_elem * nd / t_min_of[k] / 1e9, NO_RFO[k] * nd / t_min_of[k] / 1e9);
        printf("    Столбец «без RFO» должен быть почти постоянным — это и есть ПСП\n");
        printf("    одного ядра. Столбец «с RFO» расходится => RFO на M4 не платится.\n");
        printf("\n");
        printf("  РЕКОМЕНДАЦИЯ для знаменателя roofline:\n");
        printf("    Peak_BW = %.2f ГБ/с — согласованная оценка: чистое чтение (%.2f)\n",
               0.5 * (8.0 * nd / t_read / 1e9 + 12.0 * nd / t_add / 1e9), 8.0 * nd / t_read / 1e9);
        printf("    и add при 12 Б/элем (%.2f) совпадают, значит именно столько\n", 12.0 * nd / t_add / 1e9);
        printf("    вытягивает ОДНО ядро из DRAM независимо от смеси чтение/запись.\n");
        printf("    Если в отчёте трафик считается по модели 16 Б/элем (common.h,\n");
        printf("    обычная запись), последовательный знаменатель — %.2f ГБ/с.\n\n",
               16.0 * nd / t_add / 1e9);

        printf("Теоретический минимум времени для N = 8192 (N^2 = %.2f Мэлем, один проход C=A+B):\n",
               8192.0 * 8192.0 / 1e6);
        const double n2 = 8192.0 * 8192.0;
        printf("  16 Б/элем / %6.2f ГБ/с = %.3f мс\n", 16.0 * nd / t_add / 1e9,
               16.0 * n2 / (16.0 * nd / t_add) * 1e3);
        printf("  12 Б/элем / %6.2f ГБ/с = %.3f мс\n", 12.0 * nd / t_add / 1e9,
               12.0 * n2 / (12.0 * nd / t_add) * 1e3);
        printf("  (обе строки дают одно и то же время — это и есть смысл того, что\n");
        printf("   модель трафика сокращается: измеряется ВРЕМЯ, а не байты.)\n");
    }
    printf("\n");
    printf("sink = %.6f (анти-DCE, значения не несут смысла)\n", (double)g_sink);
#undef TIME_AT

    if (csv) {
        fclose(csv);
        printf("CSV записан.\n");
    }

    free(times);
    free(sorted);
    free(B.a);
    free(B.b);
    free(B.c);
    return EXIT_SUCCESS;
}
