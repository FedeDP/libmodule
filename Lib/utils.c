#include "priv.h"
#include "mem.h"

typedef struct {
    size_t refs;
    ref_dtor dtor;
} header_t;

static inline header_t *get_header(void *src);

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

static inline header_t *get_header(void *src) {
    return (header_t *)(((uint8_t *)src) - sizeof(header_t));
}

/* Create new ref counted memory area */
void *mem_new(size_t size, ref_dtor dtor) {
    header_t *header = memhook._calloc(1, size + sizeof(header_t));
    header->refs = 1;
    header->dtor = dtor;
    return &header[1];
}

/* Gain a new ref on a memory area */
void mem_ref(void *src) {
    if (src) {
        header_t *header = get_header(src);
        header->refs++;
    }
}

/* Remove a ref from a memory area */
void mem_unref(void *src) {
    if (src) {
        header_t *header = get_header(src);
        if (--header->refs == 0) {
            if (header->dtor) {
                header->dtor(src); // destroy internal data
            }
            memhook._free(header);
        }
    }
}
