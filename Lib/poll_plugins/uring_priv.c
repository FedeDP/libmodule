#include "poll_priv.h"
#include <liburing.h>
#include <sys/poll.h>
#include <sys/signalfd.h>
#include <sys/timerfd.h>
#include <sys/inotify.h>
#include <limits.h>

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

static void flush_reqs(poll_priv_t *priv);

/* Inotify related defines */
#define BUF_LEN (sizeof(struct inotify_event) + NAME_MAX + 1)

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
    MOD_ALLOC_ASSERT(up->req_list);
    return MOD_OK;
}

int poll_set_new_evt(poll_priv_t *priv, ev_src_t *tmp, const enum op_type flag) {
    GET_PRIV_DATA();
    
    int ret = 0;
    if (up->inited) {
        /* Eventually request uring sqe if needed */
        if (!tmp->ev) { // we can safely use fd_src.ev here as "ev" is first struct field.
            if (flag == ADD) {
                tmp->ev = io_uring_get_sqe(&up->ring);
                MOD_ALLOC_ASSERT(tmp->ev);
            } else {
                /* We need to RM an unregistered ev. Fine. */
                return MOD_OK;
            }
        }
        
        struct io_uring_sqe *sqe = (struct io_uring_sqe *)tmp->ev;
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
                timerfd_settime(fd, 0, &timerValue, NULL);
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

        if (fd != -1) {
            if (flag == ADD) {
                io_uring_prep_poll_add(sqe, fd, POLLIN);
                /* properly set userdata */
                io_uring_sqe_set_data(sqe, tmp);
            } else {
                io_uring_prep_poll_remove(sqe, tmp);
                /* Eventually release uring sqe if needed */
                tmp->ev = NULL;
                
                if (tmp->type == TYPE_SGN || tmp->type == TYPE_TMR || tmp->type == TYPE_PT) {
                    close(fd); // automatically close internally used FDs
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
            list_insert(up->req_list, tmp, NULL);
        } else {
            list_remove(up->req_list, tmp, NULL);
        }
    }
    return ret;
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
        udata->ev = NULL;
        poll_set_new_evt(priv, udata, ADD);
    }
    return udata;
}

mod_ret poll_consume_sgn(poll_priv_t *priv, ev_src_t *src, sgn_msg_t *sgn_msg) {
    struct signalfd_siginfo fdsi;
    ssize_t s = read(src->sgn_src.f.fd, &fdsi, sizeof(struct signalfd_siginfo));
    if (s == sizeof(struct signalfd_siginfo)) {
        return MOD_OK;
    }
    return MOD_ERR;
}

mod_ret poll_consume_tmr(poll_priv_t *priv, ev_src_t *src, tm_msg_t *tm_msg) {
    uint64_t t;
    if (read(src->tm_src.f.fd, &t, sizeof(uint64_t)) == sizeof(uint64_t)) {
        return MOD_OK;
    }
    return MOD_ERR;
}

mod_ret poll_consume_pt(poll_priv_t *priv, ev_src_t *src, pt_msg_t *pt_msg) {
    char buffer[BUF_LEN];
    const size_t length = read(src->pt_src.f.fd, buffer, BUF_LEN);
    if (length > 0) {
        struct inotify_event *event = (struct inotify_event *) buffer;
        if (event->len) {
            pt_msg->type = event->mask;
            return MOD_OK;
        }
    }
    return MOD_ERR;
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
