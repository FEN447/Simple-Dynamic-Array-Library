#include "dnmc_arr.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <stdalign.h>

#define DA_FAST_MODE DARR_FAST_MODE

#define DA_STAT_EMPTY DARR_STAT_EMPTY
#define DA_STAT_OK DARR_STAT_OK
#define DA_STAT_MOVED DARR_STAT_MOVED
#define DA_STAT_FAIL DARR_STAT_FAIL

#define DA_FUNC_OK DARR_FUNC_OK
#define DA_FUNC_FAIL DARR_FUNC_FAIL
 
_Static_assert(DARR_SIZE_MAX_CALC <= SIZE_MAX,
    "DARR_SIZE_MAX should not exceed SIZE_MAX.\n");
#define DA_SIZE_MAX DARR_SIZE_MAX_CALC
#define DA_STAT_LEN_MAX DARR_BITS_MAX_CALC / 2

#define DA_ALIGN alignof(max_align_t)

#define DA_MAX_ALLOC ((DA_SIZE_MAX >> 1) + 1)

#define MAX(a, b) ((a) > (b) ? (a) : (b))

static inline size_t pow2_next(size_t n) {
    assert(n <= DA_MAX_ALLOC);
    if(n <= 1) {
        return 1;
    }
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    #if DA_SIZE_MAX >= UINT16_MAX
        n |= n >> 8;
    #if DA_SIZE_MAX >= UINT32_MAX
        n |= n >> 16;
    #if DA_SIZE_MAX >= UINT64_MAX
        n |= n >> 32;
    #endif
    #endif
    #endif
    return n + 1;
}

typedef struct {
    size_t len;
    size_t esize;
    size_t alloc;
    void *body;
    size_t stat;
} header_base;

#define DA_PAD_SIZE ((DA_ALIGN - (sizeof(header_base) % DA_ALIGN)) % DA_ALIGN)

typedef struct {
    size_t len;
    size_t esize;
    size_t alloc;
    void *body;
    size_t stat;
    unsigned char pad[DA_PAD_SIZE];
} header;

#define DA_MIN_ALLOC (pow2_next(sizeof(header)))

typedef struct {
    void *value;
    size_t index;
} data_in;

static inline header* hdr_get(void* darr) {
    return (header*)darr - 1;
}

static inline void* darr_get(header* hdr) {
    return (void*)(hdr + 1);
}

static inline size_t len_get(size_t alloc, size_t esize) {
    return (alloc - sizeof(header)) / esize;
}

void* darr_init(size_t esize) {
    if(esize == 0 || esize > DA_MAX_ALLOC) {
        return NULL;
    }
    header *hdr = (header*)malloc(DA_MIN_ALLOC);
    if(hdr == NULL) {
        return NULL;
    }
    void *darr = darr_get(hdr);
    hdr->len = 0;
    hdr->alloc = DA_MIN_ALLOC;
    hdr->esize = esize;
    hdr->body = darr;
    hdr->stat = 0;
    return darr;
}

#ifdef DA_FAST_MODE
#   define darr_test(d) 1
static inline int darr_fast_test(void* darr) {
    if(darr == NULL) {
        return 0;
    }
    if((uintptr_t)darr % DA_ALIGN != 0) {
        return 0;
    }
    header *hdr = hdr_get(darr);
    if(hdr == NULL) {
        return 0;
    }
    if((uintptr_t)hdr % DA_ALIGN != 0) {
        return 0;
    }
    return 1;
}
#else
#   define darr_fast_test(d) 1
static inline int hdr_mat(header* hdr) {
    if(hdr == NULL) {
        return 0;
    }
    if((uintptr_t)hdr % DA_ALIGN != 0) {
        return 0;
    }
    if(hdr->body != darr_get(hdr)) {
        return 0;
    }
    return 1;
}

static inline int hdr_vld(header* hdr) {
    if((hdr->alloc & (hdr->alloc - 1)) != 0) {
        return 0;
    }
    if(hdr->alloc < DA_MIN_ALLOC || hdr->alloc > DA_MAX_ALLOC) {
        return 0;
    }
    if(hdr->esize == 0) {
        return 0;
    }
    if(hdr->len > len_get(hdr->alloc, hdr->esize)) {
        return 0;
    }
    return 1;
}

static inline int darr_mat(void* darr) {
    if(darr == NULL) {
        return 0;
    }
    if((uintptr_t)darr % DA_ALIGN != 0) {
        return 0;
    }
    return 1;
}

static inline int darr_test(void* darr) {
    if(!darr_mat(darr)) {
        return 0;
    }
    header *hdr = hdr_get(darr);
    if(!hdr_mat(hdr)) {
        return 0;
    }
    if(!hdr_vld(hdr)) {
        return 0;
    }
    return 1;
}
#endif

static inline int update_arg_mat(header* hdr, size_t len, size_t esize, data_in *data) {
    if(esize == 0 || esize > DA_MAX_ALLOC) {
        return 0;
    }
    if(esize != hdr->esize && len != 0) {
        return 0;
    }
    if(len > len_get(DA_MAX_ALLOC, esize)) {
        return 0;
    }
    if(data != NULL && (data->value == NULL || data->index >= len)) {
        return 0;
    }
    return 1;
}

static inline void stat_push(size_t *stat, size_t code) {
    *stat <<= 2;
    *stat |= code;
}

static void* darr_update(void *darr, size_t len, size_t esize, data_in *data) {
    header *hdr = hdr_get(darr);
    if(!update_arg_mat(hdr, len, esize, data)) {
        stat_push(&hdr->stat, DA_STAT_FAIL);
        return darr;
    }
    if(len == hdr->len && data == NULL) {
        stat_push(&hdr->stat, DA_STAT_OK);
        return darr;
    }
    size_t alloc = hdr->alloc;
    size_t target = MAX(sizeof(header) + len * esize, DA_MIN_ALLOC);
    while(alloc < target) {
        alloc *= 2;
    }
    while((alloc / 2) > target) {
        alloc /= 2;
    }
    size_t stat_bk = hdr->stat;
    header *new_hdr = (header*)realloc((void*)hdr, alloc);
    if(new_hdr == NULL) {
        stat_push(&hdr->stat, DA_STAT_FAIL);
        return darr;
    }
    void *new_darr = darr_get(new_hdr);
    new_hdr->len = len;
    new_hdr->alloc = alloc;
    new_hdr->esize = esize;
    new_hdr->body = new_darr;
    new_hdr->stat = stat_bk;
    if(new_hdr != hdr) {
        stat_push(&new_hdr->stat, DA_STAT_MOVED);
    } else {
        stat_push(&new_hdr->stat, DA_STAT_OK);
    }
    if(data != NULL) {
        void *ptr = (unsigned char*)new_darr + new_hdr->esize * data->index;
        memcpy(ptr, data->value, new_hdr->esize);
    }
    return new_darr;
}

static inline int addlen_safe(header* hdr, size_t len) {
    size_t max_len = len_get(DA_MAX_ALLOC, hdr->esize);
    if(hdr->len > max_len) {
        return 0;
    }
    if(len > max_len - hdr->len) {
        return 0;
    }
    return 1;
}

void* darr_setlen(void* darr, size_t len) {
    if(!darr_test(darr) || !darr_fast_test(darr)) {
        return NULL;
    }
    return darr_update(darr, len, hdr_get(darr)->esize, NULL);
}

void* darr_addlen(void* darr, size_t len) {
    if(!darr_test(darr) || !darr_fast_test(darr)) {
        return NULL;
    }
    header *hdr = hdr_get(darr);
    if(!addlen_safe(hdr, len)) {
        stat_push(&hdr->stat, DA_STAT_FAIL);
        return darr;
    }
    return darr_update(darr, hdr->len + len, hdr->esize, NULL);
}

void* darr_clear(void* darr) {
    if(!darr_test(darr) || !darr_fast_test(darr)) {
        return NULL;
    }
    return darr_update(darr, 0, hdr_get(darr)->esize, NULL);
}

void* darr_reset(void* darr, size_t esize) {
    if(!darr_test(darr) || !darr_fast_test(darr)) {
        return NULL;
    }
    return darr_update(darr, 0, esize, NULL);
}

void* darr_push(void* darr, void* data) {
    if(!darr_test(darr) || !darr_fast_test(darr)) {
        return NULL;
    }
    header *hdr = hdr_get(darr);
    if(!addlen_safe(hdr, 1)) {
        stat_push(&hdr->stat, DA_STAT_FAIL);
        return darr;
    }
    data_in din = {data, hdr->len};
    return darr_update(darr, hdr->len + 1, hdr->esize, &din);
}

static inline int buf_arg_mat(const char* buf, size_t len) {
    if(len < 2 || buf == NULL) {
        return 0;
    }
    return 1;
}

char* darr_stat(void* darr, char* buf, size_t len) {
    if(!buf_arg_mat(buf, len) || !darr_test(darr) || !darr_fast_test(darr)) {
        return NULL;
    }
    size_t init = (len > DA_STAT_LEN_MAX + 1) ? len - DA_STAT_LEN_MAX - 1 : 0;
    for(int i = 0; i < (int)init; i++) {
        buf[i] = '0';
    }
    size_t stat = hdr_get(darr)->stat;
    for(int i = (int)len - 2; i >= (int)init; i--) {
        buf[i] = '0' + (stat & 3);
        stat >>= 2;
    }
    buf[len-1] = '\0';
    return buf;
}

int darr_stat_clear(void* darr) {
    if(!darr_test(darr) || !darr_fast_test(darr)) {
        return DA_FUNC_FAIL;
    }
    hdr_get(darr)->stat = 0;
    return DA_FUNC_OK;
}

int64_t darr_tot(void* darr) {
    if(!darr_test(darr) || !darr_fast_test(darr)) {
        return DA_FUNC_FAIL;
    }
    header *hdr = hdr_get(darr);
    return (int64_t)len_get(hdr->alloc, hdr->esize);
}

int64_t darr_len(void* darr) {
    if(!darr_test(darr) || !darr_fast_test(darr)) {
        return DA_FUNC_FAIL;
    }
    return (int64_t)(hdr_get(darr)->len);
}

int64_t darr_rem(void* darr) {
    if(!darr_test(darr) || !darr_fast_test(darr)) {
        return DA_FUNC_FAIL;
    }
    header *hdr = hdr_get(darr);
    return (int64_t)(len_get(hdr->alloc, hdr->esize) - hdr->len);
}

int darr_free(void* darr) {
    if(!darr_test(darr) || !darr_fast_test(darr)) {
        return DA_FUNC_FAIL;
    }
    free(hdr_get(darr));
    return DA_FUNC_OK;
}
