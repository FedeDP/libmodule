#include "priv.h"

typedef struct {
    uint32_t refs;
    ref_dtor dtor;
} m_header_t;

char *mem_strdup(const char *s) {
    char *new = NULL;
    if (s) {
        const int len = strlen(s) + 1;
        new = memhook._malloc(len);
        if (new) {
            memcpy(new, s, len);
        }
    }
    return new;
}

void fetch_ms(uint64_t *val, uint64_t *ctr) {
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    *val = spec.tv_sec * 1000 + spec.tv_nsec / 1000000;
    
    if (ctr) {
        (*ctr)++;
    }
}

static inline m_header_t *get_header(void *src) {
    return (m_header_t *)(((uint8_t *)src) - sizeof(m_header_t));
}

/* Create new ref counted memory area */
void *mem_ref_new(size_t size, ref_dtor dtor) {
    m_header_t *header = memhook._calloc(1, size + sizeof(m_header_t));
    header->refs = 1;
    header->dtor = dtor;
    return &header[1];
}

/* Gain a new ref on a memory area */
void mem_ref(void *src) {
    m_header_t *header = get_header(src);
    header->refs++;
}

/* Remove a ref from a memory area */
void mem_unref(void *src) {
    m_header_t *header = get_header(src);
    if (--header->refs == 0) {
        if (header->dtor) {
            header->dtor(src); // destroy internal data
        }
        memhook._free(header);
    }
}
