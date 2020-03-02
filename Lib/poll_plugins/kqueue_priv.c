#include "poll_priv.h"
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

typedef struct {
    int fd;
    struct kevent *pevents;
} kqueue_priv_t;

#define GET_PRIV_DATA()     kqueue_priv_t *kp = (kqueue_priv_t *)priv->data

mod_ret poll_create(poll_priv_t *priv) {
    priv->data = memhook._calloc(1, sizeof(kqueue_priv_t));
    MOD_ALLOC_ASSERT(priv->data);    
    GET_PRIV_DATA();
    kp->fd = kqueue();
    fcntl(kp->fd, F_SETFD, FD_CLOEXEC);
    return MOD_OK;
}

int poll_set_new_evt(poll_priv_t *priv, ev_src_t *tmp, const enum op_type flag) {
    GET_PRIV_DATA();
    
    /* Eventually alloc kqueue data if needed */
    if (!tmp->fd_src.ev) {
        if (flag == ADD) {
            tmp->fd_src.ev = memhook._calloc(1, sizeof(struct kevent));
            MOD_ALLOC_ASSERT(tmp->fd_src.ev);
        } else {
            /* We need to RM an unregistered ev. Fine. */
            return MOD_OK;
        }
    }
    
    int f = flag == ADD ? EV_ADD : EV_DELETE;
    struct kevent *_ev = (struct kevent *)tmp->fd_src.ev;
    EV_SET(_ev, tmp->fd_src.fd, EVFILT_READ, f, 0, 0, (void *)tmp);
    int ret = kevent(kp->fd, _ev, 1, NULL, 0, NULL);
    /* Workaround for STDIN_FILENO: it is actually pollable */
    if (tmp->fd_src.fd == STDIN_FILENO) {
        ret = 0;
    }
    
    /* Eventually free kqueue data if needed */
    if (flag == RM) {
        memhook._free(tmp->fd_src.ev);
        tmp->fd_src.ev = NULL;
    }
    
    return ret;
}

mod_ret poll_init(poll_priv_t *priv) {
    GET_PRIV_DATA();
    kp->pevents = memhook._calloc(priv->max_events, sizeof(struct kevent));
    MOD_ALLOC_ASSERT(kp->pevents);
    return MOD_OK;
}

int poll_wait(poll_priv_t *priv, const int timeout) {
    GET_PRIV_DATA();
    struct timespec t = {0};
    t.tv_sec = timeout;
    return kevent(kp->fd, NULL, 0, kp->pevents, priv->max_events, timeout >= 0 ? &t : NULL);
}

ev_src_t *poll_recv(poll_priv_t *priv, const int idx) {
    GET_PRIV_DATA();
    if (kp->pevents[idx].flags & EV_ERROR) {
        return NULL;
    }
    return (ev_src_t *)kp->pevents[idx].udata;
}

int poll_get_fd(poll_priv_t *priv) {
    GET_PRIV_DATA();
    return kp->fd;
}

mod_ret poll_clear(poll_priv_t *priv) {
    GET_PRIV_DATA();
    memhook._free(kp->pevents);
    kp->pevents = NULL;
    return MOD_OK;
}

mod_ret poll_destroy(poll_priv_t *priv) {
    GET_PRIV_DATA();
    poll_clear(priv);
    close(kp->fd);
    memhook._free(kp);
    return MOD_OK;
}
