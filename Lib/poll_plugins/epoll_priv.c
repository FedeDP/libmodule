#include "poll_priv.h"
#include <sys/epoll.h>

typedef struct {
    int fd;
    struct epoll_event *pevents;
} epoll_priv_t;

#define GET_PRIV_DATA()     epoll_priv_t *ep = (epoll_priv_t *)priv->data

mod_ret poll_create(poll_priv_t *priv) {
    priv->data = memhook._calloc(1, sizeof(epoll_priv_t));
    MOD_ALLOC_ASSERT(priv->data);    
    GET_PRIV_DATA();
    ep->fd = epoll_create1(EPOLL_CLOEXEC);
    return ep->fd != -1 ? MOD_OK : MOD_ERR;
}

int poll_set_new_evt(poll_priv_t *priv, ev_src_t *tmp, const enum op_type flag) {
    GET_PRIV_DATA();
    
    /* Eventually alloc epoll data if needed */
    if (!tmp->fd_src.ev) {
        if (flag == ADD) {
            tmp->fd_src.ev = memhook._calloc(1, sizeof(struct epoll_event));
            MOD_ALLOC_ASSERT(tmp->fd_src.ev);
        } else {
            /* We need to RM an unregistered ev. Fine. */
            return MOD_OK;
        }
    }
    
    int f = flag == ADD ? EPOLL_CTL_ADD : EPOLL_CTL_DEL;
    struct epoll_event *ev = (struct epoll_event *)tmp->fd_src.ev;
    ev->data.ptr = tmp;
    ev->events = EPOLLIN;
    int ret = epoll_ctl(ep->fd, f, tmp->fd_src.fd, ev);
    /* Workaround for STDIN_FILENO: it returns EPERM but it is actually pollable */
    if (ret == -1 && tmp->fd_src.fd == STDIN_FILENO && errno == EPERM) {
        ret = 0;
    }
    
    /* Eventually free epoll data if needed */
    if (flag == RM) {
        memhook._free(tmp->fd_src.ev);
        tmp->fd_src.ev = NULL;
    }
    
    return ret;
}

mod_ret poll_init(poll_priv_t *priv) {
    GET_PRIV_DATA();
    ep->pevents = memhook._calloc(priv->max_events, sizeof(struct epoll_event));
    MOD_ALLOC_ASSERT(ep->pevents);
    return MOD_OK;
}

int poll_wait(poll_priv_t *priv, const int timeout) {
    GET_PRIV_DATA();
    return epoll_wait(ep->fd, (struct epoll_event *) ep->pevents, priv->max_events, timeout);
}

ev_src_t *poll_recv(poll_priv_t *priv, const int idx) {
    GET_PRIV_DATA();
    if (ep->pevents->events & EPOLLERR) {
        return NULL;
    }
    return (ev_src_t *)ep->pevents[idx].data.ptr;
}

int poll_get_fd(poll_priv_t *priv) {
    GET_PRIV_DATA();
    return ep->fd;
}

mod_ret poll_clear(poll_priv_t *priv) {
    GET_PRIV_DATA();
    memhook._free(ep->pevents);
    ep->pevents = NULL;
    return MOD_OK;
}

mod_ret poll_destroy(poll_priv_t *priv) {
    GET_PRIV_DATA();
    poll_clear(priv);
    close(ep->fd);
    memhook._free(ep);
    return MOD_OK;
}
