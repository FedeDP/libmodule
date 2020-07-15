#include "mod.h"
#include "ctx.h"
#include "poll_priv.h"

/** Generic module interface **/

static void module_dtor(void *data);
static int _pipe(mod_t *mod);
static int init_pubsub_fd(mod_t *mod);
static void src_priv_dtor(void *data);
static void *task_thread(void *data);
static int start_task(ev_src_t *src);

static int _register_src(mod_t *mod, const m_src_types type, const void *src_data, 
                             const m_src_flags flags, const void *userptr);
static int _deregister_src(mod_t *mod, const m_src_types type, void *src_data);

static int fdcmp(void *my_data, void *node_data);
static int tmrcmp(void *my_data, void *node_data);
static int sgncmp(void *my_data, void *node_data);
static int pathcmp(void *my_data, void *node_data);
static int pidcmp(void *my_data, void *node_data);
static int taskcmp(void *my_data, void *node_data);

static int manage_fds(mod_t *mod, ctx_t *c, const int flag, const bool stop);
static void reset_module(mod_t *mod);

extern int fs_cleanup(mod_t *mod);

static m_bst_cmp src_cmp_map[] = {
    fdcmp,      // M_SRC_TYPE_PS we use internal pipe fd used for pubsub as module PS source
    fdcmp,      // M_SRC_TYPE_FD
    tmrcmp,     // M_SRC_TYPE_TMR
    sgncmp,     // M_SRC_TYPE_SGN
    pathcmp,    // M_SRC_TYPE_PATH
    pidcmp,     // M_SRC_TYPE_PID
    taskcmp     // M_SRC_TYPE_TASK
};
_Static_assert(sizeof(src_cmp_map) / sizeof(*src_cmp_map) == M_SRC_TYPE_END, "Undefined source compare function.");
static const char *src_names[] = {
    "Priv",     // M_SRC_TYPE_PS is not exposed externally
    "Fds",      // M_SRC_TYPE_FD
    "Tmrs",     // M_SRC_TYPE_TMR
    "Sgns",     // M_SRC_TYPE_SGN
    "Paths",    // M_SRC_TYPE_PATH
    "Pids",     // M_SRC_TYPE_PID
    "Tasks"     // M_SRC_TYPE_TASK
};
_Static_assert(sizeof(src_names) / sizeof(*src_names) == M_SRC_TYPE_END, "Undefined source name.");

static void module_dtor(void *data) {
    mod_t *mod = (mod_t *)data;
    if (mod) {
        m_mem_unref(mod->ctx);
        
        m_map_free(&mod->subscriptions);
        m_stack_free(&mod->recvs);
        for (int i = 0; i < M_SRC_TYPE_END; i++) {
            m_bst_free(&mod->srcs[i]);
        }
        
        memhook._free((void *)mod->local_path);
        
        if (mod->flags & M_MOD_NAME_AUTOFREE) {
            memhook._free((void *)mod->name);
        }
        
        if (mod->flags & M_MOD_USERDATA_AUTOFREE) {
            memhook._free((void *)mod->userdata);
        }
    }
}

static int _pipe(mod_t *mod) {
    int ret = pipe(mod->pubsub_fd);
    if (ret == 0) {
        for (int i = 0; i < 2; i++) {
            int flags = fcntl(mod->pubsub_fd[i], F_GETFL, 0);
            if (flags == -1) {
                flags = 0;
            }
            fcntl(mod->pubsub_fd[i], F_SETFL, flags | O_NONBLOCK);
            fcntl(mod->pubsub_fd[i], F_SETFD, FD_CLOEXEC);
        }
    }
    return ret;
}

static int init_pubsub_fd(mod_t *mod) {
    if (_pipe(mod) == 0) {
        fd_src_t fd_src = {0};
        fd_src.fd = mod->pubsub_fd[0];
        if (_register_src(mod, M_SRC_TYPE_PS, &fd_src, M_SRC_FD_AUTOCLOSE, NULL) == 0) {
            return 0;
        }
        close(mod->pubsub_fd[0]);
        close(mod->pubsub_fd[1]);
        mod->pubsub_fd[0] = -1;
        mod->pubsub_fd[1] = -1;
    }
    return -errno;
}

static void src_priv_dtor(void *data) {
    ev_src_t *t = (ev_src_t *)data;
    M_MOD_CTX(t->mod);
    
    /* If a fd is deregistered for a RUNNING module, stop polling on it */
    if (m_mod_is(t->mod, RUNNING)) {
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
    
    memhook._free(t);
}

static void *task_thread(void *data) {
    ev_src_t *src = (ev_src_t *)data;    
    src->task_src.retval = src->task_src.tid.fn((void *)src->userptr);
    poll_notify_task(src);
    pthread_exit(NULL);
}

static int start_task(ev_src_t *src) {
    if (src->task_src.tid.thpool) {
        return m_thpool_add(src->task_src.tid.thpool, task_thread, src);
    }
    
    /* Create detached */
    pthread_attr_t tattr;
    pthread_attr_init(&tattr);
    pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
    int ret = pthread_create(&src->task_src.th, &tattr, task_thread, src);
    pthread_attr_destroy(&tattr);
    return ret;
}

static int _register_src(mod_t *mod, const m_src_types type, const void *src_data, 
                             const m_src_flags flags, const void *userptr) {
    M_MOD_ASSERT(mod);
    ev_src_t *src = memhook._calloc(1, sizeof(ev_src_t));
    M_ALLOC_ASSERT(src);
    
    src->flags = flags;
    src->userptr = userptr;
    src->type = type;
    src->mod = mod;
    
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
        break;
    }
    case M_SRC_TYPE_TMR: {
        tmr_src_t *tm_src = &src->tmr_src;
        memcpy(&tm_src->its, src_data, sizeof(mod_tmr_t));
        break;
    }
    case M_SRC_TYPE_SGN: {
        sgn_src_t *sgn_src = &src->sgn_src;
        memcpy(&sgn_src->sgs, src_data, sizeof(mod_sgn_t));
        break;
    }
    case M_SRC_TYPE_PATH: {
        path_src_t *pt_src = &src->path_src;
        memcpy(&pt_src->pt, src_data, sizeof(mod_path_t));
        if (flags & M_SRC_DUP) {
            pt_src->pt.path = mem_strdup(pt_src->pt.path);
        }
        break;
    }
    case M_SRC_TYPE_PID: {
        pid_src_t *pid_src = &src->pid_src;
        memcpy(&pid_src->pid, src_data, sizeof(mod_pid_t));
        break;
    }
    case M_SRC_TYPE_TASK: {
        task_src_t *task_src = &src->task_src;
        memcpy(&task_src->tid, src_data, sizeof(mod_task_t));
        break;
    }
    default:
        M_DEBUG("Wrong src type: %d\n", type);
        memhook._free(src);
        return -EINVAL;
    }
    
    int ret = m_bst_insert(mod->srcs[type], src);
    if (ret == 0) {
        fetch_ms(&mod->stats.last_seen, &mod->stats.action_ctr);
        /* If a src is registered at runtime, start receiving its events */
        if (m_mod_is(mod, RUNNING)) {
            M_MOD_CTX(mod);
            ret = poll_set_new_evt(&c->ppriv, src, ADD);
            
            /* For type task: create task thread */
            if (ret == 0 && src->type == M_SRC_TYPE_TASK) {
                ret = start_task(src);
            }
        }
        return !ret ? 0 : -errno;
    }
    memhook._free(src);
    return ret;
}

static int _deregister_src(mod_t *mod, const m_src_types type, void *src_data) {   
    M_MOD_ASSERT(mod);
    
    int ret = m_bst_remove(mod->srcs[type], src_data);
    if (ret == 0) {
        fetch_ms(&mod->stats.last_seen, &mod->stats.action_ctr);
    }
    return ret;
}

static int fdcmp(void *my_data, void *node_data) {
    ev_src_t *src = (ev_src_t *)node_data;
    int fd = *((int *)my_data);
    
    return fd - src->fd_src.fd;
}

static int tmrcmp(void *my_data, void *node_data) {
    ev_src_t *src = (ev_src_t *)node_data;
    const mod_tmr_t *its = (const mod_tmr_t *)my_data;
    
    return its->ms - src->tmr_src.its.ms;
}

static int sgncmp(void *my_data, void *node_data) {
    ev_src_t *src = (ev_src_t *)node_data;
    const mod_sgn_t *sgs = (const mod_sgn_t *)my_data;
    
    return sgs->signo - src->sgn_src.sgs.signo;
}

static int pathcmp(void *my_data, void *node_data) {
    ev_src_t *src = (ev_src_t *)node_data;
    const mod_path_t *pt = (const mod_path_t *)my_data;
    
    return strcmp(pt->path, src->path_src.pt.path);
}

static int pidcmp(void *my_data, void *node_data) {
    ev_src_t *src = (ev_src_t *)node_data;
    const mod_pid_t *pid = (const mod_pid_t *)my_data;
    
    return pid->pid - src->pid_src.pid.pid;
}

static int taskcmp(void *my_data, void *node_data) {
    ev_src_t *src = (ev_src_t *)node_data;
    const mod_task_t *tid = (const mod_task_t *)my_data;
    
    return tid->tid - src->task_src.tid.tid;;
}

static int manage_fds(mod_t *mod, ctx_t *c, const int flag, const bool stop) {    
    int ret = 0;
    
    fetch_ms(&mod->stats.last_seen, &mod->stats.action_ctr);
    for (int i = 0; i < M_SRC_TYPE_END; i++) {
        m_itr_foreach(mod->srcs[i], {
            ev_src_t *t = m_itr_get(itr);
            if (flag == RM && stop) {
                if (t->type == M_SRC_TYPE_PS) {
                    /*
                    * Free all unread pubsub msg for this module.
                    *
                    * Pass a !NULL pointer as first parameter,
                    * so that flush_pubsub_msgs() will free messages instead of delivering them.
                    */
                    flush_pubsub_msgs(mod, NULL, mod);
                }
                ret = m_itr_rm(itr);
            } else {
                ret = poll_set_new_evt(&c->ppriv, t, flag);
                
                /* For type task: create task thread now */
                if (ret == 0 && t->type == M_SRC_TYPE_TASK && flag == ADD) {
                    ret = start_task(t);
                }
            }
        });
    }
    return ret;
}

static void reset_module(mod_t *mod) {
    if (mod->pubsub_fd[1] != -1) {
        close(mod->pubsub_fd[1]);
        mod->pubsub_fd[0] = -1;
        mod->pubsub_fd[1] = -1;
    }
    m_map_clear(mod->subscriptions);
    m_stack_clear(mod->recvs);
}

/** Private API **/

int evaluate_module(void *data, const char *key, void *value) {
    mod_t *mod = (mod_t *)value;
    if (m_mod_is(mod, IDLE) && 
        (!mod->hook.eval || mod->hook.eval())) {
        
        start(mod, true);
    }
    return 0;
}

int start(mod_t *mod, const bool starting) {
    static const char *errors[] = { "Failed to resume module.", "Failed to start module." };
    
    M_MOD_CTX(mod);

    /* 
     * Starting module for the first time
     * or after it was stopped.
     * Properly add back its pubsub fds.
     */
    if (starting) {
        /* THIS IS NOT A RESUME */
        int ret = init_pubsub_fd(mod);
        if (ret != 0) {
            return ret;
        }
    }
    
    int ret = manage_fds(mod, c, ADD, false);
    M_ASSERT(!ret, errors[starting], ret);
    
    mod->state = RUNNING;
    
    /* Call module init() callback only if module is being (re)started */
    if (!starting || mod->hook.init()) {
        M_DEBUG("%s '%s'.\n", starting ? "Started" : "Resumed", mod->name);
        tell_system_pubsub_msg(NULL, c, MODULE_STARTED, mod, NULL);
        return 0;
    }
    
    /* If init() returns false, we need to stop this module right away */
    stop(mod, true);
    return 0;
}

int stop(mod_t *mod, const bool stopping) {
    static const char *errors[] = { "Failed to pause module.", "Failed to stop module." };
    
    M_MOD_CTX(mod);
        
    int ret = manage_fds(mod, c, RM, stopping);
    M_ASSERT(!ret, errors[stopping], ret);
    
    mod->state = stopping ? STOPPED : PAUSED;
    
    /*
     * When module gets stopped, its write-end pubsub fd is closed too 
     * Read-end is already closed by manage_fds().
     * Moreover, its subscriptions are cleared.
     * 
     * Finally, deinit() callback is called.
     */
    if (stopping) {
        reset_module(mod);
        if (mod->hook.deinit) {
            mod->hook.deinit();
        }
    }
    
    M_DEBUG("%s '%s'.\n", stopping ? "Stopped" : "Paused", mod->name);
    
    tell_system_pubsub_msg(NULL, c, MODULE_STOPPED, mod, NULL);
    return 0;
}

/** Public API **/

int m_mod_register(const char *name, ctx_t *c, mod_t **self, const userhook_t *hook, 
                   const m_mod_flags flags, const void *userdata) {
    M_PARAM_ASSERT(name);
    M_PARAM_ASSERT(self);
    M_PARAM_ASSERT(!*self);
    M_PARAM_ASSERT(hook);
    /* Mandatory callbacks */
    M_PARAM_ASSERT(hook->init);
    M_PARAM_ASSERT(hook->recv);

    /* Use default context, if NULL is passed, eventually creating it. */
    if (!c) {
        c = check_ctx(M_CTX_DEFAULT);
        if (!c) {
            m_ctx_register(M_CTX_DEFAULT, &c, 0, NULL);
        }
    }
    M_PARAM_ASSERT(c);
    
    int ret;
    mod_t *old_mod = m_map_get(c->modules, name);
    if (old_mod) {
        if (!(old_mod->flags & M_MOD_ALLOW_REPLACE)) { 
            M_DEBUG("Module with same name already registered in context.");
            return -EEXIST;
        }
        ret = m_mod_deregister(&old_mod);
        if (ret != 0) {
            return ret;
        }
    }
    
    M_DEBUG("Registering module '%s'.\n", name);
    
    mod_t *mod = m_mem_new(sizeof(mod_t), module_dtor);
    M_ALLOC_ASSERT(mod);
    
    mod->ctx = m_mem_ref(c);
    
    mod->flags = flags;
    if (flags & M_MOD_NAME_DUP) {
        mod->flags |= M_MOD_NAME_AUTOFREE;
    }
    mod->userdata = userdata;
    
    ret = -ENOMEM;
    /* Let us gladly jump out with break on error */
    do {
        mod->name = flags & M_MOD_NAME_DUP ? mem_strdup(name) : name;
        
        for (int i = 0; i < M_SRC_TYPE_END; i++) {
            mod->srcs[i] = m_bst_new(src_cmp_map[i], src_priv_dtor);
            if (!mod->srcs[i]) {
                break;
            }
        }
        
        mod->recvs = m_stack_new(NULL);
        if (!mod->recvs) {
            break;
        }
        
        if (m_map_put(c->modules, mod->name, mod) == 0) {
            memcpy(&mod->hook, hook, sizeof(userhook_t));
            mod->state = IDLE;
            
            *self = mod;
            
            mod->pubsub_fd[0] = -1;
            mod->pubsub_fd[1] = -1;
            
            fetch_ms(&mod->stats.registration_time, NULL);
            return 0;
        }
    } while (false);
    
    m_mem_unref(mod);
    return ret;
}

int m_mod_deregister(mod_t **mod) {
    M_PARAM_ASSERT(mod);
    M_MOD_ASSERT((*mod));
    M_MOD_CTX((*mod));
    
    mod_t *m = *mod;
    if ((m->flags & M_MOD_PERSIST) && c->looping) {
        return -EPERM;
    }
    
    M_DEBUG("Deregistering module '%s'.\n", m->name);
    
    /* Keep module alive untile destroy() callback is called */
    M_MEM_LOCK(m, {
        /* Stop module */
        stop(m, true);
        m->state = ZOMBIE;
        
        /* Remove the module from the context */
        m_map_remove(c->modules, m->name);
        
        /* Free FS internal data */
        fs_cleanup(m);
        
        /*
        * Call destroy once self is NULL and module has been removed from context;
        * ie: no more libmodule operations can be called on this handler 
        * neither it can be ref'd through module_ref.
        */
        *mod = NULL;
    });
    
    /* Destroy context if needed */
    if (m_map_length(c->modules) == 0 && !(c->flags & M_CTX_PERSIST)) {
        return m_ctx_deregister(&c);
    }
    return 0;
}

ctx_t *m_mod_ctx(const mod_t *mod) {
    M_RET_ASSERT(mod, NULL);
    M_RET_ASSERT(mod->ctx->th_id == pthread_self(), NULL);
    M_RET_ASSERT(!m_mod_is(mod, ZOMBIE), NULL);
    
    return mod->ctx;
}


int m_mod_become(mod_t *mod, const recv_cb new_recv) {
    M_PARAM_ASSERT(new_recv);
    M_MOD_ASSERT_STATE(mod, RUNNING);
    
    int ret = m_stack_push(mod->recvs, new_recv);
    if (ret == 0) {
        fetch_ms(&mod->stats.last_seen, &mod->stats.action_ctr);
    }
    return ret;
}

int m_mod_unbecome(mod_t *mod) {
    M_MOD_ASSERT_STATE(mod, RUNNING);
    
    if (m_stack_pop(mod->recvs) != NULL) {
        fetch_ms(&mod->stats.last_seen, &mod->stats.action_ctr);
        return 0;
    }
    return -EINVAL;
}

int m_mod_log(const mod_t *mod, const char *fmt, ...) {
    M_MOD_ASSERT(mod);
    M_MOD_CTX(mod);
    
    va_list args;
    va_start(args, fmt);
    c->logger(mod, fmt, args);
    va_end(args);
    return 0;
}

int m_mod_set_userdata(mod_t *mod, const void *userdata) {
    M_MOD_ASSERT(mod);
    
    mod->userdata = userdata;
    return 0;
}

const void *m_mod_get_userdata(const mod_t *mod) {
    M_RET_ASSERT(mod, NULL);
    M_RET_ASSERT(mod->ctx->th_id == pthread_self(), NULL);
    M_RET_ASSERT(!m_mod_is(mod, ZOMBIE), NULL);
    
    return mod->userdata;
}

int m_mod_register_fd(mod_t *mod, const int fd, const m_src_flags flags, const void *userptr) {
    M_PARAM_ASSERT(fd >= 0);

    return _register_src(mod, M_SRC_TYPE_FD, &fd, flags, userptr);
}

int m_mod_deregister_fd(mod_t *mod, const int fd) {
    M_PARAM_ASSERT(fd >= 0);
    
    return _deregister_src(mod, M_SRC_TYPE_FD, (void *)&fd);
}

int m_mod_register_tmr(mod_t *mod, const mod_tmr_t *its, const m_src_flags flags, const void *userptr) {
    M_PARAM_ASSERT(its && its->ms > 0);
    
    return _register_src(mod, M_SRC_TYPE_TMR, its, flags, userptr);
}

int m_mod_deregister_tmr(mod_t *mod, const mod_tmr_t *its) {
    M_PARAM_ASSERT(its && its->ms > 0);
    
    return _deregister_src(mod, M_SRC_TYPE_TMR, (void *)its);
}

int m_mod_register_sgn(mod_t *mod, const mod_sgn_t *sgs, const m_src_flags flags, const void *userptr) {
    M_PARAM_ASSERT(sgs && sgs->signo > 0);
    
    return _register_src(mod, M_SRC_TYPE_SGN, sgs, flags, userptr);
}

int m_mod_deregister_sgn(mod_t *mod, const mod_sgn_t *sgs) {
    M_PARAM_ASSERT(sgs && sgs->signo > 0);
    
    return _deregister_src(mod, M_SRC_TYPE_SGN, (void *)sgs);
}

int m_mod_register_path(mod_t *mod, const mod_path_t *pt, const m_src_flags flags, const void *userptr) {
    M_PARAM_ASSERT(pt);
    M_PARAM_ASSERT(pt->path && strlen(pt->path));
    M_PARAM_ASSERT(pt->events > 0);
    
    return _register_src(mod, M_SRC_TYPE_PATH, pt, flags, userptr);
}

int m_mod_deregister_path(mod_t *mod, const mod_path_t *pt) {
    M_PARAM_ASSERT(pt);
    M_PARAM_ASSERT(pt->path && strlen(pt->path));
    
    return _deregister_src(mod, M_SRC_TYPE_PATH, (void *)pt);
}

int m_mod_register_pid(mod_t *mod, const mod_pid_t *pid, const m_src_flags flags, const void *userptr) {
    M_PARAM_ASSERT(pid && pid->pid > 0);
    
    return _register_src(mod, M_SRC_TYPE_PID, pid, flags, userptr);
}

int m_mod_deregister_pid(mod_t *mod, const mod_pid_t *pid) {
    M_PARAM_ASSERT(pid && pid->pid > 0);
    
    return _deregister_src(mod, M_SRC_TYPE_PID, (void *)pid);
}

int m_mod_register_task(mod_t *mod, const mod_task_t *tid, const m_src_flags flags, const void *userptr) {
    M_PARAM_ASSERT(tid && tid->fn);
    
    return _register_src(mod, M_SRC_TYPE_TASK, tid, flags | M_SRC_ONESHOT, userptr); // force ONESHOT flag
}

int m_mod_deregister_task(mod_t *mod, const mod_task_t *tid) {
    M_PARAM_ASSERT(tid);
    
    return _deregister_src(mod, M_SRC_TYPE_TASK, (void *)tid);
}

const char *m_mod_name(const mod_t *mod_self) {
    M_RET_ASSERT(mod_self, NULL);
    M_RET_ASSERT(mod_self->ctx->th_id == pthread_self(), NULL);
    M_RET_ASSERT(mod_self, NULL);
    
    return mod_self->name;
}

/** Module state getters **/

bool m_mod_is(const mod_t *mod_self, const mod_states st) {
    M_RET_ASSERT(mod_self, false);
    M_RET_ASSERT(mod_self->ctx->th_id == pthread_self(), false);
    M_RET_ASSERT(mod_self, false);
    
    return mod_self->state & st;
}

int m_mod_dump(const mod_t *mod) {
    M_MOD_ASSERT(mod);
    M_MOD_CTX(mod);
    
    uint64_t curr_ms;
    fetch_ms(&curr_ms, NULL);

    ctx_logger(c, mod, "{\n");
    ctx_logger(c, mod, "\t\"Name\": \"'%s\",\n", mod->name);
    ctx_logger(c, mod, "\t\"State\": %#x,\n", mod->state);
    ctx_logger(c, mod, "\t\"Flags\": %x,\n", mod->flags);
    if (mod->userdata) {
        ctx_logger(c, mod, "\t\"UP\": %p,\n", mod->userdata);
    }
    ctx_logger(c, mod, "\t\"Stats\": {\n");
    ctx_logger(c, mod, "\t\t\"Reg_time\": %lu,\n", mod->stats.registration_time);
    ctx_logger(c, mod, "\t\t\"Sent_msgs\": %lu,\n", mod->stats.sent_msgs);
    ctx_logger(c, mod, "\t\t\"Recv_msgs\": %lu,\n", mod->stats.recv_msgs);
    ctx_logger(c, mod, "\t\t\"Last_seen\": %lu,\n", mod->stats.last_seen);
    ctx_logger(c, mod, "\t\t\"Num_actions\": %lu,\n", mod->stats.action_ctr);
    ctx_logger(c, mod, "\t\t\"Action_freq\": %lf\n", (double)mod->stats.action_ctr / (curr_ms - mod->stats.registration_time));
    
    bool closed_stats = false;
    int i = 0;
    if (m_map_length(mod->subscriptions) > 0) {
        closed_stats = true;
        ctx_logger(c, mod, "\t},\n");
        ctx_logger(c, mod, "\t\"Subs\": [\n");
        m_itr_foreach(mod->subscriptions, {
            ev_src_t *sub = m_itr_get(itr);
            if (i++ > 0) {
                ctx_logger(c, mod, "\t},\n");
            }
            ctx_logger(c, mod, "\t{\n");
            ctx_logger(c, mod, "\t\t\"Topic\": \"%s\",\n", sub->ps_src.topic);
            if (sub->userptr) {
                ctx_logger(c, mod, "\t\t\"UP\": %p,\n", sub->userptr);
            }
            ctx_logger(c, mod, "\t\t\"Flags\": %#x\n", sub->flags);
        });
        ctx_logger(c, mod, "\t}\n");
        ctx_logger(c, mod, "\t],\n");
    }
    
    /* Skip internal M_SRC_TYPE_PS */
    for (int k = M_SRC_TYPE_FD; k < M_SRC_TYPE_END; k++) {
        if (m_bst_length(mod->srcs[k]) > 0) {
            if (!closed_stats) {
                ctx_logger(c, mod, "\t},\n");
                closed_stats = true;
            }
            ctx_logger(c, mod, "\t\"%s\": [\n", src_names[k]);
            i = 0;
            m_itr_foreach(mod->srcs[k], {
                ev_src_t *t = m_itr_get(itr);

                if (i++ > 0) {
                    ctx_logger(c, mod, "\t},\n");
                }
                
                ctx_logger(c, mod, "\t{\n");
                switch (t->type) {
                case M_SRC_TYPE_FD:
                    ctx_logger(c, mod, "\t\t\"FD\": %d,\n", t->fd_src.fd);
                    break;
                case M_SRC_TYPE_SGN:
                    ctx_logger(c, mod, "\t\t\"SGN\": %d,\n", t->sgn_src.sgs.signo);
                    break;
                case M_SRC_TYPE_TMR:
                    ctx_logger(c, mod, "\t\t\"TMR_MS\": %lu,\n", t->tmr_src.its.ms);
                    ctx_logger(c, mod, "\t\t\"TMR_CID\": %d,\n", t->tmr_src.its.clock_id);
                    break;
                case M_SRC_TYPE_PATH:
                    ctx_logger(c, mod, "\t\t\"PATH\": \"%s\",\n", t->path_src.pt.path);
                    ctx_logger(c, mod, "\t\t\"EV\": %#x,\n", t->path_src.pt.events);
                    break;
                case M_SRC_TYPE_PID:
                    ctx_logger(c, mod, "\t\t\"PID\": %d,\n", t->pid_src.pid.pid);
                    ctx_logger(c, mod, "\t\t\"EV\": %u,\n", t->pid_src.pid.events);
                    break;
                default:
                    break;
                }
                if (t->userptr) {
                    ctx_logger(c, mod, "\t\t\"UP\": %p,\n", t->userptr);
                }
                ctx_logger(c, mod, "\t\t\"Flags\": %#x\n", t->flags);
            });
            ctx_logger(c, mod, "\t}\n");
            ctx_logger(c, mod, "\t]\n");
        }
    }
    
    if (!closed_stats) {
        ctx_logger(c, mod, "\t}\n");
    }
    
    ctx_logger(c, mod, "}\n");
    return 0;
}

int m_mod_stats(const mod_t *mod, stats_t *stats) {
    M_MOD_ASSERT(mod);
    M_PARAM_ASSERT(stats);
    
    uint64_t curr_ms;
    fetch_ms(&curr_ms, NULL);
    
    stats->inactive_ms = curr_ms - mod->stats.last_seen;
    stats->activity_freq = ((double)mod->stats.action_ctr) / (curr_ms - mod->stats.registration_time);
    return 0;
}

/** Module state setters **/

int m_mod_start(mod_t *mod) {
    M_MOD_ASSERT_STATE(mod, IDLE | STOPPED);
    
    return start(mod, true);
}

int m_mod_pause(mod_t *mod) {
    M_MOD_ASSERT_STATE(mod, RUNNING);
    
    return stop(mod, false);
}

int m_mod_resume(mod_t *mod) {
    M_MOD_ASSERT_STATE(mod, PAUSED);
    
    return start(mod, false);
}

int m_mod_stop(mod_t *mod) {
    M_MOD_ASSERT_STATE(mod, RUNNING | PAUSED);
    
    return stop(mod, true);
}
