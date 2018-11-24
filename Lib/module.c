#include "module.h"
#include "poll_priv.h"

static module_ret_code init_ctx(const char *ctx_name, m_context **context);
static void destroy_ctx(m_context *context);
static m_context *check_ctx(const char *ctx_name);
static int _pipe(module *mod);
static module_ret_code init_pubsub_fd(const self_t *self);
static void default_logger(const self_t *self, const char *fmt, va_list args, const void *userdata);
static int tell_if(void *data, void *m);
static pubsub_msg_t *create_pubsub_msg(const unsigned char *message, const self_t *sender, const char *topic, enum msg_type type, const size_t size);
static module_ret_code tell_pubsub_msg(pubsub_msg_t *m, module *mod, m_context *c);
static module_ret_code publish_msg(const self_t *self, const char *topic, 
                                   const unsigned char *message, const ssize_t size);
static int manage_fds(const self_t *self, m_context *c, int flag, int stop);
static module_ret_code start(const self_t *self, const enum module_states mask, const char *err_str);
static module_ret_code stop(const self_t *self, const enum module_states mask, const char *err_str, int stop);

static module_ret_code init_ctx(const char *ctx_name, m_context **context) {
    MODULE_DEBUG("Creating context '%s'.\n", ctx_name);
    
    *context = memhook._malloc(sizeof(m_context));
    MOD_ALLOC_ASSERT(*context);
    
    **context = (m_context) {0};
    
    (*context)->fd = poll_create();
    MOD_ASSERT((*context)->fd >= 0, "Failed to create context fd.", MOD_ERR);
     
    (*context)->logger = default_logger;
    
    (*context)->modules = map_new();
    (*context)->topics = map_new();
    (*context)->name = mem_strdup(ctx_name);
    if ((*context)->topics && (*context)->modules && map_put(ctx, (*context)->name, *context, false) == MAP_OK) {
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
    memhook._free((char *)context->name);
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

static module_ret_code init_pubsub_fd(const self_t *self) {
    GET_MOD(self);
    if (_pipe(mod) == 0) {
        if (module_register_fd(self, mod->pubsub_fd[0], true, NULL) == MOD_OK) {
            return MOD_OK;
        }
        close(mod->pubsub_fd[0]);
        close(mod->pubsub_fd[1]);
        mod->pubsub_fd[0] = -1;
        mod->pubsub_fd[1] = -1;
    }
    return MOD_ERR;
}

module_ret_code module_register(const char *name, const char *ctx_name, const self_t **self, const userhook *hook) {
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
    
    module *mod = memhook._malloc(sizeof(module));
    MOD_ALLOC_ASSERT(mod);
    
    *mod = (module) {{ 0 }};
    mod->name = mem_strdup(name);
    if (map_put(context->modules, mod->name, mod, false) == MAP_OK) {
        mod->hook = *hook;
        mod->state = IDLE;
        mod->fds = NULL;
        mod->subscriptions = map_new();
        mod->recvs = stack_new();
        *self = memhook._malloc(sizeof(self_t));
        self_t *s = (self_t *)*self;
        *((module **)&s->mod) = mod;
        *((m_context **)&s->ctx) = context;
        mod->self = s;
        evaluate_module(NULL, mod);
        return MOD_OK;
    }
    memhook._free(mod);
    return MOD_ERR;
}

module_ret_code module_deregister(const self_t **self) {
    MOD_PARAM_ASSERT(self);
    
    self_t *tmp = (self_t *) *self;
    GET_MOD(tmp);
    MODULE_DEBUG("Deregistering module '%s'.\n", mod->name);
    
    /* Free all unread pubsub msg for this module */
    flush_pubsub_msg(tmp, mod);
    
    stop(*self, RUNNING | IDLE | PAUSED | STOPPED, "Failed to stop module.", 1);
    
    /* Remove the module from the context */
    map_remove(tmp->ctx->modules, mod->name);
    /* Remove context without modules */
    if (map_length(tmp->ctx->modules) == 0) {
        destroy_ctx(tmp->ctx);
    }
    map_free(mod->subscriptions);
    stack_free(mod->recvs);
    memhook._free((self_t *)*self);
    *self = NULL;
    mod->self = NULL;

    /*
     * Call destroy once self is NULL 
     * (ie: no more libmodule operations can be called on this handler) 
     */
    mod->hook.destroy();

    /* Finally free module */
    memhook._free((char *)mod->name);
    memhook._free(mod);
    
    return MOD_OK;
}

static void default_logger(const self_t *self, const char *fmt, va_list args, const void *userdata) {
    printf("[%s]|%s|: ", self->ctx->name, self->mod->name);
    vprintf(fmt, args);
}

int evaluate_module(void *data, void *m) {
    module *mod = (module *)m;
    if (module_is(mod->self, IDLE)
        && mod->hook.evaluate()) {
        
        mod->hook.init();
        module_start(mod->self);
    }
    return MAP_OK;
}

module_ret_code module_become(const self_t *self, const recv_cb new_recv) {
    MOD_PARAM_ASSERT(new_recv);
    GET_MOD_IN_STATE(self, RUNNING);
    
    if (stack_push(mod->recvs, new_recv, false) == STACK_OK) {
        return MOD_OK;
    }
    return MOD_ERR;
}

module_ret_code module_unbecome(const self_t *self) {
    GET_MOD_IN_STATE(self, RUNNING);
    
    if (stack_pop(mod->recvs) == STACK_OK) {
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

/* 
 * Append this fd to our list of fds and 
 * if module is in RUNNING state, start listening on its events 
 */
module_ret_code module_register_fd(const self_t *self, const int fd, const bool autoclose, const void *userptr) {
    MOD_PARAM_ASSERT(fd >= 0);
    
    GET_MOD(self);
    GET_CTX(self);

    module_poll_t *tmp = memhook._malloc(sizeof(module_poll_t));
    MOD_ALLOC_ASSERT(tmp);
      
    if (poll_set_data(&tmp->ev) == MOD_OK) {
        tmp->fd = fd;
        tmp->autoclose = autoclose;
        tmp->userptr = userptr;
        tmp->prev = mod->fds;
        tmp->self = (self_t *)self;
        mod->fds = tmp;
        c->num_fds++;
        /* If a fd is registered at runtime, start polling on it */
        int ret = 0;
        if (module_is(self, RUNNING)) {
            ret = poll_set_new_evt(tmp, c, ADD);
        }
        return !ret ? MOD_OK : MOD_ERR;
    }
    memhook._free(tmp);
    return MOD_ERR;
}

/* Linearly searching for fd */
module_ret_code module_deregister_fd(const self_t *self, const int fd) {
    MOD_PARAM_ASSERT(fd >= 0);
    
    GET_MOD(self);
    GET_CTX(self);
    MOD_ASSERT(mod->fds, "No fd registered in this module.", MOD_ERR);
    
    module_poll_t **tmp = &mod->fds;
    
    int ret = 0;
    while (*tmp) {
        if ((*tmp)->fd == fd) {
            module_poll_t *t = *tmp;
            *tmp = (*tmp)->prev;
            /* If a fd is deregistered for a RUNNING module, stop polling on it */
            if (module_is(self, RUNNING)) {
                ret = poll_set_new_evt(t, c, RM);
            }
            if (t->autoclose) {
                close(t->fd);
            }
            memhook._free(t->ev);
            memhook._free(t);
            c->num_fds--;
            return !ret ? MOD_OK : MOD_ERR;
        }
        tmp = &(*tmp)->prev;
    }
    return MOD_ERR;
}

module_ret_code module_get_name(const self_t *self, char **name) {
    GET_MOD(self);
    *name = mem_strdup(mod->name);
    return MOD_OK;
}

module_ret_code module_get_context(const self_t *self, char **ctx) {
    GET_CTX(self);
    *ctx = mem_strdup(c->name);
    return MOD_OK;
}

char *mem_strdup(const char *s) {
    char *new = NULL;
    if (s) {
        const int len = strlen(s) + 1;
        new = memhook._malloc(len);
        if (new) {
            memcpy(new, s, len);
        }
    }
    return new;
}

/** Actor-like PubSub interface **/

module_ret_code module_register_topic(const self_t *self, const char *topic) {
    MOD_PARAM_ASSERT(topic);
    GET_MOD(self);
    GET_CTX(self);
    
    if (!map_has_key(c->topics, topic)) {
        if (map_put(c->topics, topic, mod, true) == MAP_OK) {
            tell_system_pubsub_msg(c, TOPIC_REGISTERED, topic);
            return MOD_OK;
        }
    }
    return MOD_ERR;
}

module_ret_code module_deregister_topic(const self_t *self, const char *topic) {
    MOD_PARAM_ASSERT(topic);
    GET_MOD(self);
    GET_CTX(self);
    void *tmp = map_get(c->topics, topic); // NULL if key is not present
    
    /* Only same mod which registered topic can deregister it */
    if (tmp == mod) {
        if (map_remove(c->topics, topic) == MAP_OK) {
            tell_system_pubsub_msg(c, TOPIC_DEREGISTERED, topic);
            return MOD_OK;
        }
    }
    return MOD_ERR;
}

module_ret_code module_subscribe(const self_t *self, const char *topic) {
    MOD_PARAM_ASSERT(topic);
    GET_MOD(self);
    GET_CTX(self);
    
    /* If topic exists and we are not already subscribed */
    if (map_has_key(c->topics, topic) && !map_has_key(mod->subscriptions, topic)) {
        /* Store pointer to mod as value, even if it will be unused; this should be a hashset */
        if (map_put(mod->subscriptions, topic, mod, true) == MAP_OK) {
            return MOD_OK;
        }
    }
    return MOD_ERR;
}

module_ret_code module_unsubscribe(const self_t *self, const char *topic) {
    MOD_PARAM_ASSERT(topic);
    GET_MOD(self);
    
    if (map_remove(mod->subscriptions, topic) == MAP_OK) {
        return MOD_OK;
    }
    return MOD_ERR;
}

module_ret_code tell_system_pubsub_msg(m_context *c, enum msg_type type, const char *topic) {
    pubsub_msg_t m = { .topic = topic, .sender = NULL, .message = NULL, .type = type, .size = 0 };
    return tell_pubsub_msg(&m, NULL, c);
}

int flush_pubsub_msg(void *data, void *m) {
    module *mod = (module *)m;
    pubsub_msg_t *mm = NULL;
    
    while (read(mod->pubsub_fd[0], &mm, sizeof(pubsub_msg_t *)) == sizeof(pubsub_msg_t *)) {
        /* 
         * Actually tell msg ONLY if we are not deregistering module, 
         * ie: we are stopping looping on the context. 
         */
        if (!data) {
            MODULE_DEBUG("Flushing pubsub message for module '%s'.\n", mod->name);
            const msg_t msg = { .is_pubsub = 1, .pubsub_msg = mm };
            mod->hook.recv(&msg, mod->userdata);
        }
        destroy_pubsub_msg(mm);
    }
    return 0;
}

static int tell_if(void *data, void *m) {
    module *mod = (module *)m;
    const pubsub_msg_t *msg = (pubsub_msg_t *)data;

    /* 
     * Only if mod is actually running or paused and 
     * it is a SYSTEM message or
     * topic is null or this module is subscribed to topic 
     */
    if (module_is(mod->self, RUNNING | PAUSED) && (msg->type != USER || !msg->topic || 
        map_has_key(mod->subscriptions, msg->topic))) {
        
        MODULE_DEBUG("Telling a message to %s.\n", mod->name);
        
        pubsub_msg_t *mm = create_pubsub_msg(msg->message, msg->sender, msg->topic, msg->type, msg->size);
        write(mod->pubsub_fd[1], &mm, sizeof(pubsub_msg_t *));
    }
    return MAP_OK;
}

static pubsub_msg_t *create_pubsub_msg(const unsigned char *message, const self_t *sender, const char *topic, 
                                       enum msg_type type, const size_t size) {
    pubsub_msg_t *m = memhook._malloc(sizeof(pubsub_msg_t));
    if (m) {
        m->message = message;
        m->sender = sender;
        m->topic = mem_strdup(topic);
        *(int *)&m->type = type;
        *(size_t *)&m->size = size;
    }
    return m;
}

void destroy_pubsub_msg(pubsub_msg_t *m) {
    memhook._free((char *)m->topic);
    memhook._free(m);
}

static module_ret_code tell_pubsub_msg(pubsub_msg_t *m, module *mod, m_context *c) {
    if (mod) {
        tell_if(m, mod);
    } else {
        map_iterate(c->modules, tell_if, m);
    }
    return MOD_OK;
}

module_ret_code module_tell(const self_t *self, const char *recipient, const unsigned char *message, 
                            const ssize_t size) {
    MOD_PARAM_ASSERT(self);
    MOD_PARAM_ASSERT(message);
    MOD_PARAM_ASSERT(size > 0);
    MOD_PARAM_ASSERT(recipient);
    
    GET_CTX(self);
    CTX_GET_MOD(recipient, c);

    pubsub_msg_t m = { .topic = NULL, .message = message, .sender = self, .type = USER, .size = size };
    return tell_pubsub_msg(&m, mod, c);
}

module_ret_code module_reply(const self_t *self, const self_t *sender, const unsigned char *message, const ssize_t size) {
    return module_tell(self, sender->mod->name, message, size);
}

static module_ret_code publish_msg(const self_t *self, const char *topic, 
                                   const unsigned char *message, const ssize_t size) {
    MOD_PARAM_ASSERT(message);
    MOD_PARAM_ASSERT(size > 0);

    GET_MOD(self);
    GET_CTX(self);

    void *tmp = NULL;
    /* 
     * Only module that registered a topic can publish on the topic.
     * Moreover, a publish can only be made on existent topic.
     */
    if (!topic || ((tmp = map_get(c->topics, topic)) && tmp == mod)) {
        pubsub_msg_t m = { .topic = topic, .message = message, .sender = self, .type = USER, .size = size };
        return tell_pubsub_msg(&m, NULL, c);
    }
    return MOD_ERR;
}

module_ret_code module_publish(const self_t *self, const char *topic, const unsigned char *message, const ssize_t size) {
    MOD_PARAM_ASSERT(topic);

    return publish_msg(self, topic, message, size);
}

module_ret_code module_broadcast(const self_t *self, const unsigned char *message, const ssize_t size) {
    return publish_msg(self, NULL, message, size);
}

/** Module state getters **/

/* Define states getters */
bool module_is(const self_t *self, const enum module_states st) {
    GET_MOD(self);
    
    return mod->state & st;
}

/** Module state setters **/

static int manage_fds(const self_t *self, m_context *c, int flag, int stop) {
    GET_MOD(self);
    
    module_poll_t *tmp = mod->fds;
    int ret = 0;
    
    while (tmp && !ret) {
        module_poll_t *t = tmp;
        tmp = tmp->prev;
        if (flag == RM && stop) {
            ret = module_deregister_fd(self, t->fd);
        } else {
            ret = poll_set_new_evt(t, c, flag);
        }
    }
    return ret;
}

static module_ret_code start(const self_t *self, const enum module_states mask, const char *err_str) {
    GET_MOD_IN_STATE(self, mask);
    GET_CTX(self);

    /* 
     * Starting module for the first time
     * or after it was stopped.
     * Properly add back its pubsub fds
     */
    if (mask & IDLE) {
        if (init_pubsub_fd(self) != MOD_OK) {
            return MOD_ERR;
        }
    }

    int ret = manage_fds(self, c, ADD, 0);
    MOD_ASSERT(!ret, err_str, MOD_ERR);

    mod->state = RUNNING;
    return MOD_OK;
}

static module_ret_code stop(const self_t *self, const enum module_states mask, const char *err_str, int stop) {
    GET_MOD_IN_STATE(self, mask);
    GET_CTX(self);
    
    int ret = manage_fds(self, c, RM, stop);
    MOD_ASSERT(!ret, err_str, MOD_ERR);
    
    mod->state = stop ? STOPPED : PAUSED;
    /*
     * When module gets stopped, its write-end pubsub fd is closed too 
     * Read-end is already closed by stop().
     */
    if (stop && mod->pubsub_fd[1] != -1) {
        close(mod->pubsub_fd[1]);
        mod->pubsub_fd[0] = -1;
        mod->pubsub_fd[1] = -1;
    }
    return MOD_OK;
}

module_ret_code module_start(const self_t *self) {
    return start(self, IDLE | STOPPED, "Failed to start module.");
}

module_ret_code module_pause(const self_t *self) {
    return stop(self, RUNNING, "Failed to pause module.", 0);
}

module_ret_code module_resume(const self_t *self) {
    return start(self, PAUSED, "Failed to resume module.");
}

module_ret_code module_stop(const self_t *self) {
    return stop(self, RUNNING | PAUSED, "Failed to stop module.", 1);
}
