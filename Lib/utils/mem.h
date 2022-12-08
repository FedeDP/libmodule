#pragma once

#include <stddef.h>

char *mem_strdup(const char *s);

/* Struct that holds user defined memory functions */
typedef struct {
    void *(*_malloc)(size_t size);
    void *(*_calloc)(size_t nmemb, size_t size);
    void (*_free)(void *ptr);
} m_memhook_t;

extern m_memhook_t memhook;
