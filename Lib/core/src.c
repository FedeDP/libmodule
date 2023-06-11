#include "public/module/thpool/thpool.h"
#include "poll.h"
#include "evts.h"

/**********************************
 * Code related to event sources. *
 **********************************/

#define M_TASK_MAX_THREADS    16

static void src_priv_dtor(void *data);
static void *task_thread(void *data);
static ev_src_t *create_src(m_mod_t *mod, m_src_types type, process_cb proc,
                            const void *src_data, m_src_flags flags, const void *userptr);

/* Compare functions */
static int fdcmp(void *my_data, void *node_data);
static int tmrcmp(void *my_data, void *node_data);
static int sgncmp(void *my_data, void *node_data);
static int pathcmp(void *my_data, void *node_data);
static int pidcmp(void *my_data, void *node_data);
static int taskcmp(void *my_data, void *node_data);
static int threshcmp(void *my_data, void *node_data);

/* Process functions */
static ev_src_t *process_ps(ev_src_t *this, m_ctx_t *c, int idx, evt_priv_t *evt);
static ev_src_t *process_fd(ev_src_t *this, m_ctx_t *c, int idx, evt_priv_t *evt);
static ev_src_t *process_tmr(ev_src_t *this, m_ctx_t *c, int idx, evt_priv_t *evt);
static ev_src_t *process_sgn(ev_src_t *this, m_ctx_t *c, int idx, evt_priv_t *evt);
static ev_src_t *process_path(ev_src_t *this, m_ctx_t *c, int idx, evt_priv_t *evt);
static ev_src_t *process_pid(ev_src_t *this, m_ctx_t *c, int idx, evt_priv_t *evt);
static ev_src_t *process_task(ev_src_t *this, m_ctx_t *c, int idx, evt_priv_t *evt);
static ev_src_t *process_thresh(ev_src_t *this, m_ctx_t *c, int idx, evt_priv_t *evt);

static m_bst_cmp src_cmp_map[] = {
        fdcmp,      // M_SRC_TYPE_PS we use internal pipe fd used for pubsub as module PS source
        fdcmp,      // M_SRC_TYPE_FD
        tmrcmp,     // M_SRC_TYPE_TMR
        sgncmp,     // M_SRC_TYPE_SGN
        pathcmp,    // M_SRC_TYPE_PATH
        pidcmp,     // M_SRC_TYPE_PID
        taskcmp,    // M_SRC_TYPE_TASK
        threshcmp,  // M_SRC_TYPE_THRESH
};
_Static_assert(sizeof(src_cmp_map) / sizeof(*src_cmp_map) == M_SRC_TYPE_END, "Undefined source compare function.");

const char *src_names[] = {
        "Priv",     // M_SRC_TYPE_PS is not exposed externally
        "Fds",      // M_SRC_TYPE_FD
        "Tmrs",     // M_SRC_TYPE_TMR
        "Sgns",     // M_SRC_TYPE_SGN
        "Paths",    // M_SRC_TYPE_PATH
        "Pids",     // M_SRC_TYPE_PID
        "Tasks",    // M_SRC_TYPE_TASK
        "Thresh",   // M_SRC_TYPE_THRESH
};
_Static_assert(sizeof(src_names) / sizeof(*src_names) == M_SRC_TYPE_END, "Undefined source name.");

static process_cb src_procs_map[] = {
    process_ps,      // M_SRC_TYPE_PS we use internal pipe fd used for pubsub as module PS source
    process_fd,      // M_SRC_TYPE_FD
    process_tmr,     // M_SRC_TYPE_TMR
    process_sgn,     // M_SRC_TYPE_SGN
    process_path,    // M_SRC_TYPE_PATH
    process_pid,     // M_SRC_TYPE_PID
    process_task,    // M_SRC_TYPE_TASK
    process_thresh,  // M_SRC_TYPE_THRESH
};
_Static_assert(sizeof(src_procs_map) / sizeof(*src_procs_map) == M_SRC_TYPE_END, "Undefined source processor function.");

static void src_priv_dtor(void *data) {
    ev_src_t *t = (ev_src_t *)data;

    /* If a fd is deregistered for a RUNNING module, stop polling on it */
    if (m_mod_is(t->mod, M_MOD_RUNNING)) {
        M_MOD_CTX(t->mod);
        poll_set_new_evt(&c->ppriv, t, RM);
    }

    /* Properly manage autoclose flag */
    if (t->flags & M_SRC_FD_AUTOCLOSE) {
        int fd = -1;
        switch (t->type) {
            case M_SRC_TYPE_PS: // M_SRC_TYPE_PS is used for pubsub_fd[0] in init_pubsub_fd()
            case M_SRC_TYPE_FD:
                fd = t->fd_src.fd;
                break;
            default:
                break;
        }
        if (fd != -1) {
            close(fd);
        }
    }

    if (t->flags & M_SRC_DUP) {
        switch (t->type) {
            case M_SRC_TYPE_PATH:
                memhook._free((void *)t->path_src.pt.path);
                break;
            default:
                break;
        }
    }

    if (t->flags & M_SRC_AUTOFREE) {
        memhook._free((void *)t->userptr);
    }
}

static void *task_thread(void *data) {
    ev_src_t *src = (ev_src_t *)data;
    M_MOD_CTX(src->mod);
    src->task_src.retval = src->task_src.tid.fn((void *)src->userptr);
    poll_notify_userevent(&c->ppriv, src);
    return NULL;
}

static ev_src_t *create_src(m_mod_t *mod, m_src_types type, process_cb proc,
                            const void *src_data, m_src_flags flags, const void *userptr) {
    ev_src_t *src = m_mem_new(sizeof(ev_src_t), src_priv_dtor);
    if (!src) {
        return NULL;
    }
    
    M_ASSERT(proc);
    M_ASSERT(type < M_SRC_TYPE_END);
    
    src->flags = flags;
    src->userptr = userptr;
    src->type = type;
    src->mod = mod;
    src->process = proc;
    
    /*
     * Same storage is used for all linux's internal fds, eg: for timerfd, signalfd...
     * as fd_src_t is always first struct field on linux.
     */
    src->fd_src.fd = -1;
    
    switch (type) {
        case M_SRC_TYPE_PS: // M_SRC_TYPE_PS is used for pubsub_fd[0] in init_pubsub_fd()
        case M_SRC_TYPE_FD: {
            fd_src_t *fd_src = &src->fd_src;
            int fd = *((int *)src_data);
            if (flags & M_SRC_DUP) {
                fd_src->fd = dup(fd);
                src->flags |= M_SRC_FD_AUTOCLOSE;
            } else {
                fd_src->fd = fd;
            }
            
            // enforce HIGH priority for fds
            if (type == M_SRC_TYPE_FD) {
                src->flags |= M_SRC_PRIO_HIGH;
            }
            break;
        }
        case M_SRC_TYPE_TMR: {
            tmr_src_t *tm_src = &src->tmr_src;
            memcpy(&tm_src->its, src_data, sizeof(m_src_tmr_t));
            break;
        }
        case M_SRC_TYPE_SGN: {
            sgn_src_t *sgn_src = &src->sgn_src;
            memcpy(&sgn_src->sgs, src_data, sizeof(m_src_sgn_t));
            break;
        }
        case M_SRC_TYPE_PATH: {
            path_src_t *pt_src = &src->path_src;
            memcpy(&pt_src->pt, src_data, sizeof(m_src_path_t));
            if (flags & M_SRC_DUP) {
                pt_src->pt.path = mem_strdup(pt_src->pt.path);
            }
            break;
        }
        case M_SRC_TYPE_PID: {
            pid_src_t *pid_src = &src->pid_src;
            memcpy(&pid_src->pid, src_data, sizeof(m_src_pid_t));
            break;
        }
        case M_SRC_TYPE_TASK: {
            task_src_t *task_src = &src->task_src;
            memcpy(&task_src->tid, src_data, sizeof(m_src_task_t));
            src->flags |= M_SRC_ONESHOT;  // force ONESHOT flag
            break;
        }
        case M_SRC_TYPE_THRESH: {
            thresh_src_t *thresh_src = &src->thresh_src;
            memcpy(&thresh_src->thr, src_data, sizeof(m_src_thresh_t));
            src->flags |= M_SRC_ONESHOT;  // force ONESHOT flag
            break;
        }
        default:
            M_WARN("Wrong src type: %d\n", type);
            m_mem_unrefp((void **)&src);
            break;
    }
    return src;
}

static int fdcmp(void *my_data, void *node_data) {
    const ev_src_t *node = (const ev_src_t *)node_data;
    const ev_src_t *other = (const ev_src_t *)my_data;

    return other->fd_src.fd - node->fd_src.fd;
}

static int tmrcmp(void *my_data, void *node_data) {
    const ev_src_t *node = (const ev_src_t *)node_data;
    const ev_src_t *other = (const ev_src_t *)my_data;

    return other->tmr_src.its.ns - node->tmr_src.its.ns;
}

static int sgncmp(void *my_data, void *node_data) {
    const ev_src_t *node = (const ev_src_t *)node_data;
    const ev_src_t *other = (const ev_src_t *)my_data;

    return other->sgn_src.sgs.signo - node->sgn_src.sgs.signo;
}

static int pathcmp(void *my_data, void *node_data) {
    const ev_src_t *node = (const ev_src_t *)node_data;
    const ev_src_t *other = (const ev_src_t *)my_data;

    return strcmp(other->path_src.pt.path, node->path_src.pt.path);
}

static int pidcmp(void *my_data, void *node_data) {
    const ev_src_t *node = (const ev_src_t *)node_data;
    const ev_src_t *other = (const ev_src_t *)my_data;

    return other->pid_src.pid.pid - node->pid_src.pid.pid;
}

static int taskcmp(void *my_data, void *node_data) {
    const ev_src_t *node = (const ev_src_t *)node_data;
    const ev_src_t *other = (const ev_src_t *)my_data;

    return other->task_src.tid.tid - node->task_src.tid.tid;
}

static int threshcmp(void *my_data, void *node_data) {
    const ev_src_t *node = (const ev_src_t *)node_data;
    const ev_src_t *other = (const ev_src_t *)my_data;

    long double other_val = (long double)other->thresh_src.thr.activity_freq
                            + (long double)other->thresh_src.thr.inactive_ms;
    long double node_val = (long double)node->thresh_src.thr.activity_freq
                            + (long double)node->thresh_src.thr.inactive_ms;
    return other_val - node_val;
}

static ev_src_t *process_ps(ev_src_t *this, m_ctx_t *c, int idx, evt_priv_t *evt) {
    ps_priv_t *ps_msg;
    /* Received on pubsub interface */
    if (read(this->fd_src.fd, (void **)&ps_msg, sizeof(ps_priv_t *)) != sizeof(ps_priv_t *)) {
        M_ERR("Failed to read message: %s\n", strerror(errno));
    } else {
        /*
         * Use real event source, ie: topic subscription, being careful to unref current src;
         * Note: it can be NULL when the ps message was created by a direct tell() or broadcast()
         */
        evt->evt.ps_evt = &ps_msg->msg;
        m_mem_unref(this);
        this = m_mem_ref(ps_msg->sub);
        evt->src = this;
    }
    return this;
}

static ev_src_t *process_fd(ev_src_t *this, m_ctx_t *c, int idx, evt_priv_t *evt) {
    evt->evt.fd_evt = m_mem_new(sizeof(*evt->evt.fd_evt), NULL);
    evt->evt.fd_evt->fd = this->fd_src.fd;
    return this;
}

static ev_src_t *process_tmr(ev_src_t *this, m_ctx_t *c, int idx, evt_priv_t *evt) {
    evt->evt.tmr_evt = m_mem_new(sizeof(*evt->evt.tmr_evt), NULL);
    if (poll_consume_tmr(&c->ppriv, idx, this, evt->evt.tmr_evt) == 0) {
        evt->evt.tmr_evt->ns = this->tmr_src.its.ns;
    }
    return this;
}

static ev_src_t *process_sgn(ev_src_t *this, m_ctx_t *c, int idx, evt_priv_t *evt) {
    evt->evt.sgn_evt = m_mem_new(sizeof(*evt->evt.sgn_evt), NULL);
    if (poll_consume_sgn(&c->ppriv, idx, this, evt->evt.sgn_evt) == 0) {
        evt->evt.sgn_evt->signo = this->sgn_src.sgs.signo;
    }
    return this;
}

static ev_src_t *process_path(ev_src_t *this, m_ctx_t *c, int idx, evt_priv_t *evt) {
    evt->evt.path_evt = m_mem_new(sizeof(*evt->evt.path_evt), NULL);
    if (poll_consume_pt(&c->ppriv, idx, this, evt->evt.path_evt) == 0) {
        evt->evt.path_evt->path = this->path_src.pt.path;
    }
    return this;
}

static ev_src_t *process_pid(ev_src_t *this, m_ctx_t *c, int idx, evt_priv_t *evt) {
    evt->evt.pid_evt = m_mem_new(sizeof(*evt->evt.pid_evt), NULL);
    if (poll_consume_pid(&c->ppriv, idx, this, evt->evt.pid_evt) == 0) {
        evt->evt.pid_evt->pid = this->pid_src.pid.pid;
    }
    return this;
}

static ev_src_t *process_task(ev_src_t *this, m_ctx_t *c, int idx, evt_priv_t *evt) {
    evt->evt.task_evt = m_mem_new(sizeof(*evt->evt.task_evt), NULL);
    if (poll_consume_task(&c->ppriv, idx, this, evt->evt.task_evt) == 0) {
        evt->evt.task_evt->tid = this->task_src.tid.tid;
    }
    return this;
}

static ev_src_t *process_thresh(ev_src_t *this, m_ctx_t *c, int idx, evt_priv_t *evt) {
    evt->evt.thresh_evt = m_mem_new(sizeof(*evt->evt.thresh_evt), NULL);
    if (poll_consume_thresh(&c->ppriv, idx, this, evt->evt.thresh_evt) == 0) {
        evt->evt.thresh_evt->inactive_ms = this->thresh_src.alarm.inactive_ms;
        evt->evt.thresh_evt->activity_freq = this->thresh_src.alarm.activity_freq;
    }
    return this;
}

/** Private API **/

int init_src(m_mod_t *mod, m_src_types t) {
    mod->srcs[t] = m_bst_new(src_cmp_map[t], mem_dtor);
    if (!mod->srcs[t]) {
        return -ENOMEM;
    }
    return 0;
}

ev_src_t *register_ctx_src(m_ctx_t *c, m_src_types type, process_cb proc,
                     const void *src_data) {
    M_ASSERT(type < M_SRC_TYPE_END);
    
    ev_src_t *src = create_src(NULL, type, proc, src_data, 0, NULL);
    if (!src) {
        return NULL;
    }
    
    src->flags = M_SRC_PRIO_HIGH | M_SRC_INTERNAL;
    /* If a src is registered at runtime, start receiving its events immediately */
    if (c->state == M_CTX_LOOPING) {
        poll_set_new_evt(&c->ppriv, src, ADD);
    }
    return src;
}

int deregister_ctx_src(m_ctx_t *c, ev_src_t **src) {
    if (src && *src) {
        poll_set_new_evt(&c->ppriv, *src, RM);
        m_mem_unrefp((void **)src);
    }
    return 0;
}

int register_mod_src(m_mod_t *mod, m_src_types type, const void *src_data,
                         m_src_flags flags, const void *userptr) {
    M_MOD_ASSERT(mod);
    M_MOD_CONSUME_TOKEN(mod);
    M_SRC_ASSERT_PRIO_FLAGS();
    
    M_ASSERT(type < M_SRC_TYPE_END);
    
    ev_src_t *src = create_src(mod, type, src_procs_map[type], src_data, flags, userptr);
    if (!src) {
        return -EINVAL;
    }
    int ret = m_bst_insert(mod->srcs[type], src);
    if (ret == 0) {
        /* If a src is registered at runtime, start receiving its events immediately */
        if (m_mod_is(mod, M_MOD_RUNNING)) {
            M_MOD_CTX(mod);
            ret = poll_set_new_evt(&c->ppriv, src, ADD);

            /* For type task: create task thread */
            if (ret == 0 && src->type == M_SRC_TYPE_TASK) {
                ret = start_task(c, src);
            }
        }
        return !ret ? 0 : -errno;
    }
    m_mem_unref(src);
    return ret;
}

int deregister_mod_src(m_mod_t *mod, m_src_types type, void *src_data) {
    M_MOD_ASSERT(mod);
    M_MOD_CONSUME_TOKEN(mod);

    // Create onetime src to check the bst
    ev_src_t *src = create_src(mod, type, src_procs_map[type], src_data, 0, NULL);
    if (!src) {
        return -EINVAL;
    }
    int ret = m_bst_remove(mod->srcs[type], src);
    m_mem_unref(src);
    return ret;
}

int start_task(m_ctx_t *c, ev_src_t *src) {
    if (!c->thpool) {
        c->thpool = m_thpool_new(M_TASK_MAX_THREADS, M_THPOOL_LAZY);
    }
    M_ALLOC_ASSERT(c->thpool);
    return m_thpool_add(c->thpool, task_thread, src);
}

/** Public API **/

_public_ int m_mod_src_register_fd(m_mod_t *mod, int fd, m_src_flags flags, const void *userptr) {
    M_PARAM_ASSERT(fd >= 0);
    
    // Only M_SRC_PRIO_HIGH is available for fd sources. Later enforced by create_src().
    const m_src_flags prio_flags = flags & M_SRC_PRIO_MASK;
    M_PARAM_ASSERT(prio_flags == 0 || prio_flags == M_SRC_PRIO_HIGH);

    return register_mod_src(mod, M_SRC_TYPE_FD, &fd, flags, userptr);
}

_public_ int m_mod_src_deregister_fd(m_mod_t *mod, int fd) {
    M_PARAM_ASSERT(fd >= 0);

    return deregister_mod_src(mod, M_SRC_TYPE_FD, (void *)&fd);
}

_public_ int m_mod_src_register_tmr(m_mod_t *mod, const m_src_tmr_t *its, m_src_flags flags, const void *userptr) {
    M_PARAM_ASSERT(its && its->ns > 0);

    return register_mod_src(mod, M_SRC_TYPE_TMR, its, flags, userptr);
}

_public_ int m_mod_src_deregister_tmr(m_mod_t *mod, const m_src_tmr_t *its) {
    M_PARAM_ASSERT(its && its->ns > 0);

    return deregister_mod_src(mod, M_SRC_TYPE_TMR, (void *)its);
}

_public_ int m_mod_src_register_sgn(m_mod_t *mod, const m_src_sgn_t *sgs, m_src_flags flags, const void *userptr) {
    M_PARAM_ASSERT(sgs && sgs->signo > 0);

    return register_mod_src(mod, M_SRC_TYPE_SGN, sgs, flags, userptr);
}

_public_ int m_mod_src_deregister_sgn(m_mod_t *mod, const m_src_sgn_t *sgs) {
    M_PARAM_ASSERT(sgs && sgs->signo > 0);

    return deregister_mod_src(mod, M_SRC_TYPE_SGN, (void *)sgs);
}

_public_ int m_mod_src_register_path(m_mod_t *mod, const m_src_path_t *pt, m_src_flags flags, const void *userptr) {
    M_PARAM_ASSERT(pt);
    M_PARAM_ASSERT(str_not_empty(pt->path));
    M_PARAM_ASSERT(pt->events > 0);

    return register_mod_src(mod, M_SRC_TYPE_PATH, pt, flags, userptr);
}

_public_ int m_mod_src_deregister_path(m_mod_t *mod, const m_src_path_t *pt) {
    M_PARAM_ASSERT(pt);
    M_PARAM_ASSERT(str_not_empty(pt->path));

    return deregister_mod_src(mod, M_SRC_TYPE_PATH, (void *)pt);
}

_public_ int m_mod_src_register_pid(m_mod_t *mod, const m_src_pid_t *pid, m_src_flags flags, const void *userptr) {
    M_PARAM_ASSERT(pid && pid->pid > 0);

    return register_mod_src(mod, M_SRC_TYPE_PID, pid, flags, userptr);
}

_public_ int m_mod_src_deregister_pid(m_mod_t *mod, const m_src_pid_t *pid) {
    M_PARAM_ASSERT(pid && pid->pid > 0);

    return deregister_mod_src(mod, M_SRC_TYPE_PID, (void *)pid);
}

_public_ int m_mod_src_register_task(m_mod_t *mod, const m_src_task_t *tid, m_src_flags flags, const void *userptr) {
    M_PARAM_ASSERT(tid && tid->fn);

    return register_mod_src(mod, M_SRC_TYPE_TASK, tid, flags, userptr);
}

_public_ int m_mod_src_deregister_task(m_mod_t *mod, const m_src_task_t *tid) {
    M_PARAM_ASSERT(tid);

    /* Tasks cannot be deregistered */
    return -EPERM;
}

_public_ int m_mod_src_register_thresh(m_mod_t *mod, const m_src_thresh_t *thr, m_src_flags flags, const void *userptr) {
    M_PARAM_ASSERT(thr && (thr->activity_freq > 0 || thr->inactive_ms > 0));

    return register_mod_src(mod, M_SRC_TYPE_THRESH, thr, flags, userptr);
}

_public_ int m_mod_src_deregister_thresh(m_mod_t *mod, const m_src_thresh_t *thr) {
    M_PARAM_ASSERT(thr && (thr->activity_freq > 0 || thr->inactive_ms > 0));

    return deregister_mod_src(mod, M_SRC_TYPE_THRESH, (void *)thr);
}

_public_ ssize_t m_mod_src_len(const m_mod_t *mod, m_src_types type) {
    M_MOD_ASSERT(mod);
    M_PARAM_ASSERT(type >= M_SRC_TYPE_PS && type <= M_SRC_TYPE_END);
    
    int len = 0;
    m_itr_foreach(mod->subscriptions, {
        ev_src_t *src = m_itr_get(m_itr);
        if (!(src->flags & M_SRC_INTERNAL)) {
            len++;
        }
    });
    
    for (int i = M_SRC_TYPE_FD; i < M_SRC_TYPE_END; i++) {
        m_itr_foreach(mod->srcs[i], {
            ev_src_t *src = m_itr_get(m_itr);
            if (!(src->flags & M_SRC_INTERNAL)) {
                len++;
            }
        });
    }
    return len;
}
