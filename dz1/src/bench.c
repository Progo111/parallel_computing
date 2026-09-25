/* bench.c — измерительная обвязка ДЗ №1 (вариант 3: C = A+B, D = A-B).
 *
 * Реализует §5 (параметры эксперимента), §6 (методика измерений) и §7
 * (контроль корректности) ТЗ. Реестр вариантов порождается из X-макроса
 * VARIANT_LIST в src/common.h — единственного источника правды.
 *
 * Сами ядра живут в отдельных единицах трансляции и вызываются через
 * указатель на функцию, поэтому флаги сборки обвязки на их скорость не влияют.
 */
#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/qos.h>
#include <sys/stat.h>
#include <time.h>

#include "alloc.h"
#include "common.h"
#include "timer.h"
#include "verify.h"

/* --------------------------------------------------------------------------
 * Значения по умолчанию
 * ------------------------------------------------------------------------ */
/* Свип подобран под кэши P-ЯДРА (L1d 128 КиБ, L2 16 МиБ), а не E-ядра:
 * рабочий набор 3*N^2*4 Б (раздельные проходы)
 *   в L1d  : N < 104   -> 64, 96
 *   в L2   : N < 1182  -> 256, 512, 1024
 *   в DRAM : N > 1182  -> 1536, 2048, 4096, 8192 */
#define DEF_SIZES        "64,96,256,512,1024,1536,2048,4096,8192"
#define DEF_RUNS         11
#define DEF_MIN_MS       50.0
#define DEF_OUT          "results/raw.csv"
#define DEF_SHIFT_BYTES  0u
#define DEF_PAUSE_MS     2000u
#define DEF_PAUSE_FROM_N 4096u
#define DEF_SEED         12345ULL

#define REPS_MAX         1000000u
#define SPREAD_WARN      0.05
#define SCRUB_BYTE       0xAA

/* --------------------------------------------------------------------------
 * Реестр вариантов из VARIANT_LIST
 * ------------------------------------------------------------------------ */
typedef struct {
    const char  *stage;
    const char  *tag;
    const char  *flags_id;
    const char  *desc;
    int          nt;      /* 1 — используется non-temporal store */
    mat_binop_fn add;
    mat_binop_fn sub;
    mat_fused_fn addsub;
} variant_t;

#define DZ_REG_ROW(stage_, tag_, flags_, desc_, nt_)                           \
    { (stage_), #tag_, (flags_), (desc_), (nt_),                               \
      MAT_ADD_FN(tag_), MAT_SUB_FN(tag_), MAT_ADDSUB_FN(tag_) },

static const variant_t g_var[] = { VARIANT_LIST(DZ_REG_ROW) };
#undef DZ_REG_ROW

static const size_t g_nvar = sizeof g_var / sizeof g_var[0];

/* Sink (§7.4): не даёт оптимизатору выбросить вычисления. */
static volatile double g_sink = 0.0;

/* Итоговый код возврата: 1, если хоть где-то побитовое расхождение. */
static int g_failed = 0;

/* --------------------------------------------------------------------------
 * Мелкие утилиты
 * ------------------------------------------------------------------------ */
static void usage(FILE *f)
{
    fprintf(f,
        "bench — свип по N для всех вариантов ядер C=A+B / D=A-B (ДЗ №1, вар. 3)\n"
        "\n"
        "Использование:\n"
        "  bench [--sizes %s]\n"
        "        [--runs %d] [--min-ms %.0f] [--out %s]\n"
        "        [--variants tag1,tag2,...] [--modes sep,fused]\n"
        "        [--shift-bytes %u] [--pause-ms %u] [--pause-from-n %u]\n"
        "        [--seed %llu] [--list] [--quiet]\n"
        "\n"
        "  --sizes         список N через запятую (матрицы N x N)\n"
        "  --runs          число замеров на конфигурацию (все попадают в CSV)\n"
        "  --min-ms        минимальная длительность одного замера, мс (подбор REPS)\n"
        "  --out           путь к CSV с сырыми данными\n"
        "  --variants      подмножество тегов из реестра (по умолчанию — все)\n"
        "  --modes         sep (два прохода), fused (слитый) или оба\n"
        "  --shift-bytes   сдвиг баз буферов, кратен %u (эксперимент E4.3);\n"
        "                  применяется с нарастанием: A=0, B=1x, C=2x, D=3x\n"
        "  --pause-ms      пауза между конфигурациями (тепловой режим, §6.3.3)\n"
        "  --pause-from-n  пауза включается только при N >= этого значения\n"
        "  --seed          seed генератора данных (детерминированный xorshift64*)\n"
        "  --list          напечатать реестр вариантов и выйти\n"
        "  --quiet         не печатать прогресс в stdout\n",
        DEF_SIZES, DEF_RUNS, DEF_MIN_MS, DEF_OUT,
        (unsigned)DEF_SHIFT_BYTES, (unsigned)DEF_PAUSE_MS, (unsigned)DEF_PAUSE_FROM_N,
        (unsigned long long)DEF_SEED, (unsigned)CACHELINE);
}

static void die_arg(const char *fmt, const char *a)
{
    fprintf(stderr, "bench: ");
    fprintf(stderr, fmt, a);
    fprintf(stderr, "\nПодсказка: bench --help\n");
    exit(2);
}

/* Разбор опции вида "--name value" или "--name=value". */
static int opt_value(const char *arg, const char *name,
                     int argc, char **argv, int *i, const char **val)
{
    size_t ln = strlen(name);
    if (strncmp(arg, name, ln) != 0) {
        return 0;
    }
    if (arg[ln] == '\0') {
        if (*i + 1 >= argc) {
            die_arg("у опции %s отсутствует значение", name);
        }
        *i += 1;
        *val = argv[*i];
        return 1;
    }
    if (arg[ln] == '=') {
        *val = arg + ln + 1;
        return 1;
    }
    return 0;
}

static unsigned long long parse_ull(const char *s, const char *what)
{
    char *end = NULL;
    errno = 0;
    unsigned long long v = strtoull(s, &end, 10);
    if (end == s || end == NULL || *end != '\0' || errno != 0) {
        fprintf(stderr, "bench: некорректное целое в %s: '%s'\n", what, s);
        exit(2);
    }
    return v;
}

static double parse_double_arg(const char *s, const char *what)
{
    char *end = NULL;
    errno = 0;
    double v = strtod(s, &end);
    if (end == s || end == NULL || *end != '\0' || errno != 0 || !(v > 0.0)) {
        fprintf(stderr, "bench: некорректное положительное число в %s: '%s'\n", what, s);
        exit(2);
    }
    return v;
}

/* Разбор списка N. */
static size_t *parse_sizes(const char *s, size_t *n_out)
{
    size_t cnt = 1;
    for (const char *p = s; *p != '\0'; ++p) {
        if (*p == ',') cnt++;
    }
    size_t *v = (size_t *)malloc(cnt * sizeof *v);
    if (v == NULL) {
        fprintf(stderr, "bench: нет памяти под список --sizes\n");
        exit(1);
    }

    size_t k = 0;
    const char *p = s;
    for (;;) {
        char *end = NULL;
        errno = 0;
        unsigned long long val = strtoull(p, &end, 10);
        if (end == p || errno != 0 || val == 0ULL) {
            die_arg("некорректный список --sizes: '%s'", s);
        }
        v[k++] = (size_t)val;
        if (*end == ',') {
            p = end + 1;
            continue;
        }
        if (*end == '\0') {
            break;
        }
        die_arg("некорректный список --sizes: '%s'", s);
    }
    *n_out = k;
    return v;
}

/* Поиск варианта по тегу; -1 если нет. */
static long find_variant(const char *tag, size_t len)
{
    for (size_t i = 0; i < g_nvar; ++i) {
        if (strlen(g_var[i].tag) == len && strncmp(g_var[i].tag, tag, len) == 0) {
            return (long)i;
        }
    }
    return -1;
}

/* Разбор --variants в список индексов реестра. */
static size_t *parse_variants(const char *s, size_t *n_out)
{
    size_t cnt = 1;
    for (const char *p = s; *p != '\0'; ++p) {
        if (*p == ',') cnt++;
    }
    size_t *idx = (size_t *)malloc(cnt * sizeof *idx);
    if (idx == NULL) {
        fprintf(stderr, "bench: нет памяти под список --variants\n");
        exit(1);
    }

    size_t k = 0;
    const char *p = s;
    while (*p != '\0') {
        const char *comma = strchr(p, ',');
        size_t len = (comma != NULL) ? (size_t)(comma - p) : strlen(p);
        if (len == 0) {
            die_arg("пустой тег в --variants: '%s'", s);
        }
        long f = find_variant(p, len);
        if (f < 0) {
            char buf[64];
            size_t cp = (len < sizeof buf - 1u) ? len : sizeof buf - 1u;
            memcpy(buf, p, cp);
            buf[cp] = '\0';
            die_arg("неизвестный вариант '%s' (см. bench --list)", buf);
        }
        idx[k++] = (size_t)f;
        if (comma == NULL) break;
        p = comma + 1;
    }
    *n_out = k;
    return idx;
}

static void parse_modes(const char *s, int *do_sep, int *do_fused)
{
    *do_sep = 0;
    *do_fused = 0;
    const char *p = s;
    while (*p != '\0') {
        const char *comma = strchr(p, ',');
        size_t len = (comma != NULL) ? (size_t)(comma - p) : strlen(p);
        if (len == 3u && strncmp(p, "sep", 3) == 0) {
            *do_sep = 1;
        } else if (len == 5u && strncmp(p, "fused", 5) == 0) {
            *do_fused = 1;
        } else {
            die_arg("некорректный список --modes: '%s' (допустимо sep,fused)", s);
        }
        if (comma == NULL) break;
        p = comma + 1;
    }
    if (*do_sep == 0 && *do_fused == 0) {
        die_arg("--modes не выбрал ни одного режима: '%s'", s);
    }
}

/* mkdir -p для каталога, в котором лежит path. */
static void ensure_dir_for(const char *path)
{
    char buf[1024];
    size_t len = strlen(path);
    if (len == 0 || len >= sizeof buf) {
        return;
    }
    memcpy(buf, path, len + 1u);

    char *slash = strrchr(buf, '/');
    if (slash == NULL || slash == buf) {
        return;
    }
    *slash = '\0';

    for (char *q = buf + 1; *q != '\0'; ++q) {
        if (*q == '/') {
            *q = '\0';
            (void)mkdir(buf, 0777);
            *q = '/';
        }
    }
    (void)mkdir(buf, 0777);
}

static void sleep_ms(unsigned ms)
{
    if (ms == 0u) {
        return;
    }
    struct timespec ts;
    ts.tv_sec = (time_t)(ms / 1000u);
    ts.tv_nsec = (long)(ms % 1000u) * 1000000L;
    (void)nanosleep(&ts, NULL);
}

/* Поле CSV в кавычках с экранированием (flags_id содержит пробелы/запятые). */
static void csv_quoted(FILE *f, const char *s)
{
    fputc('"', f);
    for (const char *p = s; *p != '\0'; ++p) {
        if (*p == '"') {
            fputc('"', f);
        }
        fputc(*p, f);
    }
    fputc('"', f);
}

/* --------------------------------------------------------------------------
 * Буферы и прогон ядра
 * ------------------------------------------------------------------------ */
typedef struct {
    real_t *A, *B, *C, *D, *refC, *refD;
    size_t  n;      /* размерность матрицы */
    size_t  nelem;  /* n * n */
    size_t  bytes;  /* nelem * sizeof(real_t) */
} buffers_t;

/* Затирание результатов мусором перед замером: «ничего не делающее» ядро
 * не должно пройти верификацию. */
static void scrub_results(const buffers_t *b)
{
    memset(b->C, SCRUB_BYTE, b->bytes);
    memset(b->D, SCRUB_BYTE, b->bytes);
}

static void run_kernel(const variant_t *v, int fused, const buffers_t *b, size_t reps)
{
    if (fused) {
        for (size_t r = 0; r < reps; ++r) {
            v->addsub(b->A, b->B, b->C, b->D, b->n);
            __asm__ volatile("" ::: "memory");
        }
    } else {
        for (size_t r = 0; r < reps; ++r) {
            v->add(b->A, b->B, b->C, b->n);
            v->sub(b->A, b->B, b->D, b->n);
            __asm__ volatile("" ::: "memory");
        }
    }
}

/* --------------------------------------------------------------------------
 * main
 * ------------------------------------------------------------------------ */
int main(int argc, char **argv)
{
    const char *sizes_s    = DEF_SIZES;
    const char *variants_s = NULL;
    const char *modes_s    = "sep,fused";
    const char *out_path   = DEF_OUT;
    int      runs          = DEF_RUNS;
    double   min_ms        = DEF_MIN_MS;
    size_t   shift_bytes   = DEF_SHIFT_BYTES;
    unsigned pause_ms      = DEF_PAUSE_MS;
    size_t   pause_from_n  = DEF_PAUSE_FROM_N;
    uint64_t seed          = DEF_SEED;
    int      do_list       = 0;
    int      quiet         = 0;

    for (int i = 1; i < argc; ++i) {
        const char *a = argv[i];
        const char *val = NULL;

        if (strcmp(a, "--help") == 0 || strcmp(a, "-h") == 0) {
            usage(stdout);
            return 0;
        }
        if (strcmp(a, "--list") == 0)  { do_list = 1; continue; }
        if (strcmp(a, "--quiet") == 0) { quiet = 1;   continue; }

        if (opt_value(a, "--sizes", argc, argv, &i, &val))    { sizes_s = val; continue; }
        if (opt_value(a, "--variants", argc, argv, &i, &val)) { variants_s = val; continue; }
        if (opt_value(a, "--modes", argc, argv, &i, &val))    { modes_s = val; continue; }
        if (opt_value(a, "--out", argc, argv, &i, &val))      { out_path = val; continue; }
        if (opt_value(a, "--runs", argc, argv, &i, &val)) {
            unsigned long long r = parse_ull(val, "--runs");
            if (r == 0ULL || r > 10000ULL) die_arg("--runs вне диапазона 1..10000: '%s'", val);
            runs = (int)r;
            continue;
        }
        if (opt_value(a, "--min-ms", argc, argv, &i, &val)) {
            min_ms = parse_double_arg(val, "--min-ms");
            continue;
        }
        if (opt_value(a, "--shift-bytes", argc, argv, &i, &val)) {
            unsigned long long s = parse_ull(val, "--shift-bytes");
            if (s % (unsigned long long)CACHELINE != 0ULL) {
                die_arg("--shift-bytes должен быть кратен размеру строки кэша: '%s'", val);
            }
            shift_bytes = (size_t)s;
            continue;
        }
        if (opt_value(a, "--pause-from-n", argc, argv, &i, &val)) {
            pause_from_n = (size_t)parse_ull(val, "--pause-from-n");
            continue;
        }
        if (opt_value(a, "--pause-ms", argc, argv, &i, &val)) {
            pause_ms = (unsigned)parse_ull(val, "--pause-ms");
            continue;
        }
        if (opt_value(a, "--seed", argc, argv, &i, &val)) {
            seed = (uint64_t)parse_ull(val, "--seed");
            continue;
        }
        die_arg("неизвестный аргумент '%s'", a);
    }

    if (do_list) {
        printf("%-5s %-10s %-6s %s\n", "stage", "tag", "nt", "desc / flags_id");
        for (size_t i = 0; i < g_nvar; ++i) {
            printf("%-5s %-10s %-6d %s\n        %s\n",
                   g_var[i].stage, g_var[i].tag, g_var[i].nt,
                   g_var[i].desc, g_var[i].flags_id);
        }
        printf("\nвсего вариантов: %zu\n", g_nvar);
        return 0;
    }

    int do_sep = 0, do_fused = 0;
    parse_modes(modes_s, &do_sep, &do_fused);

    size_t nsizes = 0;
    size_t *sizes = parse_sizes(sizes_s, &nsizes);

    size_t nsel = 0;
    size_t *sel = NULL;
    if (variants_s != NULL) {
        sel = parse_variants(variants_s, &nsel);
    } else {
        nsel = g_nvar;
        sel = (size_t *)malloc(nsel * sizeof *sel);
        if (sel == NULL) {
            fprintf(stderr, "bench: нет памяти под реестр вариантов\n");
            return 1;
        }
        for (size_t i = 0; i < nsel; ++i) sel[i] = i;
    }

    /* §6.3.1: обязательный перевод потока в QOS_CLASS_USER_INTERACTIVE,
     * иначе планировщик macOS может увести бенчмарк на E-ядра. */
    errno = 0;
    int qos_rc = pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
    if (qos_rc == 0) {
        fprintf(stderr, "[qos] pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0)"
                        " -> 0 (ok, поток приоритетно на P-ядрах)\n");
    } else {
        fprintf(stderr, "[qos] pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0)"
                        " -> %d (errno=%d: %s) — ВНИМАНИЕ: замеры могут уехать на E-ядра\n",
                qos_rc, errno, strerror(errno));
    }

    double tres = dz_timer_resolution();
    fprintf(stderr, "[timer] CLOCK_MONOTONIC_RAW, разрешение ~%.1f нс\n", tres * 1e9);
    fprintf(stderr, "[cfg]  runs=%d min_ms=%.1f seed=%llu shift_bytes=%zu "
                    "pause_ms=%u pause_from_n=%zu dtype=%s\n",
            runs, min_ms, (unsigned long long)seed, shift_bytes,
            pause_ms, pause_from_n, DTYPE_NAME);

    ensure_dir_for(out_path);
    FILE *out = fopen(out_path, "w");
    if (out == NULL) {
        fprintf(stderr, "bench: не удалось открыть '%s' на запись: %s\n",
                out_path, strerror(errno));
        return 1;
    }
    fprintf(out, "stage,variant,flags_id,dtype,N,mode,bytes_per_elem,reps,"
                 "run_idx,time_sec,gflops,gib_per_sec,checksum,bitexact\n");

    double *times = (double *)malloc((size_t)runs * sizeof *times);
    if (times == NULL) {
        fprintf(stderr, "bench: нет памяти под массив времён\n");
        fclose(out);
        return 1;
    }

    for (size_t zi = 0; zi < nsizes; ++zi) {
        buffers_t buf;
        buf.n = sizes[zi];

        if (buf.n > SIZE_MAX / buf.n) {
            fprintf(stderr, "bench: N=%zu слишком велик (переполнение N*N)\n", buf.n);
            g_failed = 1;
            continue;
        }
        buf.nelem = buf.n * buf.n;
        buf.bytes = buf.nelem * sizeof(real_t);

        /* Буферы выделяются один раз на N и переиспользуются всеми вариантами.
         * Сдвиг нарастающий (A=0, B=1x, C=2x, D=3x) — только так --shift-bytes
         * действительно разносит буферы по наборам кэша (E4.3). */
        buf.A    = dz_alloc_matrix(buf.nelem, 0u);
        buf.B    = dz_alloc_matrix(buf.nelem, shift_bytes);
        buf.C    = dz_alloc_matrix(buf.nelem, 2u * shift_bytes);
        buf.D    = dz_alloc_matrix(buf.nelem, 3u * shift_bytes);
        buf.refC = dz_alloc_matrix(buf.nelem, 0u);
        buf.refD = dz_alloc_matrix(buf.nelem, 0u);

        dz_prefault(buf.A, buf.bytes);
        dz_prefault(buf.B, buf.bytes);
        dz_prefault(buf.C, buf.bytes);
        dz_prefault(buf.D, buf.bytes);
        dz_prefault(buf.refC, buf.bytes);
        dz_prefault(buf.refD, buf.bytes);

        dz_fill_random(buf.A, buf.nelem, seed);
        dz_fill_random(buf.B, buf.nelem, seed ^ 0x9E3779B97F4A7C15ULL);

        /* §7.1: эталон — ядро E0 (-O0), считается один раз на каждое N. */
        MAT_ADD_FN(e0_O0)(buf.A, buf.B, buf.refC, buf.n);
        MAT_SUB_FN(e0_O0)(buf.A, buf.B, buf.refD, buf.n);
        uint64_t ref_cs = dz_fnv1a_cont(dz_fnv1a(buf.refC, buf.bytes), buf.refD, buf.bytes);

        if (!quiet) {
            printf("N=%zu  (%.2f МиБ на матрицу, рабочий набор %.2f МиБ, эталон checksum=%016" PRIx64 ")\n",
                   buf.n, (double)buf.bytes / 1048576.0,
                   4.0 * (double)buf.bytes / 1048576.0, ref_cs);
            fflush(stdout);
        }

        for (size_t si = 0; si < nsel; ++si) {
            const variant_t *v = &g_var[sel[si]];

            for (int fused = 0; fused <= 1; ++fused) {
                if (fused && !do_fused) continue;
                if (!fused && !do_sep)  continue;

                const char *mode = fused ? "fused" : "sep";
                const double bpe = dz_bytes_per_elem(fused, v->nt);

                /* §6.3.3: пауза для теплового режима только на больших N. */
                if (pause_ms > 0u && buf.n >= pause_from_n) {
                    sleep_ms(pause_ms);
                }

                /* §5.2.2: калибровка REPS по одному пробному прогону. */
                scrub_results(&buf);
                double c0 = dz_wtime();
                run_kernel(v, fused, &buf, 1u);
                double t1 = dz_wtime() - c0;

                size_t reps;
                if (t1 > 0.0) {
                    double want = ceil((min_ms * 1e-3) / t1);
                    if (!(want >= 1.0)) {
                        want = 1.0;
                    }
                    if (want > (double)REPS_MAX) {
                        want = (double)REPS_MAX;
                    }
                    reps = (size_t)want;
                } else {
                    reps = REPS_MAX; /* таймер не разрешил один проход */
                }

                /* §6.2.1: один прогрев, не учитывается. */
                scrub_results(&buf);
                run_kernel(v, fused, &buf, reps);

                int cfg_reported = 0;

                /* §6.2.2: runs замеров подряд, сохраняем ВСЕ. */
                for (int run = 0; run < runs; ++run) {
                    scrub_results(&buf);

                    double s0 = dz_wtime();
                    run_kernel(v, fused, &buf, reps);
                    double elapsed = dz_wtime() - s0;

                    double t = elapsed / (double)reps;
                    times[run] = t;

                    /* §7.4: sink — чтение элементов C и D в volatile. */
                    size_t sidx = ((size_t)run * 7919u) % buf.nelem;
                    g_sink += (double)buf.C[sidx] + (double)buf.D[sidx];

                    /* §7.2: побитовая сверка с эталоном. */
                    int bitexact = 1;
                    size_t fdiff = 0;
                    const char *which = NULL;
                    if (dz_bitcmp(buf.C, buf.refC, buf.bytes, &fdiff) != 0) {
                        bitexact = 0;
                        which = "C";
                    } else if (dz_bitcmp(buf.D, buf.refD, buf.bytes, &fdiff) != 0) {
                        bitexact = 0;
                        which = "D";
                    }
                    if (!bitexact) {
                        g_failed = 1;
                        if (!cfg_reported) {
                            cfg_reported = 1;
                            fprintf(stderr,
                                    "FAIL %s %s N=%zu first_diff=%zu  buf=%s elem=%zu\n",
                                    v->tag, mode, buf.n, fdiff, which,
                                    fdiff / sizeof(real_t));
                        }
                    }

                    /* §7.3: контрольная сумма по C и D (скомбинированная). */
                    uint64_t cs = dz_fnv1a_cont(dz_fnv1a(buf.C, buf.bytes),
                                                buf.D, buf.bytes);

                    double flop   = 2.0 * (double)buf.nelem;
                    double bytes  = bpe * 2.0 * (double)buf.nelem;
                    double gflops = (t > 0.0) ? flop / t / 1e9 : 0.0;
                    double gibs   = (t > 0.0) ? bytes / t / 1073741824.0 : 0.0;

                    fprintf(out, "%s,%s,", v->stage, v->tag);
                    csv_quoted(out, v->flags_id);
                    fprintf(out, ",%s,%zu,%s,%.6f,%zu,%d,%.9e,%.6f,%.6f,%016" PRIx64 ",%d\n",
                            DTYPE_NAME, buf.n, mode, bpe, reps, run,
                            t, gflops, gibs, cs, bitexact);
                }

                dz_stats_t st = dz_stats(times, (size_t)runs);

                /* §6.2.3: разброс > 5% — прогон зашумлён. */
                if (st.spread > SPREAD_WARN) {
                    fprintf(stderr,
                            "[warn] spread=%.2f%% > %.0f%%: N=%zu %s %s "
                            "(min=%.6e max=%.6e sd=%.3e)\n",
                            st.spread * 100.0, SPREAD_WARN * 100.0,
                            buf.n, v->tag, mode, st.min, st.max, st.sd);
                }

                if (!quiet) {
                    double bytes = bpe * 2.0 * (double)buf.nelem;
                    double gibs   = (st.min > 0.0) ? bytes / st.min / 1073741824.0 : 0.0;
                    double gflops = (st.min > 0.0) ? 2.0 * (double)buf.nelem / st.min / 1e9 : 0.0;
                    printf("  %-10s %-5s reps=%-7zu min=%.6e s  med=%.6e s  "
                           "%8.2f ГиБ/с  %7.2f GFLOP/s  spread=%5.2f%%  %s\n",
                           v->tag, mode, reps, st.min, st.median, gibs, gflops,
                           st.spread * 100.0, cfg_reported ? "BITEXACT-FAIL" : "ok");
                    fflush(stdout);
                }
                fflush(out);
            }
        }

        dz_free_matrix(buf.refD);
        dz_free_matrix(buf.refC);
        dz_free_matrix(buf.D);
        dz_free_matrix(buf.C);
        dz_free_matrix(buf.B);
        dz_free_matrix(buf.A);
    }

    fclose(out);
    free(times);
    free(sel);
    free(sizes);

    if (!quiet) {
        printf("\nCSV: %s\n", out_path);
        printf("sink=%.6e (не используется, защита от DCE)\n", g_sink);
    }
    if (g_failed) {
        fprintf(stderr, "bench: ЕСТЬ побитовые расхождения с эталоном — см. строки FAIL выше\n");
    }
    return g_failed ? 1 : 0;
}
