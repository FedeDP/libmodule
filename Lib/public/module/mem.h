#pragma once

#ifndef LIBMODULE_MEM_H
    #define LIBMODULE_MEM_H
#endif
#include "cmn.h"

#define M_MEM_LOCK(ptr, fn) \
    m_mem_ref(ptr); \
    fn; \
    m_mem_unref(ptr);

typedef void (*m_ref_dtor)(void *);

_public_ void *m_mem_new(size_t size, m_ref_dtor dtor);
_public_ void *m_mem_ref(void *src);
_public_ void *m_mem_unref(void *src);
_public_ void m_mem_unrefp(void **src);
