#include "poll_priv.h"
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/timerfd.h>
#include <sys/inotify.h>
#include <limits.h>

/* Inotify related defines */
#define BUF_LEN (sizeof(struct inotify_event) + NAME_MAX + 1)

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
            tmp->tm_src.f.fd = timerfd_create(tmp->tm_src.its.clock_id, TFD_NONBLOCK | TFD_CLOEXEC);
            struct itimerspec timerValue = {{0}};
            timerValue.it_value.tv_sec = tmp->tm_src.its.ms / 1000;
            timerValue.it_value.tv_nsec = (tmp->tm_src.its.ms % 1000) * 1000 * 1000;
            if (!(tmp->flags & SRC_ONESHOT)) {
                /* Set interval */
                timerValue.it_interval.tv_sec = tmp->tm_src.its.ms / 1000;
                timerValue.it_interval.tv_nsec = (tmp->tm_src.its.ms % 1000) * 1000 * 1000;
            }
            timerfd_settime(tmp->tm_src.f.fd, 0, &timerValue, NULL);
        }
        fd = tmp->tm_src.f.fd;
        break;
    }
    case TYPE_SGN: {
        if (flag == ADD) {
            sigset_t mask;
            sigemptyset(&mask);
            sigaddset(&mask, tmp->sgn_src.sgs.signo);
            sigprocmask(SIG_BLOCK, &mask, NULL);
            tmp->sgn_src.f.fd = signalfd(-1, &mask, 0);
        }
        fd = tmp->sgn_src.f.fd;
        break;
    }
    case TYPE_PT: {
        if (flag == ADD) {
            tmp->pt_src.f.fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
            inotify_add_watch(tmp->pt_src.f.fd, tmp->pt_src.pt.path, tmp->pt_src.pt.op_flag);
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
        
        if (tmp->type == TYPE_SGN || tmp->type == TYPE_TMR || tmp->type == TYPE_PT) {
            close(fd); // automatically close internally used FDs
        }
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

mod_ret poll_consume_sgn(poll_priv_t *priv, const int idx, ev_src_t *src, sgn_msg_t *sgn_msg) {
    struct signalfd_siginfo fdsi;
    ssize_t s = read(src->sgn_src.f.fd, &fdsi, sizeof(struct signalfd_siginfo));
    if (s == sizeof(struct signalfd_siginfo)) {
        return MOD_OK;
    }
    return MOD_ERR;
}

mod_ret poll_consume_tmr(poll_priv_t *priv, const int idx, ev_src_t *src, tm_msg_t *tm_msg) {
    uint64_t t;
    if (read(src->tm_src.f.fd, &t, sizeof(uint64_t)) == sizeof(uint64_t)) {
        return MOD_OK;
    }
    return MOD_ERR;
}

mod_ret poll_consume_pt(poll_priv_t *priv, const int idx, ev_src_t *src, pt_msg_t *pt_msg) {
    char buffer[BUF_LEN];
    const size_t length = read(src->pt_src.f.fd, buffer, BUF_LEN);
    if (length > 0) {
        struct inotify_event *event = (struct inotify_event *) buffer;
        if (event->len) {
            pt_msg->events = event->mask;
            return MOD_OK;
        }
    }
    return MOD_ERR;
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
