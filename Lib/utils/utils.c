#include "utils.h"
#include <time.h>

void fetch_ms(uint64_t *val, uint64_t *ctr) {
    struct timespec spec;
#ifdef CLOCK_BOOTTIME
    clock_gettime(CLOCK_BOOTTIME, &spec);
#else
    clock_gettime(CLOCK_MONOTONIC, &spec);
#endif
    *val = spec.tv_sec * 1000 + spec.tv_nsec / 1000000;
    
    if (ctr) {
        (*ctr)++;
    }
}

bool str_not_empty(const char *str) {
    return str && str[0] != '\0';
}
