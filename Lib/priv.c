#include "priv.h"

/*******************************************
 * Code related to generic priv utilities. *
 *******************************************/

static void evt_dtor(void *data) {
    evt_priv_t *evt = (evt_priv_t *)data;
    m_mem_unref(evt->src);
    /* We use fd_evt as all messages share address inside union */
    m_mem_unref(evt->evt.fd_evt);
}

/** Private API **/

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

void fetch_ms(uint64_t *val, uint64_t *ctr) {
    struct timespec spec;
#ifdef __linux__
    clock_gettime(CLOCK_BOOTTIME, &spec);
#else
    clock_gettime(CLOCK_MONOTONIC, &spec);
#endif
    *val = spec.tv_sec * 1000 + spec.tv_nsec / 1000000;
    
    if (ctr) {
        (*ctr)++;
    }
}

evt_priv_t *new_evt(ev_src_t *src) {
    evt_priv_t *msg = m_mem_new(sizeof(evt_priv_t), evt_dtor);
    if (msg) {
        msg->evt.type = src->type;
        msg->src = m_mem_ref(src);
    }
    return msg;
}

bool str_not_empty(const char *str) {
    return str && str[0] != '\0';
}
