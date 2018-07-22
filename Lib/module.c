#include "module.h"
#include "poll_priv.h"
#include <string.h>
#include <stdarg.h>

static module_ret_code init_ctx(const char *ctx_name, m_context **context);
static void destroy_ctx(const char *ctx_name, m_context *context);
static m_context *check_ctx(const char *ctx_name);
static module_ret_code init_pubsub_fd(module *mod);
static void default_logger(const self_t *self, const char *fmt, va_list args, const void *userdata);
static int tell_if(void *data, void *m);
static pubsub_msg_t *create_pubsub_msg(const char *message, const self_t *sender, const char *topic, enum msg_type type);
static module_ret_code tell_pubsub_msg(pubsub_msg_t *m, module *mod, m_context *c);
static int manage_fds(module *mod, m_context *c, int flag, int stop);
static module_ret_code start(const self_t *self, const enum module_states mask, const char *err_str);
static module_ret_code stop(const self_t *self, const enum module_states mask, const char *err_str, int stop);

static module_ret_code init_ctx(const char *ctx_name, m_context **context) {
    MODULE_DEBUG("Creating context '%s'.\n", ctx_name);
    
    *context = memhook._malloc(sizeof(m_context));
    MOD_ASSERT(*context, "Failed to malloc.", MOD_ERR);
    
    **context = (m_context) {0};
    
    (*context)->fd = poll_create();
    MOD_ASSERT(((*context)->fd >= 0), "Failed to create context fd.", MOD_ERR);
     
    (*context)->logger = default_logger;
    
    (*context)->modules = hashmap_new();
    (*context)->topics = hashmap_new();
    if ((*context)->topics && (*context)->modules && hashmap_put(ctx, (char *)ctx_name, *context) == MAP_OK) {
        return MOD_OK;
    }
    
    destroy_ctx(ctx_name, *context);
    *context = NULL;
    return MOD_ERR;
}

static void destroy_ctx(const char *ctx_name, m_context *context) {
    MODULE_DEBUG("Destroying context '%s'.\n", ctx_name);
    hashmap_free(context->modules);
    hashmap_free(context->topics);
    poll_close(context->fd, &context->pevents, &context->max_events);
    memhook._free(context);
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

static module_ret_code init_pubsub_fd(module *mod) {
    if (_pipe(mod->pubsub_fd) == 0) {
        if (module_add_fd(&mod->self, mod->pubsub_fd[0]) == MOD_OK) {
            return MOD_OK;
        }
        close(mod->pubsub_fd[0]);
        close(mod->pubsub_fd[1]);
    }
    return MOD_ERR;
}

module_ret_code module_register(const char *name, const char *ctx_name, const self_t **self, userhook *hook) {
    MOD_ASSERT(name, "NULL module name.", MOD_ERR);
    MOD_ASSERT(ctx_name, "NULL context name.", MOD_ERR);
    MOD_ASSERT(self, "NULL self double pointer.", MOD_ERR);
    MOD_ASSERT((!*self), "Self pointer to already registered module.", MOD_ERR);
    MOD_ASSERT(hook, "NULL userhook.", MOD_ERR);
    
    m_context *context = check_ctx(ctx_name);
    MOD_ASSERT(context, "Failed to create context.", MOD_ERR);
    
    module *mod = NULL;
    hashmap_get(context->modules, (char *)name, (void **)&mod);
    MOD_ASSERT(!mod, "Module with same name already registered in context.", MOD_ERR);
    
    MODULE_DEBUG("Registering module '%s'.\n", name);
    
    mod = memhook._malloc(sizeof(module));
    MOD_ASSERT(mod, "Failed to malloc.", MOD_ERR);
    
    *mod = (module) {{ 0 }};
    if (hashmap_put(context->modules, (char *)name, mod) == MAP_OK) {
        mod->hook = *hook;
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
    MOD_ASSERT(self, "NULL self double pointer.", MOD_ERR);
    
    self_t *tmp = (self_t *) *self;
    GET_MOD(tmp);
    
    MODULE_DEBUG("Deregistering module '%s'.\n", tmp->name);
    
    module_stop(tmp);
    mod->hook.destroy();
    /* Remove the module from the context */
    hashmap_remove(c->modules, (char *)tmp->name);
    /* Remove context without modules */
    if (hashmap_length(c->modules) == 0) {
        destroy_ctx(tmp->ctx, c);
    }
    hashmap_free(mod->subscriptions);
    free((void *)tmp->name);
    free((void *)tmp->ctx);
    *self = NULL;
    free(mod);
    return MOD_OK;
}

static void default_logger(const self_t *self, const char *fmt, va_list args, const void *userdata) {
    printf("[%s]|%s|: ", self->ctx, self->name);
    vprintf(fmt, args);
}

int evaluate_module(void *data, void *m) {
    module *mod = (module *)m;
    if (module_is(&mod->self, IDLE) 
        && mod->hook.evaluate()) {
            
        mod->hook.init();
        module_start(&mod->self);
    }
    return MAP_OK;
}

module_ret_code module_become(const self_t *self,  recv_cb new_recv) {
    MOD_ASSERT(new_recv, "Null recv callback.", MOD_ERR);
    GET_MOD_IN_STATE(self, RUNNING);
    
    mod->hook.recv = new_recv;
    return MOD_OK;
}

module_ret_code module_log(const self_t *self, const char *fmt, ...) {
    GET_MOD(self);
    
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
module_ret_code module_add_fd(const self_t *self, int fd) {
    /* Cannot add a fd for STOPPED modules */
    GET_MOD_IN_STATE(self, IDLE | RUNNING | PAUSED);
    MOD_ASSERT((fd >= 0), "Wrong fd.", MOD_ERR);
    
    module_poll_t *tmp = memhook._malloc(sizeof(module_poll_t));
    MOD_ASSERT(tmp, "Failed to malloc.", MOD_ERR);
       
    tmp->fd = fd;
    poll_set_data(&tmp->ev, (void *)tmp);
    tmp->prev = mod->fds;
    tmp->self = (void *)self;
    mod->fds = tmp;
    c->num_fds++;

    /* If a fd is added at runtime, start polling on it */
    if (module_is(self, RUNNING)) {
        int ret = poll_set_new_evt(tmp, c, ADD);
        return !ret ? MOD_OK : MOD_ERR;
    }
    return MOD_OK;
}

/* Linearly searching for fd */
module_ret_code module_rm_fd(const self_t *self, int fd, int close_fd) {
    /* Cannot rm a fd for STOPPED modules */
    GET_MOD_IN_STATE(self, IDLE | RUNNING | PAUSED);
    MOD_ASSERT((fd >= 0), "Wrong fd.", MOD_ERR);
    
    MOD_ASSERT(mod->fds, "No fd registered in this module.", MOD_ERR);
    module_poll_t **tmp = &mod->fds;
    
    while (*tmp) {
        if ((*tmp)->fd == fd) {
            module_poll_t *t = *tmp;
            *tmp = (*tmp)->prev;
            if (close_fd) {
                close(t->fd);
            }
            free(t->ev);
            free(t);
            c->num_fds--;
            return MOD_OK;
        }
        tmp = &(*tmp)->prev;
    }
    return MOD_ERR;
}

module_ret_code module_update_fd(const self_t *self, int old_fd, int new_fd, int close_old) {
    MOD_ASSERT((old_fd >= 0), "Wrong old fd.", MOD_ERR);
    MOD_ASSERT((new_fd >= 0), "Wrong new fd.", MOD_ERR);
    
    if (module_rm_fd(self, old_fd, close_old) == MOD_OK) {
        return module_add_fd(self, new_fd);
    }
    return MOD_ERR;
}

module_ret_code module_get_name(const self_t *self, char **name) {
    MOD_ASSERT(self, "NULL self handler.", MOD_NO_SELF);
    
    *name = strdup(self->name);
    return MOD_OK;
}

module_ret_code module_get_context(const self_t *self, char **ctx) {
    MOD_ASSERT(self, "NULL self handler.", MOD_NO_SELF);
    
    *ctx = strdup(self->ctx);
    return MOD_OK;
}

/** Actor-like PubSub interface **/

module_ret_code module_register_topic(const self_t *self, const char *topic) {
    MOD_ASSERT(topic, "NULL topic.", MOD_ERR);
    GET_MOD(self);
    void *tmp = NULL;
    
    if (hashmap_get(c->topics, (char *)topic, (void **)&tmp) == MAP_MISSING) {
        if (hashmap_put(c->topics, (char *)topic, mod) == MAP_OK) {
            tell_system_pubsub_msg(c, TOPIC_REGISTERED, topic);
            return MOD_OK;
        }
    }
    return MOD_ERR;
}

module_ret_code module_deregister_topic(const self_t *self, const char *topic) {
    MOD_ASSERT(topic, "NULL topic.", MOD_ERR);
    GET_MOD(self);
    void *tmp = NULL;
    
    /* Only same mod which registered topic can deregister it */
    if (hashmap_get(c->topics, (char *)topic, (void **)&tmp) == MAP_OK && tmp == mod) {
        if (hashmap_remove(c->topics, (char *)topic) == MAP_OK) {
            tell_system_pubsub_msg(c, TOPIC_DEREGISTERED, topic);
            return MOD_OK;
        }
    }
    return MOD_ERR;
}

module_ret_code module_subscribe(const self_t *self, const char *topic) {
    MOD_ASSERT(topic, "NULL topic.", MOD_ERR);
    GET_MOD(self);
    void *tmp = NULL;
    
    if (hashmap_get(c->topics, (char *)topic, (void **)&tmp) == MAP_OK && 
        hashmap_get(mod->subscriptions, (char *)topic, (void **)&tmp) == MAP_MISSING) {
        /* Store pointer to mod as value, even if it will be unused; this should be a hashset */
        if (hashmap_put(mod->subscriptions, (char *)topic, mod) == MAP_OK) {
            return MOD_OK;
        }
    }
    return MOD_ERR;
}

module_ret_code module_unsubscribe(const self_t *self, const char *topic) {
    MOD_ASSERT(topic, "NULL topic.", MOD_ERR);
    GET_MOD(self);
    void *tmp = NULL;
    
    if (hashmap_get(c->topics, (char *)topic, (void **)&tmp) == MAP_OK && 
        hashmap_remove(mod->subscriptions, (char *)topic) == MAP_OK) {
        return MOD_OK;
    }
    return MOD_ERR;
}

module_ret_code tell_system_pubsub_msg(m_context *c, enum sys_msg_t type, ...) {
    pubsub_msg_t m = { .topic = NULL, .sender = NULL, .message = NULL, .type = SYSTEM };
    switch (type) {
        case LOOP_STARTED:
            m.message = "LOOP_STARTED";
            break;
        case LOOP_STOPPED:
            m.message = "LOOP_STOPPED";
            break;
        case TOPIC_REGISTERED:
        case TOPIC_DEREGISTERED:{
            char name[256] = { 0 };
            
            va_list args;
            va_start(args, type);
            
            char *topic = va_arg(args, char *);
            snprintf(name, sizeof(name) - 1, "TOPIC_%s: %s", type == TOPIC_REGISTERED ? "REGISTERED" : "DEREGISTERED", topic);
            m.message = name;
            
            va_end(args);
            }
            break;
        default:
            break;
    }
    if (m.message) {
        return tell_pubsub_msg(&m, NULL, c);
    }
    return MOD_ERR;
}

static int tell_if(void *data, void *m) {
    module *mod = (module *)m;
    const pubsub_msg_t *msg = (pubsub_msg_t *)data;
    void *tmp = NULL;

    /* 
     * Only if mod is actually running and 
     * if topic is null or this module is subscribed to topic 
     */
    if (module_is(&mod->self, RUNNING) && (!msg->topic || 
        hashmap_get(mod->subscriptions, (char *)msg->topic, (void **)&tmp) == MAP_OK)) {
        
        MODULE_DEBUG("Telling a message to %s.\n", mod->self.name);
        
        pubsub_msg_t *mm = create_pubsub_msg(msg->message, msg->sender, msg->topic, msg->type);
        write(mod->pubsub_fd[1], &mm, sizeof(pubsub_msg_t *));
    }
    return MAP_OK;
}

static pubsub_msg_t *create_pubsub_msg(const char *message, const self_t *sender, const char *topic, enum msg_type type) {
    pubsub_msg_t *m = memhook._malloc(sizeof(pubsub_msg_t));
    if (m) {
        m->message = strdup(message);
        m->sender = sender;
        m->topic = strdup(topic);
        *(int *)&m->type = type;
    }
    return m;
}

static module_ret_code tell_pubsub_msg(pubsub_msg_t *m, module *mod, m_context *c) {
    /* Note that if (c) here is just to silence a stupid codacy warning */
    if (c && c->looping) {
        if (mod) {
            tell_if(m, mod);
        } else if (c) {
            hashmap_iterate(c->modules, tell_if, m);
        }
        return MOD_OK;
    }
    return MOD_ERR;
}

module_ret_code module_tell(const self_t *self, const char *recipient, const char *message) {
    MOD_ASSERT(self, "NULL self handler.", MOD_NO_SELF);
    MOD_ASSERT(message, "NULL message.", MOD_ERR);
    MOD_ASSERT(recipient, "NULL recipient.", MOD_ERR);
    
    GET_CTX(self->ctx);
    CTX_GET_MOD(recipient, c);

    pubsub_msg_t m = { .topic = NULL, .message = message, .sender = self, .type = USER };
    return tell_pubsub_msg(&m, mod, NULL);
}

module_ret_code module_reply(const self_t *self, const self_t *sender, const char *message) {
    return module_tell(self, sender->name, message);
}

module_ret_code module_publish(const self_t *self, const char *topic, const char *message) {
    MOD_ASSERT(message, "NULL message.", MOD_ERR);
    
    GET_MOD(self);
    
    void *tmp = NULL;
    /* 
     * Only module that registered a topic can publish on the topic.
     * Moreover, a publish can only be made on existent topic.
     */
    if (!topic || (hashmap_get(c->topics, (char *)topic, (void **)&tmp) == MAP_OK && tmp == mod)) {
        pubsub_msg_t m = { .topic = topic, .message = message, .sender = self, .type = USER };
        return tell_pubsub_msg(&m, NULL, c);
    }
    return MOD_ERR;
}

/** Module state getters **/

/* Define states getters */
int module_is(const self_t *self, const enum module_states st) {
    GET_MOD(self);
    
    return mod->state & st;
}

/** Module state setters **/

static int manage_fds(module *mod, m_context *c, int flag, int stop) {
    module_poll_t *tmp = mod->fds, *t = NULL;
    int ret = 0;
    
    while (tmp && !ret) {
        ret = poll_set_new_evt(tmp, c, flag);
        if (flag == RM && stop) {
            ret += close(tmp->fd);
            free(tmp->ev);
            t = tmp;
        }
        tmp = tmp->prev;
        free(t); // only used when called with stop: properly free module_poll_t
    }
    return ret;
}

static module_ret_code start(const self_t *self, const enum module_states mask, const char *err_str) {
    GET_MOD_IN_STATE(self, mask);
    
    /* 
     * Starting module for the first time
     * or after it was stopped. 
     * Properly add back its pubsub fds
     */
    if (mask & IDLE) {
        if (init_pubsub_fd(mod) != MOD_OK) {
            return MOD_ERR;
        }
    }
    
    int ret = manage_fds(mod, c, ADD, 0);
    MOD_ASSERT(!ret, err_str, MOD_ERR);
    
    mod->state = RUNNING;
    return MOD_OK;
}

static module_ret_code stop(const self_t *self, const enum module_states mask, const char *err_str, int stop) {
    GET_MOD_IN_STATE(self, mask);
    
    int ret = manage_fds(mod, c, RM, stop);
    MOD_ASSERT(!ret, err_str, MOD_ERR);
    
    mod->state = stop ? STOPPED : PAUSED;
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
    return stop(self, RUNNING | IDLE, "Failed to stop module.", 1);
}
