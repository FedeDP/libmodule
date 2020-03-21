#include "priv.h"

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
