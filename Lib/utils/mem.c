#include "mem.h"
#include <stdlib.h>
#include <string.h>

m_memhook_t memhook = { malloc, calloc, free };

char *mem_strdup(const char *s) {
    char *new = NULL;
    if (s) {
        const size_t len = strlen(s) + 1;
        new = memhook._malloc(len);
        if (new) {
            memcpy(new, s, len);
        }
    }
    return new;
}
