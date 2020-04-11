#pragma once

#include "module_cmn.h"

#define MEM_LOCK(ptr, fn) \
    mem_ref(ptr); \
    fn; \
    mem_unref(ptr);

typedef void (*ref_dtor)(void *);

_public_ void *mem_new(size_t size, ref_dtor dtor);
_public_ void *mem_ref(void *src);
_public_ void mem_unref(void *src);
