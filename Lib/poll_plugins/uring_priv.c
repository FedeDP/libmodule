#include "poll_priv.h"
#include <liburing.h>
#include <sys/poll.h>

/*
 * Doc: https://kernel.dk/io_uring.pdf
 * 
 * Naming: 
 * SQE -> Submission Queue Event
 * CQE -> Completion Queue Event
 * 
 * Note that we need to pre-allocate MODULES_MAX_EVENTS in poll_create() (calling poll_init())
 * else any fd registered before poll_init() (and thus before ring is actually created)
 * would failt to be registered.
 */

/**
 * TODO:
 * 
 * * Avoid re-arming each time fd in poll_recv()
 * * Avoid pre-allocating MODULES_MAX_EVENTS uring in poll_create()
 * * Avoid io_uring_submit() in poll_wait() ?
 */

typedef struct {
    struct io_uring ring;
    struct io_uring_cqe *cqe;
    bool inited;
} uring_priv_t;

#define GET_PRIV_DATA()     uring_priv_t *up = (uring_priv_t *)priv->data

mod_ret poll_create(poll_priv_t *priv) {
    priv->data = memhook._calloc(1, sizeof(uring_priv_t));
    MOD_ALLOC_ASSERT(priv->data);
    priv->max_events = MODULES_MAX_EVENTS;
    return poll_init(priv);
}

int poll_new_data(poll_priv_t *priv, void **_ev) {
    GET_PRIV_DATA();
    *_ev = io_uring_get_sqe(&up->ring);
    MOD_ALLOC_ASSERT(*_ev);
    return MOD_OK;
}

mod_ret poll_free_data(poll_priv_t *priv, void **_ev) {
    *_ev = NULL;
    return MOD_OK;
}

int poll_set_new_evt(poll_priv_t *priv, ev_src_t *tmp, const enum op_type flag) {
    int ret = 0;
    GET_PRIV_DATA();
    struct io_uring_sqe *sqe = (struct io_uring_sqe *)tmp->fd_src.ev;
    if (flag == ADD) {
        io_uring_prep_poll_add(sqe, tmp->fd_src.fd, POLLIN);
        io_uring_sqe_set_data(sqe, tmp);
    } else if (up->cqe) {
        io_uring_prep_poll_remove(sqe, tmp);
        ret = -(up->cqe->res != 0); // -1 on error
    }
    return ret;
}

mod_ret poll_init(poll_priv_t *priv) {
    GET_PRIV_DATA();
    if (up->inited || 
        io_uring_queue_init(priv->max_events, &up->ring, IORING_SETUP_IOPOLL) == 0) {
        
        up->inited = true;
        return MOD_OK;
    }
    return MOD_ERR;
}

int poll_wait(poll_priv_t *priv, const int timeout) {
    GET_PRIV_DATA();
    /*
     * TODO: is this needed? 
     * Doc states otherwise: 
     * > Note that the application need not call io_uring_submit() before calling
     * > this function, as we will do that on its behalf.
     * 
     * But without it, it won't work.
     */
    io_uring_submit(&up->ring);
    struct __kernel_timespec t = {0};
    t.tv_sec = timeout;
    int ret = io_uring_wait_cqe_timeout(&up->ring, &up->cqe, timeout >= 0 ? &t : NULL);
    if (ret == 0) {
        return 1; // single event is returned
    }
    return ret; // errno error code
}

ev_src_t *poll_recv(poll_priv_t *priv, const int idx) {
    GET_PRIV_DATA();
    ev_src_t *udata = (ev_src_t *)io_uring_cqe_get_data(up->cqe);
    io_uring_cqe_seen(&up->ring, up->cqe);
    
    /* udata != NULL only if fd is still registered */
    if (udata) {
        /* We need to re-arm it: IORING_OP_POLL_ADD interface works in oneshot mode */
        poll_new_data(priv, &udata->fd_src.ev);
        poll_set_new_evt(priv, udata, ADD);
    }
    return udata;
}

int poll_get_fd(poll_priv_t *priv) {
    GET_PRIV_DATA();
    return up->ring.ring_fd;
}

mod_ret poll_clear(poll_priv_t *priv) {
    GET_PRIV_DATA();
    io_uring_queue_exit(&up->ring);
    up->cqe = NULL;
    up->inited = false;
    return MOD_OK;
}

mod_ret poll_destroy(poll_priv_t *priv) {
    GET_PRIV_DATA();
    if (up->inited) { 
        poll_clear(priv);
    }
    memhook._free(up);
    return MOD_OK;
}
