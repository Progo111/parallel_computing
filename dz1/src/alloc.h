/* alloc.h — выделение/освобождение выровненных матричных буферов (§3.1.3 ТЗ).
 *
 * Требования ТЗ:
 *   - база выровнена на границу страницы (DZ_PAGESIZE = 16384 на macOS/arm64),
 *     возвращаемый указатель — минимум на CACHELINE (128 Б);
 *   - опциональный сдвиг базы (shift_bytes, кратен CACHELINE) для эксперимента
 *     E4.3 «cache set aliasing» — разнесение A/B/C/D по наборам кэша;
 *   - обязательный prefault (проход ЗАПИСЬЮ по всем страницам) до замеров;
 *   - детерминированное заполнение значениями из [1.0, 2.0) — §5.3.
 */
#ifndef DZ1_ALLOC_H
#define DZ1_ALLOC_H

#include <stddef.h>
#include <stdint.h>

#include "common.h"

/* Выделяет nelem элементов real_t. База выровнена на DZ_PAGESIZE, затем
 * сдвинута на shift_bytes (обязан быть кратен CACHELINE). Возвращённый
 * указатель всегда выровнен минимум на CACHELINE. При нехватке памяти или
 * некорректных аргументах печатает сообщение в stderr и завершает процесс. */
real_t *dz_alloc_matrix(size_t nelem, size_t shift_bytes);

/* Освобождает буфер, полученный из dz_alloc_matrix (умеет снимать сдвиг). */
void dz_free_matrix(real_t *p);

/* Проход ЗАПИСЬЮ по всем страницам диапазона [p, p+bytes) с шагом DZ_PAGESIZE:
 * снимает lazy-allocation, иначе первый прогон меряет page faults. */
void dz_prefault(void *p, size_t bytes);

/* Детерминированный xorshift64*: значения строго в [1.0f, 2.0f)
 * (ни денормалов, ни inf). Один seed -> побитово одинаковые данные. */
void dz_fill_random(real_t *p, size_t nelem, uint64_t seed);

#endif /* DZ1_ALLOC_H */
