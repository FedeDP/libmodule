#include "poll_priv.h"
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

int poll_create(void) {
#ifdef __APPLE__
    return kqueue();
#else
    return kqueue1(O_CLOEXEC);
#endif
}

int poll_set_data(void **_ev, void *p) {
    *_ev = malloc(sizeof(struct kevent));
    return MOD_OK;
}

int poll_set_new_evt(module_poll_t *tmp, m_context *c, enum op_type flag) {
    int f = flag == ADD ? EV_ADD : EV_DELETE;
    struct kevent *_ev = (struct kevent *)tmp->ev;
    EV_SET(_ev, tmp->fd, EVFILT_READ, f, 0, 0, (void *)tmp);
    return kevent(c->fd, _ev, 1, NULL, 0, NULL);
}

int poll_init_pevents(void **pevents, int max_events) {
    *pevents = memhook._calloc(max_events, sizeof(struct kevent));
    if (*pevents) {
        return MOD_OK;
    }
    return MOD_ERR;
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

int _pipe(int fd[2]) {
    int ret = pipe(fd);
    if (ret == 0) {
        for (int i = 0; i < 2; i++) {
            int flags = fcntl(fd[i], F_GETFL, 0);
            if (flags == -1) {
                flags = 0;
            }
            fcntl(fd[i], F_SETFL, flags | O_NONBLOCK);
        }
    }
    return ret;
}
