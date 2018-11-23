#include "poll_priv.h"
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

int poll_create(void) {
    int fd = kqueue();
    fcntl(fd, F_SETFD, FD_CLOEXEC);
    return fd;
}

int poll_set_data(void **_ev) {
    *_ev = memhook._malloc(sizeof(struct kevent));
    MOD_ALLOC_ASSERT(*_ev);
    return MOD_OK;
}

int poll_set_new_evt(module_poll_t *tmp, m_context *c, enum op_type flag) {
    int f = flag == ADD ? EV_ADD : EV_DELETE;
    struct kevent *_ev = (struct kevent *)tmp->ev;
    EV_SET(_ev, tmp->fd, EVFILT_READ, f, 0, 0, (void *)tmp);
    int ret = kevent(c->fd, _ev, 1, NULL, 0, NULL);
    /* Workaround for STDIN_FILENO: it is actually pollable */
    if (tmp->fd == STDIN_FILENO) {
        ret = 0;
    }
    return ret;
}

int poll_init_pevents(void **pevents, int max_events) {
    *pevents = memhook._calloc(max_events, sizeof(struct kevent));
    MOD_ALLOC_ASSERT(*pevents);
    return MOD_OK;
}

int poll_wait(int fd, int max_events, void *pevents) {
    return kevent(fd, NULL, 0, (struct kevent *)pevents, max_events, NULL);
}

module_poll_t *poll_recv(int idx, void *pevents) {
    struct kevent *pev = (struct kevent *)pevents;
    if (pev[idx].flags & EV_ERROR) {
        return NULL;
    }
    return (module_poll_t *)pev[idx].udata;
}

int poll_destroy_pevents(void **pevents, int *max_events) {
    memhook._free(*pevents);
    *pevents = NULL;
    *max_events = 0;
    return MOD_OK;
}

int poll_close(int fd, void **pevents, int *max_events) {
    poll_destroy_pevents(pevents, max_events);
    return close(fd);
}
