#pragma once

#include <stddef.h>

#define M_MEM_LOCK(mem, func) \
    m_mem_ref(mem); \
    func; \
    m_mem_unref(mem);

char *mem_strdup(const char *s);
void mem_dtor(void *src);

/* Struct that holds user defined memory functions */
typedef struct {
    void *(*_malloc)(size_t size);
    void *(*_calloc)(size_t nmemb, size_t size);
    void (*_free)(void *ptr);
} m_memhook_t;

extern m_memhook_t memhook;
