#include "module.h"
#include "poll_priv.h"

/** Actor-like PubSub interface **/

static map_ret_code tell_if(void *data, const char *key, void *value);
static ps_priv_t *create_pubsub_msg(const void *message, const self_t *sender, const char *topic, 
                               enum msg_type type, const size_t size, const bool autofree);
static map_ret_code tell_global(void *data, const char *key, void *value);
static void destroy_pubsub_msg(ps_priv_t *pubsub_msg);
static module_ret_code tell_pubsub_msg(ps_priv_t *m, module *mod, m_context *c, const bool global);
static module_ret_code publish_msg(const module *mod, const char *topic, const void *message, 
                                   const ssize_t size, const bool autofree, const bool global);

static map_ret_code tell_if(void *data, const char *key, void *value) {
    module *mod = (module *)value;
    ps_priv_t *msg = (ps_priv_t *)data;

    if (_module_is(mod, RUNNING | PAUSED) &&                         // mod is running or paused
        ((msg->msg.type != USER && msg->msg.sender != &mod->self) || // system messages with sender != this module (avoid sending ourselves system messages produced by us)
        (msg->msg.type == USER &&                                    // it is a publish and mod is subscribed on topic, or it is a broadcast/direct tell message
        (!msg->msg.topic || map_has_key(mod->subscriptions, msg->msg.topic))))) {     
        
        MODULE_DEBUG("Telling a message to '%s'.\n", mod->name);
        
        if (write(mod->pubsub_fd[1], &msg, sizeof(ps_priv_t *)) != sizeof(ps_priv_t *)) {
            MODULE_DEBUG("Failed to write message: %s\n", strerror(errno));
        } else {
            msg->refs++;
        }
    }
    return MAP_OK;
}

static ps_priv_t *create_pubsub_msg(const void *message, const self_t *sender, const char *topic, 
                               enum msg_type type, const size_t size, const bool autofree) {
    ps_priv_t *m = memhook._malloc(sizeof(ps_priv_t));
    if (m) {
        m->msg.message = message;
        m->msg.sender = sender;
        m->msg.topic = topic;
        m->msg.type = type;
        m->msg.size = size;
        m->refs = 0;
        m->autofree = autofree;
    }
    return m;
}

static map_ret_code tell_global(void *data, const char *key, void *value) {
    m_context *c = (m_context *)value;
    ps_priv_t *msg = (ps_priv_t *)data;
    
    map_iterate(c->modules, tell_if, msg);
    return MAP_OK;
}

static void destroy_pubsub_msg(ps_priv_t *pubsub_msg) {
    /* Properly free pubsub msg if its ref count reaches 0 and autofree bit is true */
    if (pubsub_msg->refs == 0 || --pubsub_msg->refs == 0) {
        if (pubsub_msg->autofree) {
            memhook._free((void *)pubsub_msg->msg.message);
        }
        memhook._free(pubsub_msg);
    }
}

static module_ret_code tell_pubsub_msg(ps_priv_t *m, module *mod, m_context *c, const bool global) {
    if (mod) {
        tell_if(m, NULL, mod);
    } else if (!global) {
        map_iterate(c->modules, tell_if, m);
    } else {
        map_iterate(ctx, tell_global, m);
    }
    
    /* Nobody received our message; destroy it right away */
    if (m->refs == 0) {
        destroy_pubsub_msg(m);
    }
    
    return MOD_OK;
}


static module_ret_code publish_msg(const module *mod, const char *topic, const void *message, 
                                   const ssize_t size, const bool autofree, const bool global) {
    MOD_PARAM_ASSERT(message);
    MOD_PARAM_ASSERT(size > 0);
    
    GET_CTX_PRIV((&mod->self));
    
    void *tmp = NULL;
    /* 
     * Only module that registered a topic can publish on the topic.
     * Moreover, a publish can only be made on existent topic.
     */
    if (!topic || ((tmp = map_get(c->topics, topic)) && tmp == mod)) {
        ps_priv_t *m = create_pubsub_msg(message, &mod->self, topic, USER, size, autofree);
        return tell_pubsub_msg(m, NULL, c, global);
    }
    return MOD_ERR;
}

/** Private API **/

module_ret_code tell_system_pubsub_msg(module *mod, m_context *c, enum msg_type type, const self_t *sender, const char *topic) {
    ps_priv_t *m = create_pubsub_msg(NULL, sender, topic, type, 0, false);
    return tell_pubsub_msg(m, mod, c, false);
}

map_ret_code flush_pubsub_msgs(void *data, const char *key, void *value) {
    module *mod = (module *)value;
    ps_priv_t *mm = NULL;

    while (read(mod->pubsub_fd[0], &mm, sizeof(ps_priv_t *)) == sizeof(ps_priv_t *)) {
        /*
         * Actually tell msg ONLY if we are not deregistering module,
         * ie: we are stopping looping on the context.
         * Else, just free msg.
         *
         * While stopping module, manage_fds() will call this with data != NULL
         * to let us know we should destroy all enqueued messages.
         */
        if (!data && _module_is(mod, RUNNING)) {
            MODULE_DEBUG("Flushing enqueued pubsub message for module '%s'.\n", mod->name);
            msg_t msg = { .is_pubsub = true, .ps_msg = &mm->msg };
            run_pubsub_cb(mod, &msg);
        } else {
            MODULE_DEBUG("Destroying enqueued pubsub message for module '%s'.\n", mod->name);
            destroy_pubsub_msg(mm);
        }
    }
    return 0;
}

void run_pubsub_cb(module *mod, msg_t *msg) {
    /* If module is using some different receive function, honor it. */
    recv_cb cb = stack_peek(mod->recvs);
    if (!cb) {
        /* Fallback to module default receive */
        cb = mod->hook.recv;
    }
    cb(msg, mod->userdata);

    if (msg->is_pubsub) {
        destroy_pubsub_msg((ps_priv_t *)msg->ps_msg);
    }
}

/** Public API **/

module_ret_code module_ref(const self_t *self, const char *name, const self_t **modref) {
    MOD_PARAM_ASSERT(name);
    MOD_PARAM_ASSERT(modref);
    MOD_PARAM_ASSERT(!*modref);

    GET_CTX(self);
    CTX_GET_MOD(name, c);
    
    *modref = &mod->self;
    return MOD_OK;
}

module_ret_code module_register_topic(const self_t *self, const char *topic) {
    MOD_PARAM_ASSERT(topic);
    GET_MOD(self);
    GET_CTX(self);
    
    if (!map_has_key(c->topics, topic)) {
        if (map_put(c->topics, topic, mod) == MAP_OK) {
            tell_system_pubsub_msg(NULL, c, TOPIC_REGISTERED, &mod->self, topic);
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
            tell_system_pubsub_msg(NULL, c, TOPIC_DEREGISTERED, &mod->self, topic);
            return MOD_OK;
        }
    }
    return MOD_ERR;
}

module_ret_code module_subscribe(const self_t *self, const char *topic) {
    MOD_PARAM_ASSERT(topic);
    GET_MOD(self);
    GET_CTX(self);
    
    if (!map_has_key(mod->subscriptions, topic) &&
        /* Store pointer to mod as value, even if it will be unused; this should be a hashset */
        map_put(mod->subscriptions, topic, mod) == MAP_OK) {
        
        return MOD_OK;
    }
    return MOD_ERR;
}

module_ret_code module_unsubscribe(const self_t *self, const char *topic) {
    MOD_PARAM_ASSERT(topic);
    GET_MOD(self);
    
    if (map_has_key(mod->subscriptions, topic) &&
        map_remove(mod->subscriptions, topic) == MAP_OK) {
        
        return MOD_OK;
    }
    return MOD_ERR;
}

module_ret_code module_tell(const self_t *self, const self_t *recipient, const void *message, 
                            const ssize_t size, const bool autofree) {
    GET_MOD(self);
    MOD_PARAM_ASSERT(message);
    MOD_PARAM_ASSERT(size > 0);
    MOD_PARAM_ASSERT(recipient);
    /* only same ctx modules can talk */
    MOD_PARAM_ASSERT(self->ctx == recipient->ctx);

    ps_priv_t *m = create_pubsub_msg(message, &mod->self, NULL, USER, size, autofree);
    return tell_pubsub_msg(m, recipient->mod, recipient->ctx, false);
}

module_ret_code module_publish(const self_t *self, const char *topic, const void *message, 
                               const ssize_t size, const bool autofree) {
    MOD_PARAM_ASSERT(topic);
    GET_MOD(self);
    
    return publish_msg(mod, topic, message, size, autofree, false);
}

module_ret_code module_broadcast(const self_t *self, const void *message, 
                                 const ssize_t size, const bool autofree, bool global) {
    GET_MOD(self);
    
    return publish_msg(mod, NULL, message, size, autofree, global);
}

module_ret_code module_poisonpill(const self_t *self, const self_t *recipient) {
    GET_MOD(self);
    GET_CTX(self);
    MOD_PARAM_ASSERT(module_is(recipient, RUNNING));
    
    return tell_system_pubsub_msg(recipient->mod, c, MODULE_POISONPILL, &mod->self, NULL);
} 
