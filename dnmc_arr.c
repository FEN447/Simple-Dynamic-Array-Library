#include "dnmc_arr.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

_Static_assert(DA_INIT_ALLOC >= 64,
    "DA_INIT_ALLOC shouldn't less than 64");

_Static_assert((DA_INIT_ALLOC & (DA_INIT_ALLOC - 1)) == 0,
    "DA_INIT_ALLOC should be a power of 2");

typedef struct {
    size_t len;
    size_t alloc;
    size_t esize;
    int stat;
} header;

static header* hdr_get(void* darr) {
    return (header*)darr - 1;
}

static void* darr_get(header* hdr) {
    return (void*)(hdr + 1);
}

void* darr_init(size_t esize) {
    if(esize == 0) {
        return NULL;
    }
    header *hdr = (header*)malloc(DA_INIT_ALLOC);
    if(hdr == NULL) {
        return NULL;
    }
    hdr->len = 0;
    hdr->alloc = DA_INIT_ALLOC;
    hdr->esize = esize;
    hdr->stat = 0;
    return darr_get(hdr);
}

static int hdr_test(header* hdr) {
    if(hdr->len > (hdr->alloc - sizeof(header)) / hdr->esize) {
        return 0;
    } else if((hdr->alloc & (hdr->alloc - 1)) != 0) {
        return 0;
    } else if(hdr->alloc < DA_INIT_ALLOC) {
        return 0;
    } else if(hdr->stat < -1 || hdr->stat > 1) {
        return 0;
    }
    return 1;
}

static int darr_test(void* darr) {
    if((uintptr_t)darr % sizeof(void*) != 0) {
        return 0;
    }
    if(darr == NULL) {
        return 0;
    }
    if(!hdr_test(hdr_get(darr))) {
        return 0;
    }
    return 1;
}

static size_t total_get(size_t esize, size_t len) {
    return sizeof(header) + esize * len;
}

static header* hdr_update(header* hdr, size_t len) {
    if(len > (SIZE_MAX - sizeof(header)) / hdr->esize) {
        hdr->stat = -1;
        return NULL;
    }
    size_t total = MAX(total_get(hdr->esize, len), DA_INIT_ALLOC);
    size_t alloc = hdr->alloc;
    while(alloc < total) {
        alloc *= 2;
    }
    while((alloc / 2) > total) {
        alloc /= 2;
    }
    header *new_hdr = (header*)realloc((void*)hdr, alloc);
    if(new_hdr == NULL) {
        hdr->stat = -1;
        return NULL;
    }
    new_hdr->len = len;
    new_hdr->alloc = alloc;
    new_hdr->esize = hdr->esize;
    new_hdr->stat = (new_hdr != hdr) ? 1 : 0;
    return new_hdr;
}

void* darr_setlen(void* darr, size_t len) {
    if(!darr_test(darr)) {
        return NULL;
    }
    header *new_hdr = hdr_update(hdr_get(darr), len);
    if(new_hdr == NULL) {
        return darr;
    }
    return darr_get(new_hdr);
}

void* darr_addlen(void* darr, size_t len) {
    if(!darr_test(darr)) {
        return NULL;
    }
    if(len == 0) {
        return darr;
    }
    header *hdr = hdr_get(darr);
    if(len > SIZE_MAX - hdr->len) {
        hdr->stat = -1;
        return darr;
    }
    return darr_setlen(darr, hdr->len + len);
}

void* darr_clear(void* darr) {
    if(!darr_test(darr)) {
        return NULL;
    }
    header *hdr = (header*)darr - 1;
    void *new_darr = realloc(hdr, DA_INIT_ALLOC);
    if(new_darr == NULL) {
        hdr->stat = -1;
        return darr;
    }
    header *new_hdr = hdr_get(new_darr);
    new_hdr->len = 0;
    new_hdr->alloc = DA_INIT_ALLOC;
    new_hdr->stat = (new_darr != darr) ? 1 : 0;
    new_hdr->esize = hdr->esize;
    return new_darr;
}

void* darr_reset(void* darr, size_t esize) {
    void *new_darr = darr_clear(darr);
    if(new_darr == NULL) {
        return NULL;
    }
    hdr_get(new_darr)->esize = esize;
    return new_darr;
}

static void* darr_add(void* darr, void* data) {
    header *hdr = hdr_get(darr);
    void *ptr = (unsigned char*)darr + hdr->esize * (hdr->len - 1);
    memcpy(ptr, data, hdr->esize);
    return darr;
}

void* darr_push(void* darr, void* data) {
    if(!darr_test(darr)) {
        return NULL;
    }
    header *hdr = hdr_get(darr);
    size_t total = total_get(hdr->esize, hdr->len + 1);
    if(hdr->alloc >= total) {
        hdr->stat = 0;
        hdr->len++;
        return darr_add(darr, data);
    }
    void *new_darr = darr_addlen(darr, 1);
    if(hdr_get(new_darr)->stat == -1) {
        hdr->stat = -1;
        return darr;
    }
    return darr_add(new_darr, data);
}

int darr_stat(void* darr) {
    if(!darr_test(darr)) {
        return -2;
    }
    return hdr_get(darr)->stat;
}

long long darr_tot(void* darr) {
    if(!darr_test(darr)) {
        return -2;
    }
    header *hdr = hdr_get(darr);
    return (long long)((hdr->alloc - sizeof(header)) / hdr->esize);
}

long long darr_len(void* darr) {
    if(!darr_test(darr)) {
        return -2;
    }
    return (long long)(hdr_get(darr)->len);
}

long long darr_rem(void* darr) {
    if(!darr_test(darr)) {
        return -2;
    }
    header *hdr = hdr_get(darr);
    return (long long)((hdr->alloc - sizeof(header)) / hdr->esize - hdr->len);
}

int darr_free(void* darr) {
    if(!darr_test(darr)) {
        return -2;
    }
    free(hdr_get(darr));
    return 0;
}