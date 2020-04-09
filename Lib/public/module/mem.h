#pragma once

#include "module_cmn.h"

#define MEM_LOCK(ptr, fn) \
    mem_ref(ptr); \
    fn; \
    mem_unref(ptr);

typedef void (*ref_dtor)(void *);

void *mem_new(size_t size, ref_dtor dtor);
void mem_ref(void *src);
void mem_unref(void *src);
