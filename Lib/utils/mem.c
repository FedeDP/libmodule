#include "priv.h"

typedef struct {
    size_t refs;
    m_ref_dtor dtor;
} mem_header_t;

static inline mem_header_t *get_header(void *src);

static inline mem_header_t *get_header(void *src) {
    return (mem_header_t *)(((uint8_t *)src) - sizeof(mem_header_t));
}

/* Create new ref counted memory area */
void *m_mem_new(size_t size, m_ref_dtor dtor) {
    mem_header_t *header = memhook._calloc(1, size + sizeof(mem_header_t));
    header->refs = 1;
    header->dtor = dtor;
    return &header[1];
}

/* Gain a new ref on a memory area */
void *m_mem_ref(void *src) {
    if (src) {
        mem_header_t *header = get_header(src);
        header->refs++;
    }
    return src;
}

/* Remove a ref from a memory area */
void m_mem_unref(void *src) {
    if (src) {
        mem_header_t *header = get_header(src);
        if (--header->refs == 0) {
            if (header->dtor) {
                header->dtor(src); // destroy internal data
            }
            memhook._free(header);
        }
    }
}
