#include "module.h"
#include "poll_priv.h"
#include <dlfcn.h> // dlopen

/** Generic module interface **/

static module_ret_code init_ctx(const char *ctx_name, m_context **context);
static void destroy_ctx(m_context *context);
static m_context *check_ctx(const char *ctx_name);
static int _pipe(module *mod);
static module_ret_code init_pubsub_fd(module *mod);
static void default_logger(const self_t *self, const char *fmt, va_list args, const void *userdata);
static module_ret_code _register_fd(module *mod, const int fd, const bool autoclose, const void *userptr);
static module_ret_code _deregister_fd(module *mod, const int fd);
static int manage_fds(module *mod, m_context *c, const int flag, const bool stop);

static module_ret_code init_ctx(const char *ctx_name, m_context **context) {
    MODULE_DEBUG("Creating context '%s'.\n", ctx_name);
    
    *context = memhook._calloc(1, sizeof(m_context));
    MOD_ALLOC_ASSERT(*context);
        
    (*context)->fd = poll_create();
    MOD_ASSERT((*context)->fd >= 0, "Failed to create context fd.", MOD_ERR);
     
    (*context)->logger = default_logger;
    
    (*context)->modules = map_new(false, NULL);
    (*context)->topics = map_new(false, NULL);
    
    (*context)->name = ctx_name;
    if ((*context)->topics && (*context)->modules &&
        map_put(ctx, (*context)->name, *context) == MAP_OK) {
        
        return MOD_OK;
    }
    
    destroy_ctx(*context);
    *context = NULL;
    return MOD_ERR;
}

static void destroy_ctx(m_context *context) {
    MODULE_DEBUG("Destroying context '%s'.\n", context->name);
    map_free(context->modules);
    map_free(context->topics);
    poll_close(context->fd, &context->pevents, &context->max_events);
    map_remove(ctx, context->name);
    memhook._free(context);
}

static m_context *check_ctx(const char *ctx_name) {
    m_context *context = map_get(ctx, ctx_name);
    if (!context) {
        init_ctx(ctx_name, &context);
    }
    return context;
}

static int _pipe(module *mod) {
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

static module_ret_code init_pubsub_fd(module *mod) {
    if (_pipe(mod) == 0) {
        if (_register_fd(mod, mod->pubsub_fd[0], true, NULL) == MOD_OK) {
            return MOD_OK;
        }
        close(mod->pubsub_fd[0]);
        close(mod->pubsub_fd[1]);
        mod->pubsub_fd[0] = -1;
        mod->pubsub_fd[1] = -1;
    }
    return MOD_ERR;
}

static void default_logger(const self_t *self, const char *fmt, va_list args, const void *userdata) {
    printf("[%s]|%s|: ", self->ctx->name, self->mod->name);
    vprintf(fmt, args);
}

/* 
 * Append this fd to our list of fds and 
 * if module is in RUNNING state, start listening on its events 
 */
static module_ret_code _register_fd(module *mod, const int fd, const bool autoclose, const void *userptr) {
    module_poll_t *tmp = memhook._malloc(sizeof(module_poll_t));
    MOD_ALLOC_ASSERT(tmp);
    
    if (poll_set_data(&tmp->ev) == MOD_OK) {
        tmp->fd = fd;
        tmp->autoclose = autoclose;
        tmp->userptr = userptr;
        tmp->prev = mod->fds;
        tmp->self = &mod->self;
        mod->fds = tmp;
        /* If a fd is registered at runtime, start polling on it */
        int ret = 0;
        if (_module_is(mod, RUNNING)) {
            ret = poll_set_new_evt(tmp, mod->self.ctx, ADD);
        }
        return !ret ? MOD_OK : MOD_ERR;
    }
    memhook._free(tmp);
    return MOD_ERR;
}

/* Linearly search for fd */
static module_ret_code _deregister_fd(module *mod, const int fd) {
    module_poll_t **tmp = &mod->fds;
    
    int ret = 0;
    while (*tmp) {
        if ((*tmp)->fd == fd) {
            module_poll_t *t = *tmp;
            *tmp = (*tmp)->prev;
            /* If a fd is deregistered for a RUNNING module, stop polling on it */
            if (_module_is(mod, RUNNING)) {
                ret = poll_set_new_evt(t, mod->self.ctx, RM);
            }
            if (t->autoclose) {
                close(t->fd);
            }
            memhook._free(t->ev);
            memhook._free(t);
            return !ret ? MOD_OK : MOD_ERR;
        }
        tmp = &(*tmp)->prev;
    }
    return MOD_ERR;
}

static int manage_fds(module *mod, m_context *c, const int flag, const bool stop) {    
    module_poll_t *tmp = mod->fds;
    int ret = 0;
    
    while (tmp && !ret) {
        module_poll_t *t = tmp;
        tmp = tmp->prev;
        if (flag == RM && stop) {
            if (t->fd == mod->pubsub_fd[0]) {
                /*
                 * Free all unread pubsub msg for this module.
                 *
                 * Pass a !NULL pointer as first parameter,
                 * so that flush_pubsub_msgs() will free messages instead of delivering them.
                 */
                flush_pubsub_msgs(mod, NULL, mod);
            }
            ret = _deregister_fd(mod, t->fd);
        } else {
            ret = poll_set_new_evt(t, c, flag);
        }
    }
    return ret;
}

/** Private API **/

bool _module_is(const module *mod, const enum module_states st) {
    return mod->state & st;
}

map_ret_code evaluate_module(void *data, const char *key, void *value) {
    module *mod = (module *)value;
    if (_module_is(mod, IDLE) && mod->hook.evaluate()) {
        start(mod, true);
    }
    return MAP_OK;
}

module_ret_code start(module *mod, const bool start) {
    static const char *errors[2] = { "Failed to resume module.", "Failed to start module." };
    
    GET_CTX_PRIV((&mod->self));
    
    const bool was_idle = _module_is(mod, IDLE);
    
    /* 
     * Starting module for the first time
     * or after it was stopped.
     * Properly add back its pubsub fds.
     */
    if (start) {
        /* THIS IS NOT A RESUME */
        if (init_pubsub_fd(mod) != MOD_OK) {
            return MOD_ERR;
        }
    }
    
    int ret = manage_fds(mod, c, ADD, false);
    MOD_ASSERT(!ret, errors[start], MOD_ERR);
    
    mod->state = RUNNING;
    
    /* If this is first time module is started, call its init() callback */
    if (was_idle) {
        mod->hook.init();
    }
    
    MODULE_DEBUG("%s '%s'.\n", start ? "Started" : "Resumed", mod->name);
    tell_system_pubsub_msg(NULL, c, MODULE_STARTED, &mod->self, NULL);
    
    c->running_mods++;
    return MOD_OK;
}

module_ret_code stop(module *mod, const bool stop) {
    static const char *errors[2] = { "Failed to pause module.", "Failed to stop module." };
    
    GET_CTX_PRIV((&mod->self));
    
    int ret = manage_fds(mod, c, RM, stop);
    MOD_ASSERT(!ret, errors[stop], MOD_ERR);
    
    mod->state = stop ? STOPPED : PAUSED;
    /*
     * When module gets stopped, its write-end pubsub fd is closed too 
     * Read-end is already closed by manage_fds().
     */
    if (stop && mod->pubsub_fd[1] != -1) {
        close(mod->pubsub_fd[1]);
        mod->pubsub_fd[0] = -1;
        mod->pubsub_fd[1] = -1;
    }
    
    MODULE_DEBUG("%s '%s'.\n", stop ? "Stopped" : "Paused", mod->name);
    tell_system_pubsub_msg(NULL, c, MODULE_STOPPED, &mod->self, NULL);
    
    c->running_mods--;
    return MOD_OK;
}

/** Public API **/

module_ret_code module_register(const char *name, const char *ctx_name, self_t **self, const userhook *hook) {
    MOD_PARAM_ASSERT(name);
    MOD_PARAM_ASSERT(ctx_name);
    MOD_PARAM_ASSERT(self);
    MOD_PARAM_ASSERT(!*self);
    MOD_PARAM_ASSERT(hook);
    
    m_context *context = check_ctx(ctx_name);
    MOD_ASSERT(context, "Failed to create context.", MOD_ERR);
    
    const bool present = map_has_key(context->modules, name);
    MOD_ASSERT(!present, "Module with same name already registered in context.", MOD_ERR);
    
    MODULE_DEBUG("Registering module '%s'.\n", name);
    
    module *mod = memhook._calloc(1, sizeof(module));
    MOD_ALLOC_ASSERT(mod);
    
    module_ret_code ret = MOD_NO_MEM;
    /* Let us gladly jump out with break on error */
    while (true) {
        mod->name = name;
        
        memcpy(&mod->hook, hook, sizeof(userhook));
        mod->state = IDLE;
        mod->fds = NULL;
        
        mod->subscriptions = map_new(false, NULL);
        if (!mod->subscriptions) {
            break;
        }
        
        mod->recvs = stack_new(NULL);
        if (!mod->recvs) {
            break;
        }
        
        /* External handler */
        *self = memhook._malloc(sizeof(self_t));
        if (!*self) {
            break;
        }
        
        /* External owner reference */
        memcpy(*self, &((self_t){ mod, context, false }), sizeof(self_t));
        
        /* Internal reference used as sender for pubsub messages */
        memcpy(&mod->self, &((self_t){ mod, context, true }), sizeof(self_t));
        
        if (map_put(context->modules, mod->name, mod) == MAP_OK) {
            mod->pubsub_fd[0] = -1;
            mod->pubsub_fd[1] = -1;
            return MOD_OK;
        }
        ret = MOD_ERR;
        break;
    }
    memhook._free(*self);
    *self = NULL;
    map_free(mod->subscriptions);
    stack_free(mod->recvs);
    memhook._free(mod);
    return ret;
}

module_ret_code module_deregister(self_t **self) {
    MOD_PARAM_ASSERT(self);
    
    GET_MOD(*self);
    MODULE_DEBUG("Deregistering module '%s'.\n", mod->name);
    
    stop(mod, true);
    
    /* Remove the module from the context */
    map_remove((*self)->ctx->modules, mod->name);
            
    /* Remove context without modules */
    if (map_length((*self)->ctx->modules) == 0) {
        destroy_ctx((*self)->ctx);
    }
    map_free(mod->subscriptions);
    stack_free(mod->recvs);
    
    /* Destroy external handler */
    memhook._free(*self);
    *self = NULL;

    /*
     * Call destroy once self is NULL 
     * (ie: no more libmodule operations can be called on this handler) 
     */
    mod->hook.destroy();

    /* Finally free module */
    memhook._free(mod);
            
    return MOD_OK;
}

module_ret_code module_load(const char *module_path, const char *ctx_name) {
    MOD_PARAM_ASSERT(module_path);
    FIND_CTX(ctx_name);
    
    const int module_size = map_length(c->modules);

    void *handle = dlopen(module_path, RTLD_NOW);
    if (!handle) {
        MODULE_DEBUG("Dlopen failed with error: %s\n", dlerror());
        return MOD_ERR;
    }
    
    /* 
     * Check that requested module has been created in requested ctx, 
     * by looking at requested ctx number of modules
     */
    if (module_size == map_length(c->modules)) { 
        dlclose(handle);
        return MOD_ERR;
    }
    return MOD_OK;
}

module_ret_code module_unload(const char *module_path) {
    MOD_PARAM_ASSERT(module_path);    
    
    void *handle = dlopen(module_path, RTLD_NOW | RTLD_NOLOAD);
    if (handle) {
        /* RTLD_NOLOAD does still increment refcounter... */
        dlclose(handle);
        dlclose(handle);
        return MOD_OK;
    }
    MODULE_DEBUG("Dlopen failed with error: %s\n", dlerror());
    return MOD_ERR;
}

module_ret_code module_become(const self_t *self, const recv_cb new_recv) {
    MOD_PARAM_ASSERT(new_recv);
    GET_MOD_IN_STATE(self, RUNNING);
    
    if (stack_push(mod->recvs, new_recv) == STACK_OK) {
        return MOD_OK;
    }
    return MOD_ERR;
}

module_ret_code module_unbecome(const self_t *self) {
    GET_MOD_IN_STATE(self, RUNNING);
    
    if (stack_pop(mod->recvs) != NULL) {
        return MOD_OK;
    }
    return MOD_ERR;
}

module_ret_code module_log(const self_t *self, const char *fmt, ...) {
    GET_MOD(self);
    GET_CTX(self);
    
    va_list args;
    va_start(args, fmt);
    c->logger(self, fmt, args, mod->userdata);
    va_end(args);
    return MOD_OK;
}

module_ret_code module_set_userdata(const self_t *self, const void *userdata) {
    GET_MOD(self);
    
    mod->userdata = userdata;
    return MOD_OK;
}

module_ret_code module_register_fd(const self_t *self, const int fd, const bool autoclose, const void *userptr) {
    MOD_PARAM_ASSERT(fd >= 0);
    GET_MOD(self);

    return _register_fd(mod, fd, autoclose, userptr);
}

module_ret_code module_deregister_fd(const self_t *self, const int fd) {
    MOD_PARAM_ASSERT(fd >= 0);
    GET_MOD(self);
    MOD_ASSERT(mod->fds, "No fd registered in this module.", MOD_ERR);
    
    return _deregister_fd(mod, fd);
}

module_ret_code module_get_name(const self_t *self, char **name) {
    GET_MOD_PURE(self);
    
    *name = mem_strdup(mod->name);
    MOD_ALLOC_ASSERT(*name);
    return MOD_OK;
}

module_ret_code module_get_context(const self_t *self, char **ctx) {
    GET_CTX_PURE(self);
    
    *ctx = mem_strdup(c->name);
    MOD_ALLOC_ASSERT(*ctx);
    return MOD_OK;
}

/** Module state getters **/

bool module_is(const self_t *self, const enum module_states st) {
    GET_MOD_PURE(self);

    return _module_is(mod, st);
}

module_ret_code module_dump(const self_t *self) {
    GET_MOD(self);

    module_log(self, "* State: %u\n", mod->state);
    module_log(self, "* Userdata: %p\n", mod->userdata);
    module_log(self, "* Subscriptions:\n");
    for (map_itr_t *itr = map_itr_new(mod->subscriptions); itr; itr = map_itr_next(itr)) {
        module_log(self, "-> %s: %p\n", map_itr_get_key(itr), map_itr_get_data(itr));
    }
    module_log(self, "* Fds:\n");
    module_poll_t *tmp = mod->fds;
    while (tmp) {
        if (tmp->fd != mod->pubsub_fd[0]) {
            module_log(self, "-> Fd: %d Ac: %d Up: %p\n", tmp->fd, tmp->autoclose, tmp->userptr);
        }
        tmp = tmp->prev;
    }
    return MOD_OK;
}

/** Module state setters **/

module_ret_code module_start(const self_t *self) {
    GET_MOD_IN_STATE(self, IDLE | STOPPED);
    
    return start(mod, true);
}

module_ret_code module_pause(const self_t *self) {
    GET_MOD_IN_STATE(self, RUNNING);
    
    return stop(mod, false);
}

module_ret_code module_resume(const self_t *self) {
    GET_MOD_IN_STATE(self, PAUSED);
    
    return start(mod, false);
}

module_ret_code module_stop(const self_t *self) {
    GET_MOD_IN_STATE(self, RUNNING | PAUSED);
    
    return stop(mod, true);
}
