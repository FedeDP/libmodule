#include "mod.h"
#include "poll_priv.h"
#include <dlfcn.h> // dlopen

/** Generic module interface **/

static void module_dtor(void *data);
static int _pipe(m_mod_t *mod);
static int init_pubsub_fd(m_mod_t *mod);
static void src_priv_dtor(void *data);
static void *task_thread(void *data);
static int start_task(ev_src_t *src);

static int _register_src(m_mod_t *mod, const m_src_types type, const void *src_data, 
                             const m_src_flags flags, const void *userptr);
static int _deregister_src(m_mod_t *mod, const m_src_types type, void *src_data);

static int fdcmp(void *my_data, void *node_data);
static int tmrcmp(void *my_data, void *node_data);
static int sgncmp(void *my_data, void *node_data);
static int pathcmp(void *my_data, void *node_data);
static int pidcmp(void *my_data, void *node_data);
static int taskcmp(void *my_data, void *node_data);
static int threshcmp(void *my_data, void *node_data);

static int manage_fds(m_mod_t *mod, m_ctx_t *c, const int flag, const bool stop);
static void reset_module(m_mod_t *mod);

extern int fs_cleanup(m_mod_t *mod);

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
static const char *src_names[] = {
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

static void module_dtor(void *data) {
    m_mod_t *mod = (m_mod_t *)data;
    if (mod) {
        m_mem_unref(ctx);
        
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

static int _pipe(m_mod_t *mod) {
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

static int init_pubsub_fd(m_mod_t *mod) {
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

    /* If a fd is deregistered for a RUNNING module, stop polling on it */
    if (m_mod_is(t->mod, M_MOD_RUNNING)) {
        poll_set_new_evt(&ctx->ppriv, t, RM);
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

static int _register_src(m_mod_t *mod, const m_src_types type, const void *src_data, 
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
        break;
    }
    case M_SRC_TYPE_THRESH: {
        thresh_src_t *thresh_src = &src->thresh_src;
        memcpy(&thresh_src->thr, src_data, sizeof(m_src_thresh_t));
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
        if (m_mod_is(mod, M_MOD_RUNNING)) {
            ret = poll_set_new_evt(&ctx->ppriv, src, ADD);

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

static int _deregister_src(m_mod_t *mod, const m_src_types type, void *src_data) {   
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
    const m_src_tmr_t *its = (const m_src_tmr_t *)my_data;
    
    return its->ms - src->tmr_src.its.ms;
}

static int sgncmp(void *my_data, void *node_data) {
    ev_src_t *src = (ev_src_t *)node_data;
    const m_src_sgn_t *sgs = (const m_src_sgn_t *)my_data;
    
    return sgs->signo - src->sgn_src.sgs.signo;
}

static int pathcmp(void *my_data, void *node_data) {
    ev_src_t *src = (ev_src_t *)node_data;
    const m_src_path_t *pt = (const m_src_path_t *)my_data;
    
    return strcmp(pt->path, src->path_src.pt.path);
}

static int pidcmp(void *my_data, void *node_data) {
    ev_src_t *src = (ev_src_t *)node_data;
    const m_src_pid_t *pid = (const m_src_pid_t *)my_data;
    
    return pid->pid - src->pid_src.pid.pid;
}

static int taskcmp(void *my_data, void *node_data) {
    ev_src_t *src = (ev_src_t *)node_data;
    const m_src_task_t *tid = (const m_src_task_t *)my_data;
    
    return tid->tid - src->task_src.tid.tid;
}

static int threshcmp(void *my_data, void *node_data) {
    ev_src_t *src = (ev_src_t *)node_data;
    const m_src_thresh_t *thr = (const m_src_thresh_t *)my_data;

    long double my_val = (long double)thr->activity_freq
            + (long double)thr->inactive_ms;
    long double their_val = (long double)src->thresh_src.thr.activity_freq
            + (long double)src->thresh_src.thr.inactive_ms;
    return my_val - their_val;
}

static int manage_fds(m_mod_t *mod, m_ctx_t *c, const int flag, const bool stop) {    
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

static void reset_module(m_mod_t *mod) {
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
    m_mod_t *mod = (m_mod_t *)value;

    /* Check thresholds, if any is set and mod is running */
    if (m_mod_is(mod, M_MOD_RUNNING)) {
        uint64_t curr_ms;
        fetch_ms(&curr_ms, NULL);

        m_itr_foreach(mod->srcs[M_SRC_TYPE_THRESH], {
            ev_src_t *src = (ev_src_t *)m_itr_get(itr);
            m_src_thresh_t *thr = &src->thresh_src.thr;
            m_src_thresh_t *alarm = &src->thresh_src.alarm;
            bool notify = false;
            if (thr->activity_freq != 0) {
                alarm->activity_freq = (double) mod->stats.action_ctr / (curr_ms - mod->stats.registration_time);
                notify = alarm->activity_freq >= thr->activity_freq;
            }

            if (thr->inactive_ms != 0) {
                alarm->inactive_ms = curr_ms - mod->stats.last_seen;
                notify = alarm->inactive_ms >= thr->inactive_ms;
            }
            if (notify) {
                poll_notify_thresh(src);
            }
        })
    }

    /* Check if module should be started */
    if (m_mod_is(mod, M_MOD_IDLE) && 
        (!mod->hook.on_eval || mod->hook.on_eval())) {
        
        start(mod, true);
    }
    return 0;
}

int start(m_mod_t *mod, bool starting) {
    static const char *errors[] = { "Failed to resume module.", "Failed to start module." };

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
    
    int ret = manage_fds(mod, ctx, ADD, false);
    M_ASSERT(!ret, errors[starting], ret);
    
    mod->state = M_MOD_RUNNING;
    ctx->stats.running_modules++;

    /* Call module init() callback only if module is being (re)started */
    if (!starting || !mod->hook.on_start || mod->hook.on_start()) {
        M_DEBUG("%s '%s'.\n", starting ? "Started" : "Resumed", mod->name);
        tell_system_pubsub_msg(NULL, ctx, M_PS_MOD_STARTED, mod, NULL);
        return 0;
    }
    
    /* If init() returns false, we need to stop this module right away */
    stop(mod, true);
    return 0;
}

int stop(m_mod_t *mod, bool stopping) {
    static const char *errors[] = { "Failed to pause module.", "Failed to stop module." };

    int ret = manage_fds(mod, ctx, RM, stopping);
    M_ASSERT(!ret, errors[stopping], ret);
    
    mod->state = stopping ? M_MOD_STOPPED : M_MOD_PAUSED;
    ctx->stats.running_modules--;
    
    /*
     * When module gets stopped, its write-end pubsub fd is closed too 
     * Read-end is already closed by manage_fds().
     * Moreover, its subscriptions are cleared.
     * 
     * Finally, deinit() callback is called.
     */
    if (stopping) {
        reset_module(mod);
        if (mod->hook.on_stop) {
            mod->hook.on_stop();
        }
    }
    
    M_DEBUG("%s '%s'.\n", stopping ? "Stopped" : "Paused", mod->name);
    
    tell_system_pubsub_msg(NULL, ctx, M_PS_MOD_STOPPED, mod, NULL);
    return 0;
}

/** Public API **/

_public_ int m_mod_register(const char *name, m_mod_t **self, const m_mod_hook_t *hook,
                            m_mod_flags flags, const void *userdata) {
    M_PARAM_ASSERT(name);
    M_PARAM_ASSERT(self);
    M_PARAM_ASSERT(!*self);
    M_PARAM_ASSERT(hook);
    /* Mandatory callback */
    M_PARAM_ASSERT(hook->on_evt);

    int ret;
    m_mod_t *old_mod = m_map_get(ctx->modules, name);
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
    
    m_mod_t *mod = m_mem_new(sizeof(m_mod_t), module_dtor);
    M_ALLOC_ASSERT(mod);

    ctx = m_mem_ref(ctx);
    
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
        
        if (m_map_put(ctx->modules, mod->name, mod) == 0) {
            memcpy(&mod->hook, hook, sizeof(m_mod_hook_t));
            mod->state = M_MOD_IDLE;
            
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

_public_ int m_mod_deregister(m_mod_t **mod) {
    M_PARAM_ASSERT(mod);
    M_MOD_ASSERT((*mod));

    m_mod_t *m = *mod;
    if ((m->flags & M_MOD_PERSIST) && ctx->looping) {
        return -EPERM;
    }
    
    M_DEBUG("Deregistering module '%s'.\n", m->name);
    
    /* Keep module alive untile destroy() callback is called */
    M_MEM_LOCK(m, {
        /* Stop module */
        stop(m, true);
        m->state = M_MOD_ZOMBIE;
        
        /* Remove the module from the context */
        m_map_remove(ctx->modules, m->name);
        
        /* Free FS internal data */
        fs_cleanup(m);
        
        /*
        * Call destroy once self is NULL and module has been removed from context;
        * ie: no more libmodule operations can be called on this handler 
        * neither it can be ref'd through module_ref.
        */
        *mod = NULL;
    });
    return 0;
}


_public_ int m_mod_load(const m_mod_t *mod, const char *module_path, m_mod_flags flags, m_mod_t **ref) {
    M_MOD_ASSERT_PERM(mod, M_MOD_DENY_LOAD);
    M_PARAM_ASSERT(module_path);
    
    const int module_size = m_map_length(ctx->modules);
    
    void *handle = dlopen(module_path, RTLD_NOW);
    if (!handle) {
        M_DEBUG("Dlopen failed with error: %s\n", dlerror());
        return -errno;
    }
    
    /* 
     * Check that requested module has been created in requested ctx, 
     * by looking at requested ctx number of modules
     */
    if (module_size == m_map_length(ctx->modules)) {
        dlclose(handle);
        return -EPERM;
    }
    
    /* Take most recently loaded module */
    m_mod_t *new_mod = m_map_peek(ctx->modules);
    new_mod->local_path = mem_strdup(module_path);
    new_mod->flags |= flags;
    
    /* Store a reference to new module if requested */
    if (ref != NULL) {
        *ref = new_mod;
    }
    return 0;
}

_public_ int m_mod_unload(const m_mod_t *mod, const char *module_path) {
    M_MOD_ASSERT_PERM(mod, M_MOD_DENY_LOAD);
    M_PARAM_ASSERT(module_path);
    
    /* Check if desired module is actually loaded in context */
    bool found = false;
    m_itr_foreach(ctx->modules, {
        m_mod_t *mod = m_itr_get(itr);
        if (mod->local_path && !strcmp(mod->local_path, module_path)) {
            found = true;
            memhook._free(itr);
            break;
        }
    });
    
    if (found) {
        void *handle = dlopen(module_path, RTLD_NOLOAD);
        if (handle) {
            dlclose(handle);
            return 0;
        }
        M_DEBUG("Dlopen failed with error: %s\n", dlerror());
        return -errno;
    }
    M_DEBUG("Module loaded from '%s' not found in ctx '%s'.\n", module_path, ctx->name);
    return -ENODEV;
}

_public_ int m_mod_become(m_mod_t *mod, m_evt_cb new_recv) {
    M_PARAM_ASSERT(new_recv);
    M_MOD_ASSERT_STATE(mod, M_MOD_RUNNING);
    
    int ret = m_stack_push(mod->recvs, new_recv);
    if (ret == 0) {
        fetch_ms(&mod->stats.last_seen, &mod->stats.action_ctr);
    }
    return ret;
}

_public_ int m_mod_unbecome(m_mod_t *mod) {
    M_MOD_ASSERT_STATE(mod, M_MOD_RUNNING);
    
    if (m_stack_pop(mod->recvs) != NULL) {
        fetch_ms(&mod->stats.last_seen, &mod->stats.action_ctr);
        return 0;
    }
    return -EINVAL;
}

_public_ __attribute__((format (printf, 2, 3))) int m_mod_log(const m_mod_t *mod, const char *fmt, ...) {
    M_MOD_ASSERT(mod);

    va_list args;
    va_start(args, fmt);
    ctx->logger(mod, fmt, args);
    va_end(args);
    return 0;
}

_public_ const void *m_mod_userdata(const m_mod_t *mod) {
    M_RET_ASSERT(mod, NULL);
    M_RET_ASSERT(!m_mod_is(mod, M_MOD_ZOMBIE), NULL);
    
    return mod->userdata;
}

_public_ int m_mod_src_register_fd(m_mod_t *mod, int fd, m_src_flags flags, const void *userptr) {
    M_PARAM_ASSERT(fd >= 0);

    return _register_src(mod, M_SRC_TYPE_FD, &fd, flags, userptr);
}

_public_ int m_mod_src_deregister_fd(m_mod_t *mod, int fd) {
    M_PARAM_ASSERT(fd >= 0);
    
    return _deregister_src(mod, M_SRC_TYPE_FD, (void *)&fd);
}

_public_ int m_mod_src_register_tmr(m_mod_t *mod, const m_src_tmr_t *its, m_src_flags flags, const void *userptr) {
    M_PARAM_ASSERT(its && its->ms > 0);
    
    return _register_src(mod, M_SRC_TYPE_TMR, its, flags, userptr);
}

_public_ int m_mod_src_deregister_tmr(m_mod_t *mod, const m_src_tmr_t *its) {
    M_PARAM_ASSERT(its && its->ms > 0);
    
    return _deregister_src(mod, M_SRC_TYPE_TMR, (void *)its);
}

_public_ int m_mod_src_register_sgn(m_mod_t *mod, const m_src_sgn_t *sgs, m_src_flags flags, const void *userptr) {
    M_PARAM_ASSERT(sgs && sgs->signo > 0);
    
    return _register_src(mod, M_SRC_TYPE_SGN, sgs, flags, userptr);
}

_public_ int m_mod_src_deregister_sgn(m_mod_t *mod, const m_src_sgn_t *sgs) {
    M_PARAM_ASSERT(sgs && sgs->signo > 0);
    
    return _deregister_src(mod, M_SRC_TYPE_SGN, (void *)sgs);
}

_public_ int m_mod_src_register_path(m_mod_t *mod, const m_src_path_t *pt, m_src_flags flags, const void *userptr) {
    M_PARAM_ASSERT(pt);
    M_PARAM_ASSERT(pt->path && strlen(pt->path));
    M_PARAM_ASSERT(pt->events > 0);
    
    return _register_src(mod, M_SRC_TYPE_PATH, pt, flags, userptr);
}

_public_ int m_mod_src_deregister_path(m_mod_t *mod, const m_src_path_t *pt) {
    M_PARAM_ASSERT(pt);
    M_PARAM_ASSERT(pt->path && strlen(pt->path));
    
    return _deregister_src(mod, M_SRC_TYPE_PATH, (void *)pt);
}

_public_ int m_mod_src_register_pid(m_mod_t *mod, const m_src_pid_t *pid, m_src_flags flags, const void *userptr) {
    M_PARAM_ASSERT(pid && pid->pid > 0);
    
    return _register_src(mod, M_SRC_TYPE_PID, pid, flags, userptr);
}

_public_ int m_mod_src_deregister_pid(m_mod_t *mod, const m_src_pid_t *pid) {
    M_PARAM_ASSERT(pid && pid->pid > 0);
    
    return _deregister_src(mod, M_SRC_TYPE_PID, (void *)pid);
}

_public_ int m_mod_src_register_task(m_mod_t *mod, const m_src_task_t *tid, m_src_flags flags, const void *userptr) {
    M_PARAM_ASSERT(tid && tid->fn);
    
    return _register_src(mod, M_SRC_TYPE_TASK, tid, flags | M_SRC_ONESHOT, userptr); // force ONESHOT flag
}

_public_ int m_mod_src_deregister_task(m_mod_t *mod, const m_src_task_t *tid) {
    M_PARAM_ASSERT(tid);
    
    return _deregister_src(mod, M_SRC_TYPE_TASK, (void *)tid);
}

_public_ int m_mod_src_register_thresh(m_mod_t *mod, const m_src_thresh_t *thr, m_src_flags flags, const void *userptr) {
    M_PARAM_ASSERT(thr && (thr->activity_freq > 0 || thr->inactive_ms > 0));

    return _register_src(mod, M_SRC_TYPE_THRESH, thr, flags | M_SRC_ONESHOT, userptr); // force ONESHOT flag
}

_public_ int m_mod_src_deregister_thresh(m_mod_t *mod, const m_src_thresh_t *thr) {
    M_PARAM_ASSERT(thr && (thr->activity_freq > 0 || thr->inactive_ms > 0));

    return _deregister_src(mod, M_SRC_TYPE_THRESH, (void *)thr);
}

_public_ _pure_ const char *m_mod_name(const m_mod_t *mod_self) {
    M_RET_ASSERT(mod_self, NULL);

    return mod_self->name;
}

/** Module state getters **/

_public_ _pure_ bool m_mod_is(const m_mod_t *mod_self, m_mod_states st) {
    M_RET_ASSERT(mod_self, false);

    return mod_self->state & st;
}

_public_ int m_mod_dump(const m_mod_t *mod) {
    M_MOD_ASSERT(mod);

    uint64_t curr_ms;
    fetch_ms(&curr_ms, NULL);

    ctx_logger(ctx, mod, "{\n");
    ctx_logger(ctx, mod, "\t\"Name\": \"'%s\",\n", mod->name);
    ctx_logger(ctx, mod, "\t\"State\": %#x,\n", mod->state);
    if (mod->flags) {
        ctx_logger(ctx, mod, "\t\"Flags\": %x,\n", mod->flags);
    }
    if (mod->userdata) {
        ctx_logger(ctx, mod, "\t\"UP\": %p,\n", mod->userdata);
    }

    ctx_logger(ctx, mod, "\t\"Stats\": {\n");
    ctx_logger(ctx, mod, "\t\t\"Reg_time\": %" PRIu64 ",\n", mod->stats.registration_time);
    ctx_logger(ctx, mod, "\t\t\"Sent_msgs\": %" PRIu64 ",\n", mod->stats.sent_msgs);
    ctx_logger(ctx, mod, "\t\t\"Recv_msgs\": %" PRIu64 ",\n", mod->stats.recv_msgs);
    ctx_logger(ctx, mod, "\t\t\"Last_seen\": %" PRIu64 ",\n", mod->stats.last_seen);
    ctx_logger(ctx, mod, "\t\t\"Num_actions\": %" PRIu64 ",\n", mod->stats.action_ctr);
    ctx_logger(ctx, mod, "\t\t\"Action_freq\": %Lf\n", (long double)mod->stats.action_ctr / (curr_ms - mod->stats.registration_time));
    
    bool closed_stats = false;
    int i = 0;
    if (m_map_length(mod->subscriptions) > 0) {
        closed_stats = true;
        ctx_logger(ctx, mod, "\t},\n");
        ctx_logger(ctx, mod, "\t\"Subs\": [\n");
        m_itr_foreach(mod->subscriptions, {
            ev_src_t *sub = m_itr_get(itr);
            if (i++ > 0) {
                ctx_logger(ctx, mod, "\t},\n");
            }
            ctx_logger(ctx, mod, "\t{\n");
            ctx_logger(ctx, mod, "\t\t\"Topic\": \"%s\",\n", sub->ps_src.topic);
            if (sub->userptr) {
                ctx_logger(ctx, mod, "\t\t\"UP\": %p,\n", sub->userptr);
            }
            if (sub->flags) {
                ctx_logger(ctx, mod, "\t\t\"Flags\": %#x\n", sub->flags);
            }
        });
        ctx_logger(ctx, mod, "\t}\n");
        ctx_logger(ctx, mod, "\t],\n");
    }
    
    /* Skip internal M_SRC_TYPE_PS */
    for (int k = M_SRC_TYPE_FD; k < M_SRC_TYPE_END; k++) {
        if (m_bst_length(mod->srcs[k]) > 0) {
            if (!closed_stats) {
                ctx_logger(ctx, mod, "\t},\n");
                closed_stats = true;
            }
            ctx_logger(ctx, mod, "\t\"%s\": [\n", src_names[k]);
            i = 0;
            m_itr_foreach(mod->srcs[k], {
                ev_src_t *t = m_itr_get(itr);

                if (i++ > 0) {
                    ctx_logger(ctx, mod, "\t},\n");
                }
                
                ctx_logger(ctx, mod, "\t{\n");
                switch (t->type) {
                case M_SRC_TYPE_FD:
                    ctx_logger(ctx, mod, "\t\t\"FD\": %d,\n", t->fd_src.fd);
                    break;
                case M_SRC_TYPE_SGN:
                    ctx_logger(ctx, mod, "\t\t\"SGN\": %d,\n", t->sgn_src.sgs.signo);
                    break;
                case M_SRC_TYPE_TMR:
                    ctx_logger(ctx, mod, "\t\t\"TMR_MS\": %lu,\n", t->tmr_src.its.ms);
                    ctx_logger(ctx, mod, "\t\t\"TMR_CID\": %d,\n", t->tmr_src.its.clock_id);
                    break;
                case M_SRC_TYPE_PATH:
                    ctx_logger(ctx, mod, "\t\t\"PATH\": \"%s\",\n", t->path_src.pt.path);
                    ctx_logger(ctx, mod, "\t\t\"EV\": %#x,\n", t->path_src.pt.events);
                    break;
                case M_SRC_TYPE_PID:
                    ctx_logger(ctx, mod, "\t\t\"PID\": %d,\n", t->pid_src.pid.pid);
                    ctx_logger(ctx, mod, "\t\t\"EV\": %u,\n", t->pid_src.pid.events);
                    break;
                case M_SRC_TYPE_TASK:
                    ctx_logger(ctx, mod, "\t\t\"TID\": %d,\n", t->task_src.tid.tid);
                    break;
                case M_SRC_TYPE_THRESH:
                    ctx_logger(ctx, mod, "\t\t\"INACTIVE_MS\": %d,\n", t->thresh_src.thr.inactive_ms);
                    ctx_logger(ctx, mod, "\t\t\"ACTIVITY_FREQ\": %d,\n", t->thresh_src.thr.activity_freq);
                    break;
                default:
                    break;
                }
                if (t->userptr) {
                    ctx_logger(ctx, mod, "\t\t\"UP\": %p,\n", t->userptr);
                }
                if (t->flags) {
                    ctx_logger(ctx, mod, "\t\t\"Flags\": %#x\n", t->flags);
                }
            });
            ctx_logger(ctx, mod, "\t}\n");
            ctx_logger(ctx, mod, "\t]\n");
        }
    }
    
    if (!closed_stats) {
        ctx_logger(ctx, mod, "\t}\n");
    }
    
    ctx_logger(ctx, mod, "}\n");
    return 0;
}

_public_ _pure_ int m_mod_stats(const m_mod_t *mod, m_mod_stats_t *stats) {
    M_MOD_ASSERT(mod);
    M_PARAM_ASSERT(stats);
    
    uint64_t curr_ms;
    fetch_ms(&curr_ms, NULL);
    
    stats->inactive_ms = curr_ms - mod->stats.last_seen;
    stats->activity_freq = ((double)mod->stats.action_ctr) / (curr_ms - mod->stats.registration_time);
    stats->recv_msgs = mod->stats.recv_msgs;
    stats->sent_msgs = mod->stats.sent_msgs;
    return 0;
}

/** Module state setters **/

_public_ int m_mod_start(m_mod_t *mod) {
    M_MOD_ASSERT_STATE(mod, M_MOD_IDLE | M_MOD_STOPPED);
    
    return start(mod, true);
}

_public_ int m_mod_pause(m_mod_t *mod) {
    M_MOD_ASSERT_STATE(mod, M_MOD_RUNNING);
    
    return stop(mod, false);
}

_public_ int m_mod_resume(m_mod_t *mod) {
    M_MOD_ASSERT_STATE(mod, M_MOD_PAUSED);
    
    return start(mod, false);
}

_public_ int m_mod_stop(m_mod_t *mod) {
    M_MOD_ASSERT_STATE(mod, M_MOD_RUNNING | M_MOD_PAUSED);
    
    return stop(mod, true);
}
