#include "module.h"
#include "poll_priv.h"

/** Generic module interface **/

static void _module_dtor(void *data);
static mod_ret init_ctx(const char *ctx_name, ctx_t **context, const mod_flags flags);
static void destroy_ctx(ctx_t *context);
static ctx_t *check_ctx(const char *ctx_name, const mod_flags flags);
static int _pipe(mod_t *mod);
static mod_ret init_pubsub_fd(mod_t *mod);
static void default_logger(const self_t *self, const char *fmt, va_list args);
static void src_priv_dtor(void *data);

static mod_ret _register_src(mod_t *mod, const mod_src_types type, const void *src_data, 
                             const mod_src_flags flags, const void *userptr);
static mod_ret _deregister_src(mod_t *mod, void *src_data, const mod_list_comp comp);

static mod_ret _register_fd(mod_t *mod, const int fd, const mod_src_flags flags, const void *userptr);
static int is_fd_same(void *my_data, void *list_data);
static mod_ret _deregister_fd(mod_t *mod, const int fd);

static mod_ret _register_tmr(mod_t *mod, const mod_tmr_t *its, 
                               const mod_src_flags flags, const void *userptr);
static int is_tmr_same(void *my_data, void *list_data);
static mod_ret _deregister_tmr(mod_t *mod, const mod_tmr_t *its);

static mod_ret _register_sgn(mod_t *mod, const mod_sgn_t *sgs, 
                             const mod_src_flags flags, const void *userptr);
static int is_sgn_same(void *my_data, void *list_data);
static mod_ret _deregister_sgn(mod_t *mod, const mod_sgn_t *sgs);

static mod_ret _register_pt(mod_t *mod, const mod_pt_t *pt, const mod_src_flags flags, const void *userptr);
static int is_pt_same(void *my_data, void *list_data);
static mod_ret _deregister_pt(mod_t *mod, const mod_pt_t *pt);

static mod_ret _register_pid(mod_t *mod, const mod_pid_t *pid, const mod_src_flags flags, const void *userptr);
static int is_pid_same(void *my_data, void *list_data);
static mod_ret _deregister_pid(mod_t *mod, const mod_pid_t *pid);

static int manage_fds(mod_t *mod, ctx_t *c, const int flag, const bool stop);
static void reset_module(mod_t *mod);

static void _module_dtor(void *data) {
    mod_t *mod = (mod_t *)data;
    if (mod) {
        /* Only if module has been registered before */
        if (mod->state != 0) {
            stop(mod, true);
        }
        
        map_free(mod->subscriptions);
        stack_free(mod->recvs);
        list_free(mod->srcs);
        
        memhook._free((void *)mod->local_path);
        
        if (mod->flags & MOD_NAME_AUTOFREE) {
            memhook._free((void *)mod->name);
        }
        
        /* Free fuse poll internal data */
        if (mod->fuse_ph) {
            memhook._free(mod->fuse_ph);
        }
        
        memhook._free(mod->self);
        mod->self = NULL;
        
        /*
         * Call destroy once self is NULL 
         * (ie: no more libmodule operations can be called on this handler) 
         */
        if (mod->hook.destroy) {
            mod->hook.destroy();
        }
        
        /* Finally free module */
        memhook._free(mod);
    }
}

static mod_ret init_ctx(const char *ctx_name, ctx_t **context, const mod_flags flags) {
    MODULE_DEBUG("Creating context '%s'.\n", ctx_name);
    
    *context = memhook._calloc(1, sizeof(ctx_t));
    MOD_ALLOC_ASSERT(*context);
    
    (*context)->flags = flags & ~(uint8_t)-1; // do not store useless module's flags (first byte)

    (*context)->logger = default_logger;
    
    (*context)->modules = map_new(false, _module_dtor);
    
    if ((*context)->flags & CTX_NAME_DUP) {
        (*context)->flags |= CTX_NAME_AUTOFREE;
        (*context)->name = mem_strdup(ctx_name);
    } else {
        (*context)->name = ctx_name;
    }
    
    if ((*context)->modules &&
        map_put(ctx, (*context)->name, *context) == MAP_OK) {
        
        if (poll_create(&(*context)->ppriv) == MOD_OK) {
            return MOD_OK;
        }
    }
    
    destroy_ctx(*context);
    *context = NULL;
    return MOD_ERR;
}

static void destroy_ctx(ctx_t *context) {
    MODULE_DEBUG("Destroying context '%s'.\n", context->name);
    map_free(context->modules);
    poll_destroy(&context->ppriv);
    map_remove(ctx, context->name);
    
    if (context->flags & CTX_NAME_AUTOFREE) {
        memhook._free((void *)context->name);
    }
    
    memhook._free(context);
}

static ctx_t *check_ctx(const char *ctx_name, const mod_flags flags) {
    ctx_t *context = map_get(ctx, ctx_name);
    if (!context) {
        init_ctx(ctx_name, &context, flags);
    }
    return context;
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

static mod_ret init_pubsub_fd(mod_t *mod) {
    if (_pipe(mod) == 0) {
        fd_src_t fd_src = {0};
        fd_src.fd = mod->pubsub_fd[0];
        if (_register_src(mod, TYPE_PS, &fd_src, SRC_FD_AUTOCLOSE, NULL) == MOD_OK) {
            return MOD_OK;
        }
        close(mod->pubsub_fd[0]);
        close(mod->pubsub_fd[1]);
        mod->pubsub_fd[0] = -1;
        mod->pubsub_fd[1] = -1;
    }
    return MOD_ERR;
}

static void default_logger(const self_t *self, const char *fmt, va_list args) {
    if (self) {
        printf("[%s]|%s|: ", self->ctx->name, self->mod->name);
    }
    vprintf(fmt, args);
}

static void src_priv_dtor(void *data) {
    ev_src_t *t = (ev_src_t *)data;
    
    ctx_t *c = t->self->ctx;
    /* If a fd is deregistered for a RUNNING module, stop polling on it */
    if (_module_is(t->self->mod, RUNNING)) {
        poll_set_new_evt(&c->ppriv, t, RM);
    }
    
    /* Properly manage autoclose flag */
    if (t->flags & SRC_FD_AUTOCLOSE) {
        int fd = -1;
        switch (t->type) {
        case TYPE_PS: // TYPE_PS is used for pubsub_fd[0] in init_pubsub_fd()
        case TYPE_FD:
            fd = t->fd_src.fd;
            break;
        default: 
            break;
        }
        if (fd != -1) {
            close(fd);
        }
    }
    
    if (t->flags & SRC_AUTOFREE) {
        memhook._free((void *)t->userptr);
    }
    
    memhook._free(t);
}

static mod_ret _register_src(mod_t *mod, const mod_src_types type, const void *src_data, 
                             const mod_src_flags flags, const void *userptr) {
    
    ev_src_t *src = memhook._calloc(1, sizeof(ev_src_t));
    MOD_ALLOC_ASSERT(src);
    
    src->flags = flags;
    src->userptr = userptr;
    src->type = type;
    src->self = &mod->ref;
    
    switch (type) {
    case TYPE_PS: // TYPE_PS is used for pubsub_fd[0] in init_pubsub_fd()
    case TYPE_FD: {
        fd_src_t *fd_src = &src->fd_src;
        int fd = *((int *)src_data);
        if (flags & SRC_DUP) {
            fd_src->fd = dup(fd);
        } else {
            fd_src->fd = fd;
        }
        break;
    }
    case TYPE_TMR: {
        tmr_src_t *tm_src = &src->tm_src;
        memcpy(&tm_src->its, src_data, sizeof(mod_tmr_t));
        tm_src->f.fd = -1;
        break;
    }
    case TYPE_SGN: {
        sgn_src_t *sgn_src = &src->sgn_src;
        memcpy(&sgn_src->sgs, src_data, sizeof(mod_sgn_t));
        sgn_src->f.fd = -1;
        break;
    }
    case TYPE_PT: {
        pt_src_t *pt_src = &src->pt_src;
        memcpy(&pt_src->pt, src_data, sizeof(mod_pt_t));
        pt_src->f.fd = -1;
        break;
    }
    case TYPE_PID: {
        pid_src_t *pid_src = &src->pid_src;
        memcpy(&pid_src->pid, src_data, sizeof(mod_pid_t));
        pid_src->f.fd = -1;
        break;
    }
    default:
        MODULE_DEBUG("Wrong src type: %d\n", type);
        memhook._free(src);
        return MOD_ERR;
    }
    
    fetch_ms(&mod->stats.last_seen, &mod->stats.action_ctr);
    list_insert(mod->srcs, src, NULL);
    ctx_t *c = mod->ref.ctx;
    /* If a fd is registered at runtime, start polling on it */
    int ret = 0;
    if (_module_is(mod, RUNNING)) {
        ret = poll_set_new_evt(&c->ppriv, src, ADD);
    }
    return !ret ? MOD_OK : MOD_ERR;
}

static mod_ret _deregister_src(mod_t *mod, void *src_data, const mod_list_comp comp) {
    if (list_remove(mod->srcs, src_data, comp) == LIST_OK) {
        fetch_ms(&mod->stats.last_seen, &mod->stats.action_ctr);
        return MOD_OK;
    }
    return MOD_ERR;
}

/* 
 * Append this fd to our list of fds and 
 * if module is in RUNNING state, start listening on its events 
 */
static mod_ret _register_fd(mod_t *mod, const int fd, const mod_src_flags flags, const void *userptr) {
    return _register_src(mod, TYPE_FD, &fd, flags, userptr);
}

static int is_fd_same(void *my_data, void *list_data) {
    ev_src_t *src = (ev_src_t *)list_data;
    int fd = *((int *)my_data);
    
    if (src->type == TYPE_FD) {
        return !(src->fd_src.fd == fd);
    }
    return 1;
}

/* Linearly search for fd */
static mod_ret _deregister_fd(mod_t *mod, const int fd) {
    return _deregister_src(mod, (void *)&fd, is_fd_same);
}

static mod_ret _register_tmr(mod_t *mod, const mod_tmr_t *its, const mod_src_flags flags, const void *userptr) {
    return _register_src(mod, TYPE_TMR, its, flags, userptr);
}

static int is_tmr_same(void *my_data, void *list_data) {
    ev_src_t *src = (ev_src_t *)list_data;
    const mod_tmr_t *its = (const mod_tmr_t *)my_data;
    
    if (src->type == TYPE_TMR) {
        return !(src->tm_src.its.ms == its->ms);
    }
    return 1;
}

/* Linearly search for its */
static mod_ret _deregister_tmr(mod_t *mod, const mod_tmr_t *its) { 
    return _deregister_src(mod, (void *)its, is_tmr_same);
}

static mod_ret _register_sgn(mod_t *mod, const mod_sgn_t *sgs, const mod_src_flags flags, const void *userptr) {
    return _register_src(mod, TYPE_SGN, sgs, flags, userptr);
}

static int is_sgn_same(void *my_data, void *list_data) {
    ev_src_t *src = (ev_src_t *)list_data;
    const mod_sgn_t *sgs = (const mod_sgn_t *)my_data;
    
    if (src->type == TYPE_SGN) {
        return !(src->sgn_src.sgs.signo == sgs->signo);
    }
    return 1;
}

/* Linearly search for sgs */
static mod_ret _deregister_sgn(mod_t *mod, const mod_sgn_t *sgs) {
    return _deregister_src(mod, (void *)sgs, is_sgn_same);
}

static mod_ret _register_pt(mod_t *mod, const mod_pt_t *pt, const mod_src_flags flags, const void *userptr) {
    return _register_src(mod, TYPE_PT, pt, flags, userptr);
}

static int is_pt_same(void *my_data, void *list_data) {
    ev_src_t *src = (ev_src_t *)list_data;
    const mod_pt_t *pt = (const mod_pt_t *)my_data;
    
    if (src->type == TYPE_PT) {
        return !(strcmp(src->pt_src.pt.path, pt->path));
    }
    return 1;
}

/* Linearly search for sgs */
static mod_ret _deregister_pt(mod_t *mod, const mod_pt_t *pt) {
    return _deregister_src(mod, (void *)pt, is_pt_same);
}

static mod_ret _register_pid(mod_t *mod, const mod_pid_t *pid, const mod_src_flags flags, const void *userptr) {
    return _register_src(mod, TYPE_PID, pid, flags, userptr);
}

static int is_pid_same(void *my_data, void *list_data) {
    ev_src_t *src = (ev_src_t *)list_data;
    const mod_pid_t *pid = (const mod_pid_t *)my_data;
    
    if (src->type == TYPE_PID) {
        return !(src->pid_src.pid.pid == pid->pid);
    }
    return 1;
}

/* Linearly search for sgs */
static mod_ret _deregister_pid(mod_t *mod, const mod_pid_t *pid) {
    return _deregister_src(mod, (void *)pid, is_pid_same);
}

static int manage_fds(mod_t *mod, ctx_t *c, const int flag, const bool stop) {    
    int ret = 0;
    
    for (mod_list_itr_t *itr = list_itr_new(mod->srcs); itr && !ret; itr = list_itr_next(itr)) {
        ev_src_t *t = list_itr_get_data(itr);
        if (flag == RM && stop) {
            if (t->type == TYPE_PS) {
                /*
                 * Free all unread pubsub msg for this module.
                 *
                 * Pass a !NULL pointer as first parameter,
                 * so that flush_pubsub_msgs() will free messages instead of delivering them.
                 */
                flush_pubsub_msgs(mod, NULL, mod);
            }
            ret = _deregister_src(mod, t, NULL);
        } else {
            ret = poll_set_new_evt(&c->ppriv, t, flag);
        }
    }
    return ret;
}

static void reset_module(mod_t *mod) {
    if (mod->pubsub_fd[1] != -1) {
        close(mod->pubsub_fd[1]);
        mod->pubsub_fd[0] = -1;
        mod->pubsub_fd[1] = -1;
    }
    map_clear(mod->subscriptions);
    stack_clear(mod->recvs);
}

/** Private API **/

bool _module_is(const mod_t *mod, const mod_states st) {
    return mod->state & st;
}

mod_map_ret evaluate_module(void *data, const char *key, void *value) {
    mod_t *mod = (mod_t *)value;
    if (_module_is(mod, IDLE) && 
        (!mod->hook.evaluate || mod->hook.evaluate())) {
        
        start(mod, true);
    }
    return MAP_OK;
}

mod_ret start(mod_t *mod, const bool starting) {
    static const char *errors[] = { "Failed to resume module.", "Failed to start module." };
    
    GET_CTX_PRIV((&mod->ref));

    /* 
     * Starting module for the first time
     * or after it was stopped.
     * Properly add back its pubsub fds.
     */
    if (starting) {
        /* THIS IS NOT A RESUME */
        if (init_pubsub_fd(mod) != MOD_OK) {
            return MOD_ERR;
        }
    }
    
    int ret = manage_fds(mod, c, ADD, false);
    MOD_ASSERT(!ret, errors[starting], MOD_ERR);
    
    mod->state = RUNNING;
    
    /* Call module init() callback only if module is being (re)started */
    if (!starting || mod->hook.init()) {
        MODULE_DEBUG("%s '%s'.\n", starting ? "Started" : "Resumed", mod->name);
        tell_system_pubsub_msg(NULL, c, MODULE_STARTED, &mod->ref, NULL);
        
        fetch_ms(&mod->stats.last_seen, &mod->stats.action_ctr);
        return MOD_OK;
    }
    
    /* If init() returns false, we need to stop this module right away */
    stop(mod, true);
    return MOD_ERR;
}

mod_ret stop(mod_t *mod, const bool stopping) {
    static const char *errors[] = { "Failed to pause module.", "Failed to stop module." };
    
    GET_CTX_PRIV((&mod->ref));
        
    int ret = manage_fds(mod, c, RM, stopping);
    MOD_ASSERT(!ret, errors[stopping], MOD_ERR);
    
    mod->state = stopping ? STOPPED : PAUSED;
    
    /*
     * When module gets stopped, its write-end pubsub fd is closed too 
     * Read-end is already closed by manage_fds().
     * Moreover, its subscriptions are cleared.
     */
    if (stopping) {
        reset_module(mod);
    }
    
    MODULE_DEBUG("%s '%s'.\n", stopping ? "Stopped" : "Paused", mod->name);
    
    /*
     * Check that we are not deregistering; this is needed to
     * avoid sending a &mod->self that will be freed right after 
     * (thus pointing to freed data).
     * When deregistering, module has already been removed from c->modules map.
     */
    self_t *self = NULL;
    if (map_has_key(c->modules, mod->name)) {
        self = &mod->ref;
    }
    tell_system_pubsub_msg(NULL, c, MODULE_STOPPED, self, NULL);
    fetch_ms(&mod->stats.last_seen, &mod->stats.action_ctr);
    return MOD_OK;
}

/** Public API **/

mod_ret module_register(const char *name, const char *ctx_name, self_t **self, const userhook_t *hook, const mod_flags flags) {
    MOD_PARAM_ASSERT(name);
    MOD_PARAM_ASSERT(ctx_name);
    MOD_PARAM_ASSERT(self);
    MOD_PARAM_ASSERT(!*self);
    MOD_PARAM_ASSERT(hook);
    /* Mandatory callbacks */
    MOD_PARAM_ASSERT(hook->init);
    MOD_PARAM_ASSERT(hook->recv);
    
    ctx_t *context = check_ctx(ctx_name, flags);
    MOD_ASSERT(context, "Failed to create context.", MOD_ERR);
    
    const mod_t *old_mod = map_get(context->modules, name);
    if (old_mod) {
        if (!(old_mod->flags & MOD_ALLOW_REPLACE)) { 
            MODULE_DEBUG("Module with same name already registered in context.");
            return MOD_EEXIST;
        }
        map_remove(context->modules, name);
    }
    
    MODULE_DEBUG("Registering module '%s'.\n", name);
    
    mod_t *mod = memhook._calloc(1, sizeof(mod_t));
    MOD_ALLOC_ASSERT(mod);
    
    mod->flags = flags & (uint8_t)-1; // do not store context's flags (only store first byte)
    if (flags & MOD_NAME_DUP) {
        mod->flags |= MOD_NAME_AUTOFREE;
    }
    
    mod_ret ret = MOD_NO_MEM;
    /* Let us gladly jump out with break on error */
    do {
        mod->name = flags & MOD_NAME_DUP ? mem_strdup(name) : name;
        
        mod->srcs = list_new(src_priv_dtor);
        if (!mod->srcs) {
            break;
        }
        
        mod->recvs = stack_new(NULL);
        if (!mod->recvs) {
            break;
        }
        
        /* External handler */
        mod->self = memhook._malloc(sizeof(self_t));
        if (!mod->self) {
            break;
        }
        
        /* External owner reference */
        memcpy(mod->self, &((self_t){ mod, context, false }), sizeof(self_t));
        
        /* Internal reference used as sender for pubsub messages */
        memcpy(&mod->ref, &((self_t){ mod, context, true }), sizeof(self_t));
        
        if (map_put(context->modules, mod->name, mod) == MAP_OK) {
            memcpy(&mod->hook, hook, sizeof(userhook_t));
            mod->state = IDLE;
            
            *self = mod->self;
            
            mod->pubsub_fd[0] = -1;
            mod->pubsub_fd[1] = -1;
            
            fetch_ms(&mod->stats.registration_time, NULL);
            return MOD_OK;
        }
        ret = MOD_ERR;
    } while (false);
    
    _module_dtor(mod);
    return ret;
}

mod_ret module_deregister(self_t **self) {
    MOD_PARAM_ASSERT(self);
    GET_MOD(*self);
    GET_CTX(*self);
    MODULE_DEBUG("Deregistering module '%s'.\n", mod->name);
    
    /* Remove the module from the context */
    map_remove(c->modules, mod->name);
    /* Remove context without modules */
    if (map_length(c->modules) == 0) {
        destroy_ctx(c);
    }
    *self = NULL;
    return MOD_OK;
}

mod_ret module_become(const self_t *self, const recv_cb new_recv) {
    MOD_PARAM_ASSERT(new_recv);
    GET_MOD_IN_STATE(self, RUNNING);
    
    if (stack_push(mod->recvs, new_recv) == STACK_OK) {
        fetch_ms(&mod->stats.last_seen, &mod->stats.action_ctr);
        return MOD_OK;
    }
    return MOD_ERR;
}

mod_ret module_unbecome(const self_t *self) {
    GET_MOD_IN_STATE(self, RUNNING);
    
    if (stack_pop(mod->recvs) != NULL) {
        fetch_ms(&mod->stats.last_seen, &mod->stats.action_ctr);
        return MOD_OK;
    }
    return MOD_ERR;
}

mod_ret module_log(const self_t *self, const char *fmt, ...) {
    GET_CTX(self);
    
    va_list args;
    va_start(args, fmt);
    c->logger(self, fmt, args);
    va_end(args);
    return MOD_OK;
}

mod_ret module_set_userdata(const self_t *self, const void *userdata) {
    GET_MOD(self);
    
    mod->userdata = userdata;
    return MOD_OK;
}

const void *module_get_userdata(const self_t *self) {
    MOD_ASSERT(self, "NULL self handler.", NULL);
    MOD_ASSERT(!self->is_ref, "Self is a reference object. It does not own module.", NULL);
    GET_MOD_PRIV(self);
    MOD_ASSERT(mod, "Module not found.", NULL);
    
    return mod->userdata;
}

mod_ret module_register_fd(const self_t *self, const int fd, const mod_src_flags flags, const void *userptr) {
    MOD_PARAM_ASSERT(fd >= 0);
    GET_MOD(self);

    return _register_fd(mod, fd, flags, userptr);
}

mod_ret module_deregister_fd(const self_t *self, const int fd) {
    MOD_PARAM_ASSERT(fd >= 0);
    GET_MOD(self);
    MOD_ASSERT(list_length(mod->srcs) > 0, "No srcs registered in this module.", MOD_ERR);
    
    return _deregister_fd(mod, fd);
}

mod_ret module_register_tmr(const self_t *self, const mod_tmr_t *its, const mod_src_flags flags, const void *userptr) {
    MOD_PARAM_ASSERT(its);
    MOD_PARAM_ASSERT(its->ms);
    GET_MOD(self);
    
    return _register_tmr(mod, its, flags, userptr);
}

mod_ret module_deregister_tmr(const self_t *self, const mod_tmr_t *its) {
    MOD_PARAM_ASSERT(its);
    MOD_PARAM_ASSERT(its->ms);
    GET_MOD(self);
    MOD_ASSERT(list_length(mod->srcs) > 0, "No srcs registered in this module.", MOD_ERR);
    
    return _deregister_tmr(mod, its);
}

mod_ret module_register_sgn(const self_t *self, const mod_sgn_t *sgs, const mod_src_flags flags, const void *userptr) {
    MOD_PARAM_ASSERT(sgs);
    GET_MOD(self);
    
    return _register_sgn(mod, sgs, flags, userptr);
}

mod_ret module_deregister_sgn(const self_t *self, const mod_sgn_t *sgs) {
    MOD_PARAM_ASSERT(sgs);
    GET_MOD(self);
    MOD_ASSERT(list_length(mod->srcs) > 0, "No srcs registered in this module.", MOD_ERR);
    
    return _deregister_sgn(mod, sgs);
}

mod_ret module_register_pt(const self_t *self, const mod_pt_t *pt, const mod_src_flags flags, const void *userptr) {
    MOD_PARAM_ASSERT(pt);
    MOD_PARAM_ASSERT(pt->path);
    MOD_PARAM_ASSERT(strlen(pt->path));
    MOD_PARAM_ASSERT(pt->events);
    GET_MOD(self);
    
    return _register_pt(mod, pt, flags, userptr);
}

mod_ret module_deregister_pt(const self_t *self, const mod_pt_t *pt) {
    MOD_PARAM_ASSERT(pt);
    MOD_PARAM_ASSERT(pt->path);
    MOD_PARAM_ASSERT(strlen(pt->path));
    GET_MOD(self);
    MOD_ASSERT(list_length(mod->srcs) > 0, "No srcs registered in this module.", MOD_ERR);
    
    return _deregister_pt(mod, pt);
}

mod_ret module_register_pid(const self_t *self, const mod_pid_t *pid, const mod_src_flags flags, const void *userptr) {
    MOD_PARAM_ASSERT(pid);
    MOD_PARAM_ASSERT(pid->pid > 0);
    GET_MOD(self);
    
    return _register_pid(mod, pid, flags, userptr);
}

mod_ret module_deregister_pid(const self_t *self, const mod_pid_t *pid) {
    MOD_PARAM_ASSERT(pid);
    MOD_PARAM_ASSERT(pid->pid > 0);
    GET_MOD(self);
    MOD_ASSERT(list_length(mod->srcs) > 0, "No srcs registered in this module.", MOD_ERR);
    
    return _deregister_pid(mod, pid);
}

const char *module_name(const self_t *mod_self) {
    MOD_ASSERT(mod_self, "NULL mod_self handler.", NULL);
    GET_MOD_PRIV(mod_self);
    MOD_ASSERT(mod, "Module not found.", NULL);
    
    return mod->name;
}

const char *module_ctx(const self_t *mod_self) {
    MOD_ASSERT(mod_self, "NULL mod_self handler.", NULL);
    GET_CTX_PRIV(mod_self);
    MOD_ASSERT(c, "Context not found.", NULL);
    
    return c->name;
}

/** Module state getters **/

bool module_is(const self_t *mod_self, const mod_states st) {
    MOD_ASSERT((mod_self), "NULL self handler.", false);

    return _module_is(mod_self->mod, st);
}

mod_ret module_dump(const self_t *self) {
    GET_MOD(self); 
    GET_CTX(self);
    
    uint64_t curr_ms;
    fetch_ms(&curr_ms, NULL);

    ctx_logger(c, self, "{\n");
    ctx_logger(c, self, "\t\"Name\": \"'%s\",\n", self->mod->name);
    ctx_logger(c, self, "\t\"State\": %#x,\n", mod->state);
    if (mod->userdata) {
        ctx_logger(c, self, "\t\"Userdata\": %p,\n", mod->userdata);
    }
    ctx_logger(c, self, "\t\"Stats\": {\n");
    ctx_logger(c, self, "\t\t\"Registration time\": %lu,\n", mod->stats.registration_time);
    ctx_logger(c, self, "\t\t\"Sent messages\": %lu,\n", mod->stats.sent_msgs);
    ctx_logger(c, self, "\t\t\"Received messages\": %lu,\n", mod->stats.recv_msgs);
    ctx_logger(c, self, "\t\t\"Last activity\": %lu,\n", mod->stats.last_seen);
    ctx_logger(c, self, "\t\t\"Num actions\": %lu,\n", mod->stats.action_ctr);
    ctx_logger(c, self, "\t\t\"Action frequency\": %lf\n", (double)mod->stats.action_ctr / (curr_ms - mod->stats.registration_time));
    ctx_logger(c, self, "\t},\n");
    
    ctx_logger(c, self, "\t\"Subs\": [\n");
    for (mod_map_itr_t *itr = map_itr_new(mod->subscriptions); itr; itr = map_itr_next(itr)) {
        ev_src_t *sub = map_itr_get_data(itr);
        ctx_logger(c, self, "\t{\n");
        ctx_logger(c, self, "\t\t\"Topic\": \"%s\",\n", sub->ps_src.topic);
        if (sub->userptr) {
            ctx_logger(c, self, "\t\t\"UP\": %p,\n", sub->userptr);
        }
        ctx_logger(c, self, "\t\t\"Flags\": %#x\n", sub->flags);
        ctx_logger(c, self, "\t},\n");
    }
    ctx_logger(c, self, "\t],\n");
    
    ctx_logger(c, self, "\t\"Srcs\": [\n");
    for (mod_list_itr_t *itr = list_itr_new(mod->srcs); itr; itr = list_itr_next(itr)) {
        ev_src_t *t = list_itr_get_data(itr);
        if (t->type == TYPE_PS) {
            /* Do not log information about internal pubsub pipe */
            continue;
        }
        
        ctx_logger(c, self, "\t{\n");
        switch (t->type) {
        case TYPE_FD:
            ctx_logger(c, self, "\t\t\"FD\": %d,\n", t->fd_src.fd);
            break;
        case TYPE_SGN:
            ctx_logger(c, self, "\t\t\"SGN\": %d,\n", t->sgn_src.sgs.signo);
            break;
        case TYPE_TMR:
            ctx_logger(c, self, "\t\t\"TMR_MS\": %lu,\n", t->tm_src.its.ms);
            ctx_logger(c, self, "\t\t\"TMR_CID\": %d,\n", t->tm_src.its.clock_id);
            break;
        case TYPE_PT:
            ctx_logger(c, self, "\t\t\"PATH\": \"%s\",\n", t->pt_src.pt.path);
            ctx_logger(c, self, "\t\t\"EV\": %u,\n", t->pt_src.pt.events);
            break;
        case TYPE_PID:
            ctx_logger(c, self, "\t\t\"PID\": %d,\n", t->pid_src.pid.pid);
            ctx_logger(c, self, "\t\t\"EV\": %u,\n", t->pid_src.pid.events);
            break;
        default:
            break;
        }
        if (t->userptr) {
            ctx_logger(c, self, "\t\t\"UP\": %p,\n", t->userptr);
        }
        ctx_logger(c, self, "\t\t\"Flags\": %#x\n", t->flags);
        ctx_logger(c, self, "\t},\n");
    }
    ctx_logger(c, self, "\t]\n");
    ctx_logger(c, self, "}\n");
    return MOD_OK;
}

mod_ret module_stats(const self_t *self, stats_t *stats) {
    GET_MOD_PURE(self);
    MOD_PARAM_ASSERT(stats);
    
    uint64_t curr_ms;
    fetch_ms(&curr_ms, NULL);
    
    stats->inactive_ms = curr_ms - mod->stats.last_seen;
    stats->activity_freq = ((double)mod->stats.action_ctr) / (curr_ms - mod->stats.registration_time);
    return MOD_OK;
}

/** Module state setters **/

mod_ret module_start(const self_t *self) {
    GET_MOD_IN_STATE(self, IDLE | STOPPED);
    
    return start(mod, true);
}

mod_ret module_pause(const self_t *self) {
    GET_MOD_IN_STATE(self, RUNNING);
    
    return stop(mod, false);
}

mod_ret module_resume(const self_t *self) {
    GET_MOD_IN_STATE(self, PAUSED);
    
    return start(mod, false);
}

mod_ret module_stop(const self_t *self) {
    GET_MOD_IN_STATE(self, RUNNING | PAUSED);
    
    return stop(mod, true);
}
