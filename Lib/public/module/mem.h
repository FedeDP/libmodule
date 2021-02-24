#pragma once

#ifndef LIBMODULE_MEM_H
    #define LIBMODULE_MEM_H
#endif
#include "cmn.h"

/* Keep a mem ref'd object alive untile end of fn */
#define M_MEM_LOCK(ptr, fn) \
    m_mem_ref(ptr); \
    fn; \
    m_mem_unref(ptr);

typedef void (*m_ref_dtor)(void *);

void *m_mem_new(size_t size, m_ref_dtor dtor);
void *m_mem_ref(void *src);
void *m_mem_unref(void *src);
void m_mem_unrefp(void **src);
size_t m_mem_size(void *src);
