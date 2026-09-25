/* verify.h — контроль корректности (§7 ТЗ): побитовая сверка с эталоном E0
 * и контрольная сумма FNV-1a по байтам результата. */
#ifndef DZ1_VERIFY_H
#define DZ1_VERIFY_H

#include <stddef.h>
#include <stdint.h>

/* FNV-1a, 64 бита, по байтам буфера. */
uint64_t dz_fnv1a(const void *p, size_t bytes);

/* Продолжение хеша FNV-1a (для комбинирования нескольких буферов). */
uint64_t dz_fnv1a_cont(uint64_t h, const void *p, size_t bytes);

/* Побитовое сравнение. Возврат 0 — буферы совпали; иначе 1 и, если
 * first_diff != NULL, туда записывается индекс первого различающегося байта. */
int dz_bitcmp(const void *a, const void *b, size_t bytes, size_t *first_diff);

#endif /* DZ1_VERIFY_H */
