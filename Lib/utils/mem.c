#include "priv.h"

typedef struct {
    size_t refs;
    m_ref_dtor dtor;
    uint8_t data[]; // Use flexible array member
} mem_header_t;

static inline mem_header_t *get_header(uint8_t *src) {
    return (mem_header_t *)(src - sizeof(mem_header_t));
}

/** Private API **/
void mem_dtor(void *src) {
    m_mem_unref(src);
}

/** Public API **/

/* Create new ref counted memory area */
void *m_mem_new(size_t size, m_ref_dtor dtor) {
    mem_header_t *header = memhook._calloc(1, sizeof(mem_header_t) + size);
    if (header) {
        header->refs = 1;
        header->dtor = dtor;
        return header->data;
    }
    return NULL;
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
void *m_mem_unref(void *src) {
    if (src) {
        mem_header_t *header = get_header(src);
        if (--header->refs == 0) {
            if (header->dtor) {
                header->dtor(src); // destroy private data
            }
            memhook._free(header);
        }
    }
    return NULL;
}

void m_mem_unrefp(void **src) {
    if (src) {
        *src = m_mem_unref(*src);
    }
}
