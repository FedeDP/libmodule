#pragma once

#define M_MEM_LOCK(mem, func) \
    m_mem_ref(mem); \
    func; \
    m_mem_unref(mem);

char *mem_strdup(const char *s);
void mem_dtor(void *src);
