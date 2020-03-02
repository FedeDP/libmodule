#include "poll_priv.h"
#include <liburing.h>
#include <sys/poll.h>

/*
 * Doc: https://kernel.dk/io_uring.pdf
 * 
 * Naming: 
 * SQE -> Submission Queue Event
 * CQE -> Completion Queue Event
 */

/**
 * TODO:
 * * Avoid re-arming each time fd in poll_recv()
 * * Avoid io_uring_submit() in poll_wait() ?
 */

static int is_src_same(void *userdata, void *data);
static void flush_reqs(poll_priv_t *priv);

typedef struct {
    struct io_uring ring;
    struct io_uring_cqe **cqe;
    bool inited;
    mod_list_t *req_list;           // keep a list of all requests while ring is not yet inited
} uring_priv_t;

#define GET_PRIV_DATA()     uring_priv_t *up = (uring_priv_t *)priv->data

mod_ret poll_create(poll_priv_t *priv) {
    priv->data = memhook._calloc(1, sizeof(uring_priv_t));
    MOD_ALLOC_ASSERT(priv->data);
    GET_PRIV_DATA();
    up->req_list = list_new(NULL);
    return MOD_OK;
}

static int is_src_same(void *userdata, void *data) {
    return !(userdata == data);
}

int poll_set_new_evt(poll_priv_t *priv, ev_src_t *tmp, const enum op_type flag) {
    GET_PRIV_DATA();
    
    if (up->inited) {
        /* Eventually request uring sqe if needed */
        if (!tmp->fd_src.ev) {
            if (flag == ADD) {
                tmp->fd_src.ev = io_uring_get_sqe(&up->ring);
                MOD_ALLOC_ASSERT(tmp->fd_src.ev);
            } else {
                /* We need to RM an unregistered ev. Fine. */
                return MOD_OK;
            }
        }
        
        struct io_uring_sqe *sqe = (struct io_uring_sqe *)tmp->fd_src.ev;
        if (flag == ADD) {
            io_uring_prep_poll_add(sqe, tmp->fd_src.fd, POLLIN);
            io_uring_sqe_set_data(sqe, tmp);
        } else {
            io_uring_prep_poll_remove(sqe, tmp);
        }
        
        /* Eventually release uring sqe if needed */
        if (flag == RM) {
            tmp->fd_src.ev = NULL;
        }
    } else {
        /* 
         * Keep a list of all requests while ring is not inited; 
         * we will register any request as soon as ring gets inited.
         */
        if (flag == ADD) {
            list_insert(up->req_list, tmp, NULL);
        } else {
            list_remove(up->req_list, tmp, is_src_same);
        }
    }
    return 0;
}

static void flush_reqs(poll_priv_t *priv) {
    GET_PRIV_DATA();
    for (mod_list_itr_t *itr = list_itr_new(up->req_list); itr; itr = list_itr_next(itr)) {
        ev_src_t *tmp = list_itr_get_data(itr);
        poll_set_new_evt(priv, tmp, ADD);
    }
    list_clear(up->req_list);
}

mod_ret poll_init(poll_priv_t *priv) {
    GET_PRIV_DATA();
    if (io_uring_queue_init(priv->max_events, &up->ring, IORING_SETUP_IOPOLL) == 0) {
        up->cqe = memhook._calloc(priv->max_events, sizeof(struct io_uring_cqe *));
        MOD_ALLOC_ASSERT(up->cqe);
        up->inited = true;
        flush_reqs(priv);
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
    int ret = io_uring_wait_cqe_timeout(&up->ring, up->cqe, timeout >= 0 ? &t : NULL);
    if (ret == 0) {
        int cqe_count = io_uring_peek_batch_cqe(&up->ring, up->cqe, priv->max_events);
        return cqe_count;
    }
    return ret; // errno error code
}

ev_src_t *poll_recv(poll_priv_t *priv, const int idx) {
    GET_PRIV_DATA();
    ev_src_t *udata = (ev_src_t *)io_uring_cqe_get_data(up->cqe[idx]);
    io_uring_cqe_seen(&up->ring, up->cqe[idx]);
    
    /* udata != NULL only if fd is still registered */
    if (udata) {
        /* We need to re-arm it: IORING_OP_POLL_ADD interface works in oneshot mode */
        udata->fd_src.ev = NULL;
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
    memhook._free(up->cqe);
    up->cqe = NULL;
    up->inited = false;
    return MOD_OK;
}

mod_ret poll_destroy(poll_priv_t *priv) {
    GET_PRIV_DATA();
    if (up->inited) { 
        poll_clear(priv);
    }
    list_free(up->req_list);
    memhook._free(up);
    return MOD_OK;
}
