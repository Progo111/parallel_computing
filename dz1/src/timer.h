/* timer.h — монотонный таймер и статистика по прогонам (§6.1, §6.2 ТЗ). */
#ifndef DZ1_TIMER_H
#define DZ1_TIMER_H

#include <stddef.h>

/* Текущее время, секунды (CLOCK_MONOTONIC_RAW — не подвержен NTP-подстройке). */
double dz_wtime(void);

/* Оценка разрешения таймера: минимальная ненулевая разница двух
 * последовательных вызовов dz_wtime() из ~1000 попыток, секунды. */
double dz_timer_resolution(void);

/* Сводка по серии замеров. spread = (max - min) / min — оценка шума (§6.2.3). */
typedef struct {
    double min;
    double max;
    double median;
    double mean;
    double sd;     /* выборочное СКО (нормировка на n-1) */
    double spread; /* (max - min) / min */
} dz_stats_t;

/* Считает статистику по v[0..n-1]. Входной массив НЕ изменяется. */
dz_stats_t dz_stats(const double *v, size_t n);

#endif /* DZ1_TIMER_H */
