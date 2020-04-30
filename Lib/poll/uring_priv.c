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

extern void create_priv_fd(ev_src_t *tmp);
static void flush_reqs(poll_priv_t *priv);

typedef struct {
    struct io_uring ring;
    struct io_uring_cqe **cqe;
    bool inited;
    m_list_t *req_list;           // keep a list of all requests while ring is not yet inited
} uring_priv_t;

#define GET_PRIV_DATA()     uring_priv_t *up = (uring_priv_t *)priv->data

int poll_create(poll_priv_t *priv) {
    priv->data = memhook._calloc(1, sizeof(uring_priv_t));
    M_ALLOC_ASSERT(priv->data);
    GET_PRIV_DATA();
    up->req_list = m_list_new(NULL);
    M_ALLOC_ASSERT(up->req_list);
    return 0;
}

int poll_set_new_evt(poll_priv_t *priv, ev_src_t *tmp, const enum op_type flag) {
    GET_PRIV_DATA();
    
    int ret = 0;
    if (up->inited) {
        /* Eventually request uring sqe if needed */
        if (!tmp->ev) {
            if (flag == ADD) {
                tmp->ev = io_uring_get_sqe(&up->ring);
                M_ALLOC_ASSERT(tmp->ev);
            } else {
                /* We need to RM an unregistered ev. Fine. */
                return 0;
            }
        }
        
        struct io_uring_sqe *sqe = (struct io_uring_sqe *)tmp->ev;
        if (flag == ADD && tmp->fd_src.fd == -1) {
            create_priv_fd(tmp);
        }
        
        /* 
         * Note that fd_src shares
         * memory space with other fds (inside union)
         */
        const int fd = tmp->fd_src.fd;
        if (fd != -1) {
            if (flag == ADD) {
                io_uring_prep_poll_add(sqe, fd, POLLIN);
                /* properly set userdata */
                io_uring_sqe_set_data(sqe, tmp);
            } else {
                io_uring_prep_poll_remove(sqe, tmp);
                tmp->ev = NULL;
                /*
                 * Automatically close internally used FDs 
                 * for special internal fds 
                 */
                if (tmp->type > TYPE_FD) {
                    close(fd);
                    /* 
                     * Reset to -1. Note that fd_src has same
                     * memory space as other fds (inside union)
                     */
                    tmp->fd_src.fd = -1;
                }
            }
        } else {
            ret = -1;
        }
    } else {
        /* 
         * Keep a list of all requests while ring is not inited; 
         * we will register any request as soon as ring gets inited.
         */
        if (flag == ADD) {
            m_list_insert(up->req_list, tmp, NULL);
        } else {
            m_list_remove(up->req_list, tmp, NULL);
        }
    }
    return ret;
}

static void flush_reqs(poll_priv_t *priv) {
    GET_PRIV_DATA();
    m_itr_foreach(up->req_list, {
        ev_src_t *tmp = m_itr_get(itr);
        poll_set_new_evt(priv, tmp, ADD);
    });
    m_list_clear(up->req_list);
}

int poll_init(poll_priv_t *priv) {
    GET_PRIV_DATA();
    int ret = io_uring_queue_init(priv->max_events, &up->ring, IORING_SETUP_IOPOLL);
    if (ret == 0) {
        up->cqe = memhook._calloc(priv->max_events, sizeof(struct io_uring_cqe *));
        M_ALLOC_ASSERT(up->cqe);
        up->inited = true;
        flush_reqs(priv);
    }
    return ret;
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
        return io_uring_peek_batch_cqe(&up->ring, up->cqe, priv->max_events);
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
        udata->ev = NULL;
        poll_set_new_evt(priv, udata, ADD);
    }
    return udata;
}

int poll_get_fd(const poll_priv_t *priv) {
    GET_PRIV_DATA();
    return up->ring.ring_fd;
}

int poll_clear(poll_priv_t *priv) {
    GET_PRIV_DATA();
    io_uring_queue_exit(&up->ring);
    memhook._free(up->cqe);
    up->cqe = NULL;
    up->inited = false;
    return 0;
}

int poll_destroy(poll_priv_t *priv) {
    GET_PRIV_DATA();
    if (up->inited) { 
        poll_clear(priv);
    }
    m_list_free(&up->req_list);
    memhook._free(up);
    return 0;
}
