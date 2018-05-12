#include <poll_priv.h>
#include <sys/epoll.h>

struct epoll_event pevents[MAX_EVENTS] = {{ 0 }};

int poll_create(void) {
    return epoll_create1(EPOLL_CLOEXEC);
}

int poll_set_data(void **_ev, void *p) {
    *_ev = malloc(sizeof(struct epoll_event));
    MOD_ASSERT(*_ev, "Failed to malloc", MOD_ERR);
    struct epoll_event *ev = (struct epoll_event *)*_ev;

    ev->data.ptr = p;
    ev->events = EPOLLIN;
    return MOD_OK;
}

int poll_set_new_evt(module_poll_t *tmp, m_context *c, enum op_type flag) {
    int f = flag == ADD ? EPOLL_CTL_ADD : EPOLL_CTL_DEL;
    return epoll_ctl(c->fd, f, tmp->fd, (struct epoll_event *)tmp->ev);
}

int poll_wait(int fd, int num_fds) {
    return epoll_wait(fd, pevents, num_fds, -1);
}

module_poll_t *poll_recv(int idx) {
    return (module_poll_t *)pevents[idx].data.ptr;
}

int poll_close(int fd) {
    return close(fd);
}
