#include <module.h> 
#include <module_priv.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

static module_ret_code init_ctx(const char *ctx_name, m_context **context);
static void destroy_ctx(const char *ctx_name, m_context *context);
static m_context *check_ctx(const char *ctx_name);
static module_ret_code add_children(module *mod, const self_t *self);
static void default_logger(const char *module_name, const char *context_name, 
                           const char *fmt, va_list args, const void *userdata);
static module_ret_code add_subscription(module *mod, const char *topic);
static int tell_if(void *data, void *m);
static pubsub_msg_t *create_pubsub_msg(const char *message, const char *sender, const char *topic);
static module_ret_code tell_pubsub_msg(pubsub_msg_t *m, module *mod, m_context *c);
static int manage_fds(module *mod, m_context *c, int type, int stop);
static module_ret_code start_children(module *m);
static module_ret_code stop_children(module *m);

static module_ret_code init_ctx(const char *ctx_name, m_context **context) {
    MODULE_DEBUG("Creating context '%s'.\n", ctx_name);
    
    *context = malloc(sizeof(m_context));
    MOD_ASSERT(*context, "Failed to malloc.", MOD_ERR);
    
    **context = (m_context) {0};
         
    (*context)->epollfd = epoll_create1(EPOLL_CLOEXEC); 
    MOD_ASSERT(((*context)->epollfd >= 0), "Failed to create epollfd.", MOD_ERR);
     
    (*context)->logger = default_logger;
    
    (*context)->modules = hashmap_new();
    if ((*context)->modules && hashmap_put(ctx, (char *)ctx_name, *context) == MAP_OK) {
        return MOD_OK;
    }
    
    destroy_ctx(ctx_name, *context);
    *context = NULL;
    return MOD_ERR;
}

static void destroy_ctx(const char *ctx_name, m_context *context) {
    MODULE_DEBUG("Destroying context '%s'.\n", ctx_name);
    hashmap_free(context->modules);
    close(context->epollfd);
    free(context);
    hashmap_remove(ctx, (char *)ctx_name);
}

static m_context *check_ctx(const char *ctx_name) {
    m_context *context = NULL;
    hashmap_get(ctx, (char *)ctx_name, (void **)&context); \
    if (!context) {
        init_ctx(ctx_name, &context);
    }
    return context;
}

module_ret_code module_register(const char *name, const char *ctx_name, const self_t **self, userhook *hook) {
    MOD_ASSERT(name, "NULL module name.", MOD_ERR);
    MOD_ASSERT(ctx_name, "NULL context name.", MOD_ERR);
    
    m_context *context = check_ctx(ctx_name);
    MOD_ASSERT(context, "Failed to create context.", MOD_ERR);
    
    module *mod = NULL;
    hashmap_get(context->modules, (char *)name, (void **)&mod);
    MOD_ASSERT(!mod, "Module already registered in context.", MOD_ERR);
    
    MODULE_DEBUG("Registering module %s.\n", name);
    
    mod = malloc(sizeof(module));
    MOD_ASSERT(mod, "Failed to malloc.", MOD_ERR);
    
    *mod = (module) {0};
    if (hashmap_put(context->modules, (char *)name, mod) == MAP_OK) {        
        mod->hook = hook;
        mod->state = IDLE;
        mod->self.name = strdup(name);
        mod->self.ctx = strdup(ctx_name);
        mod->fds = NULL;
        mod->subscriptions = hashmap_new();
        *self = &mod->self;
        evaluate_module(NULL, mod);
        return MOD_OK;
    }
    free(mod);
    return MOD_ERR;
}

module_ret_code module_deregister(const self_t **self) {
    self_t *tmp = (self_t *) *self;
    
    GET_MOD(tmp);
    
    MODULE_DEBUG("Deregistering module %s.\n", tmp->name);
        
    module_stop(tmp);
        
    mod->hook->destroy();
    /* Remove the module from the context */
    hashmap_remove(c->modules, (char *)tmp->name);
    /* Remove context without modules */
    if (hashmap_length(c->modules) == 0) {
        destroy_ctx(tmp->ctx, c);
    }
    
    hashmap_free(mod->subscriptions);
    free((void *)tmp->name);
    free((void *)tmp->ctx);
    tmp = NULL;
    free(mod);
    return MOD_OK;
}

static module_ret_code add_children(module *mod, const self_t *self) {
    child_t **tmp = &mod->children;
    while (*tmp) {
        tmp = &(*tmp)->next;
    }
    *tmp = malloc(sizeof(child_t));
    if (*tmp) {
        (*tmp)->self = self;
        (*tmp)->next = NULL;
        return MOD_OK;
    }
    return MOD_ERR;
}

module_ret_code module_binds_to(const self_t *self, const char *parent) {
    GET_CTX(self->ctx);
    
    module *mod = NULL;
    hashmap_get(c->modules, (char *)parent, (void **)&mod);
    MOD_ASSERT(mod, "Parent module not found.", MOD_NO_PARENT);
    
    return add_children(mod, self);
}

static void default_logger(const char *module_name, const char *context_name, 
                           const char *fmt, va_list args, const void *userdata) {
    printf("[%s]|%s|: ", context_name, module_name);
    vprintf(fmt, args);
}

int evaluate_module(void *data, void *m) {
    module *mod = (module *)m;
    if (module_is(&mod->self, IDLE) 
        && mod->hook->evaluate()) {
            
        mod->hook->init();
        module_start(&mod->self);
    }
    return MAP_OK;
}

module_ret_code module_become(const self_t *self,  recv_cb new_recv) {    
    GET_MOD_IN_STATE(self, RUNNING);
    
    mod->hook->recv = new_recv;
    return MOD_OK;
}

module_ret_code module_log(const self_t *self, const char *fmt, ...) {
    GET_MOD(self);
    
    va_list args;
    va_start(args, fmt);
    c->logger(self->name, self->ctx, fmt, args, mod->userdata);
    va_end(args);
    return MOD_OK;
}

module_ret_code module_set_userdata(const self_t *self, const void *userdata) {    
    GET_MOD(self);
    
    mod->userdata = userdata;
    return MOD_OK;
}

module_ret_code module_add_fd(const self_t *self, int fd) {
    GET_MOD(self);
    MOD_ASSERT(c->num_fds < MAX_EVENTS, "Reached max number of events for this context.", MOD_ERR);
    
    module_poll_t *tmp = malloc(sizeof(module_poll_t));
    MOD_ASSERT(tmp, "Failed to malloc.", MOD_ERR);
       
    tmp->fd = fd;
    tmp->ev.data.ptr = (void *)tmp;
    tmp->ev.events = EPOLLIN;
    tmp->prev = mod->fds;
    tmp->self = (void *)self;
    mod->fds = tmp;
    c->num_fds++;
    
    /* If a fd is added at runtime, start polling on it */
    if (module_is(self, RUNNING)) {
        if (!epoll_ctl(c->epollfd, EPOLL_CTL_ADD, tmp->fd, &tmp->ev)) {
            return MOD_OK;
        }
        return MOD_ERR;
    }
    return MOD_OK;
}

module_ret_code module_rm_fd(const self_t *self, int fd, int close_fd) {
    GET_MOD_IN_STATE(self, RUNNING);
    
    MOD_ASSERT(mod->fds, "No fd registered in this module.", MOD_ERR);
    module_poll_t **tmp = &mod->fds;
    
    while (*tmp) {
        if ((*tmp)->fd == fd) {
            module_poll_t *t = *tmp;
            *tmp = (*tmp)->prev;
            if (close_fd) {
                close(t->fd);
            }
            free(t);
            c->num_fds--;
            return MOD_OK;
        }
        tmp = &(*tmp)->prev;
    }
    return MOD_ERR;
}

module_ret_code module_update_fd(const self_t *self, int old_fd, int new_fd, int close_old) {    
    if (module_rm_fd(self, old_fd, close_old) == MOD_OK) {
        return module_add_fd(self, new_fd);
    }
    return MOD_ERR;
}

/** Actor-like PubSub interface **/

static module_ret_code add_subscription(module *mod, const char *topic) {
    void *tmp = NULL;
    if (hashmap_get(mod->subscriptions, (char *)topic, (void **)&tmp) == MAP_MISSING) {
        /* Store pointer to mod as value, even if it will be unused; this should be a hashset */
        if (hashmap_put(mod->subscriptions, (char *)topic, mod) == MAP_OK) {
            return MOD_OK;
        }
    }
    return MOD_ERR;
}

module_ret_code module_subscribe(const self_t *self, const char *topic) {
    MOD_ASSERT(topic, "NULL topic.", MOD_ERR);
    GET_MOD(self);
    
    return add_subscription(mod, topic);
}

static int tell_if(void *data, void *m) {
    module *mod = (module *)m;
    const msg_t *msg = (msg_t *)data;
    void *tmp = NULL;

    /* 
     * Only if mod is actually running and 
     * if topic is null or this module is subscribed to topic 
     */
    if (module_is(&mod->self, RUNNING) && (!msg->msg->topic || 
        hashmap_get(mod->subscriptions, (char *)msg->msg->topic, (void **)&tmp) == MAP_OK)) {
        
        MODULE_DEBUG("Telling a message to %s.\n", mod->self.name);
        mod->hook->recv(msg, mod->userdata);
    }
    return MAP_OK;
}

static pubsub_msg_t *create_pubsub_msg(const char *message, const char *sender, const char *topic) {
    pubsub_msg_t *m = malloc(sizeof(pubsub_msg_t));
    if (m) {
        m->message = message;
        m->sender = sender;
        m->topic = topic;
    }
    return m;
}

static module_ret_code tell_pubsub_msg(pubsub_msg_t *m, module *mod, m_context *c) {
    if (m) {
        msg_t msg = { -1, m };
        if (mod) {
            tell_if(&msg, mod);
        } else if (c) {
            hashmap_iterate(c->modules, tell_if, &msg);
        }
        free(m);
        return MOD_OK;
    }
    return MOD_ERR;
}

module_ret_code module_tell(const self_t *self, const char *recipient, const char *message) {
    MOD_ASSERT(message, "NULL message.", MOD_ERR);
    MOD_ASSERT(message, "NULL recipient.", MOD_ERR);
    
    GET_CTX(self->ctx);
    CTX_GET_MOD(recipient, c);

    pubsub_msg_t *m = create_pubsub_msg(message, self->name, NULL);
    return tell_pubsub_msg(m, mod, NULL);
}

module_ret_code module_publish(const self_t *self, const char *topic, const char *message) {
    MOD_ASSERT(message, "NULL message.", MOD_ERR);
    
    GET_CTX(self->ctx);
    
    pubsub_msg_t *m = create_pubsub_msg(message, self->name, topic);
    return tell_pubsub_msg(m, NULL, c);
}

/** Module state getters **/

/* Define states getters */
int module_is(const self_t *self, const enum module_states st) {
    GET_MOD(self);
    
    return mod->state & st;
}

/** Module state setters **/

static int manage_fds(module *mod, m_context *c, int type, int stop) {
    module_poll_t *tmp = mod->fds, *t = NULL;
    int ret = 0;
    
    while (tmp && !ret) {
        if (stop) {
            ret = close(tmp->fd);
            t = tmp;
        } else {
            ret = epoll_ctl(c->epollfd, type, tmp->fd, &tmp->ev);
        }
        tmp = tmp->prev;
        free(t); // only used when called with stop: properly free module_poll_t
    }
    return ret;
}

module_ret_code module_start(const self_t *self) {
    GET_MOD_IN_STATE(self, IDLE);
    
    MODULE_DEBUG("Starting module %s.\n", self->name);
    return module_resume(self);
}

module_ret_code module_pause(const self_t *self) {
    GET_MOD_IN_STATE(self, RUNNING);
    
    if (!manage_fds(mod, c, EPOLL_CTL_DEL, 0)) {
        mod->state = PAUSED;
        return MOD_OK;
    }
    return MOD_ERR;
}

module_ret_code module_resume(const self_t *self) {
    GET_MOD_IN_STATE(self, IDLE | PAUSED);
    
    if (!manage_fds(mod, c, EPOLL_CTL_ADD, 0)) {
        mod->state = RUNNING;
        return MOD_OK;
    }
    return MOD_ERR;
}

module_ret_code module_stop(const self_t *self) {
    GET_MOD_IN_STATE(self, RUNNING);
    
    MODULE_DEBUG("Stopping module %s.\n", self->name);
    if (!manage_fds(mod, c, -1, 1)) { // implicitly calls EPOLL_CTL_DEL
        mod->state = STOPPED;
        return MOD_OK;
    }
    return MOD_ERR;
}

static module_ret_code start_children(module *m) {
    CHILDREN_LOOP({
        mod->hook->init();
        module_start(&mod->self);
    });
    return MOD_OK;
}

static module_ret_code stop_children(module *m) {
    CHILDREN_LOOP({
        module_stop(&mod->self);
    });
    return MOD_OK;
}
