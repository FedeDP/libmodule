#include "poll_priv.h"
#include <sys/epoll.h>

int poll_create(void) {
    return epoll_create1(EPOLL_CLOEXEC);
}

int poll_set_data(void **_ev) {
    *_ev = memhook._calloc(1, sizeof(struct epoll_event));
    MOD_ALLOC_ASSERT(*_ev);
    return MOD_OK;
}

int poll_set_new_evt(fd_priv_t *tmp, ctx_t *c, const enum op_type flag) {
    int f = flag == ADD ? EPOLL_CTL_ADD : EPOLL_CTL_DEL;
    struct epoll_event *ev = (struct epoll_event *)tmp->ev;
    ev->data.ptr = tmp;
    ev->events = EPOLLIN;
    int ret = epoll_ctl(c->fd, f, tmp->fd, (struct epoll_event *)tmp->ev);
    /* Workaround for STDIN_FILENO: it returns EPERM but it is actually pollable */
    if (ret == -1 && tmp->fd == STDIN_FILENO && errno == EPERM) {
        ret = 0;
    }
    return ret;
}

int poll_init_pevents(void **pevents, const int max_events) {
    *pevents = memhook._calloc(max_events, sizeof(struct epoll_event));
    MOD_ALLOC_ASSERT(*pevents);
    return MOD_OK;
}

int poll_wait(const int fd, const int max_events, void *pevents, const int timeout) {
    return epoll_wait(fd, (struct epoll_event *) pevents, max_events, timeout);
}

fd_priv_t *poll_recv(const int idx, void *pevents) {
    struct epoll_event *pev = (struct epoll_event *) pevents;
    return (fd_priv_t *)pev[idx].data.ptr;
}

int poll_destroy_pevents(void **pevents, int *max_events) {
    memhook._free(*pevents);
    *pevents = NULL;
    *max_events = 0;
    return MOD_OK;
}

int poll_close(const int fd, void **pevents, int *max_events) {
    poll_destroy_pevents(pevents, max_events);
    return close(fd);
}
