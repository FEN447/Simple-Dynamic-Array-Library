#ifndef DA_DNMC_ARR_H
#define DA_DNMC_ARR_H

#include <stdint.h>
#include <stddef.h>
#include <limits.h>

#ifdef DARR_FAST
#   define DARR_FAST_MODE 1
#else
#   define DARR_FAST_MODE 0
#endif

#ifndef DARR_SIZE_MAX_64
#ifndef DARR_SIZE_MAX_32
#ifndef DARR_SIZE_MAX_16
#   define DARR_SIZE_MAX_CALC SIZE_MAX
#   define DARR_BITS_MAX_CALC (sizeof(size_t) * 8)
#endif
#endif
#endif

#if (defined(DARR_SIZE_MAX_64) + defined(DARR_SIZE_MAX_32) + defined(DARR_SIZE_MAX_16)) > 1
#   error "Multiple DARR_SIZE_MAX macros defined."
#endif

#ifdef DARR_SIZE_MAX_64
#   define DARR_SIZE_MAX_CALC UINT64_MAX
#   define DARR_BITS_MAX_CALC 64
#endif
#ifdef DARR_SIZE_MAX_32
#   define DARR_SIZE_MAX_CALC UINT32_MAX
#   define DARR_BITS_MAX_CALC 32
#endif
#ifdef DARR_SIZE_MAX_16
#   define DARR_SIZE_MAX_CALC UINT16_MAX
#   define DARR_BITS_MAX_CALC 16
#endif

#define DARR_STAT_EMPTY 0
#define DARR_STAT_OK 1
#define DARR_STAT_MOVED 2
#define DARR_STAT_FAIL 3

#define DARR_FUNC_OK 0
#define DARR_FUNC_FAIL -1

void* darr_init(size_t esize);
void* darr_setlen(void* darr, size_t len);
void* darr_addlen(void* darr, size_t len);
void* darr_clear(void* darr);
void* darr_reset(void* darr, size_t esize);
void* darr_push(void* darr, void* data);
char* darr_stat(void* darr, char* buf, size_t len);
int darr_stat_clear(void* darr);
int64_t darr_tot(void* darr);
int64_t darr_len(void* darr);
int64_t darr_rem(void* darr);
int darr_free(void* darr);

#endif
