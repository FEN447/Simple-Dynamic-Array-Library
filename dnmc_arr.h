#ifndef DA_DNMC_ARR_H
#define DA_DNMC_ARR_H

#include <stddef.h>

#ifndef DA_INIT_ALLOC
#define DA_INIT_ALLOC 64
#endif

void* darr_init(size_t esize);
void* darr_setlen(void* darr, size_t len);
void* darr_addlen(void* darr, size_t len);
void* darr_clear(void* darr);
void* darr_reset(void* darr, size_t esize);
void* darr_push(void* darr, void* data);
int darr_stat(void* darr);
long long darr_tot(void* darr);
long long darr_len(void* darr);
long long darr_rem(void* darr);
int darr_free(void* darr);

#endif