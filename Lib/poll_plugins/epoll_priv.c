#include "poll_priv.h"
#include <sys/epoll.h>

extern void create_timerfd(ev_src_t *tmp);
extern void create_signalfd(ev_src_t *tmp);
extern void create_inotifyfd(ev_src_t *tmp);
extern void create_pidfd(ev_src_t *tmp);
extern void reset_fd(ev_src_t *tmp);

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
    if (!tmp->ev) {
        if (flag == ADD) {
            tmp->ev = memhook._calloc(1, sizeof(struct epoll_event));
            MOD_ALLOC_ASSERT(tmp->ev);
        } else {
            /* We need to RM an unregistered ev. Fine. */
            return MOD_OK;
        }
    }
    
    int f = flag == ADD ? EPOLL_CTL_ADD : EPOLL_CTL_DEL;
    struct epoll_event *ev = (struct epoll_event *)tmp->ev;
    ev->data.ptr = tmp;
    ev->events = EPOLLIN;
    if (tmp->flags & SRC_ONESHOT) {
        ev->events |= EPOLLONESHOT;
    }
    
    errno = 0;
    int fd = -1;
    switch (tmp->type) {
    case TYPE_PS: // TYPE_PS is used for pubsub_fd[0] in init_pubsub_fd()
    case TYPE_FD:
        fd = tmp->fd_src.fd;
        break;
    case TYPE_TMR: {
        if (flag == ADD) {
            create_timerfd(tmp);
        }
        fd = tmp->tm_src.f.fd;
        break;
    }
    case TYPE_SGN: {
        if (flag == ADD) {
            create_signalfd(tmp);
        }
        fd = tmp->sgn_src.f.fd;
        break;
    }
    case TYPE_PT: {
        if (flag == ADD) {
            create_inotifyfd(tmp);
        }
        fd = tmp->pt_src.f.fd;
        break;
    }
    case TYPE_PID: {
        if (flag == ADD) {
            create_pidfd(tmp);
        }
        fd = tmp->pt_src.f.fd;
        break;
    }
    default:
        break;
    }
    
    int ret = epoll_ctl(ep->fd, f, fd, ev);

    /* Workaround for STDIN_FILENO: it returns EPERM but it is actually pollable */
    if (ret == -1 && fd == STDIN_FILENO && errno == EPERM) {
        ret = 0;
    }
    
    /* Eventually free epoll data if needed */
    if (flag == RM) {
        memhook._free(tmp->ev);
        tmp->ev = NULL;
        
        /*
         * Automatically close internally used FDs 
         * for special internal fds 
         */
        reset_fd(tmp); 
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
    if (ep->pevents[idx].events & EPOLLERR) {
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
