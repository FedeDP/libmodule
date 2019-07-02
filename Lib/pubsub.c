#include "module.h"
#include "poll_priv.h"

/** Actor-like PubSub interface **/

static map_ret_code tell_if(void *data, const char *key, void *value);
static pubsub_priv_t *create_pubsub_msg(const unsigned char *message, const self_t *sender, const char *topic, 
                               enum msg_type type, const size_t size, const bool autofree);
static void destroy_pubsub_msg(pubsub_priv_t *pubsub_msg);
static module_ret_code tell_pubsub_msg(pubsub_priv_t *m, module *mod, m_context *c);
static module_ret_code publish_msg(const module *mod, const char *topic, const unsigned char *message, 
                                   const ssize_t size, const bool autofree);
static map_ret_code unsubscribe(void *data, const char *key, void *value);

static map_ret_code tell_if(void *data, const char *key, void *value) {
    module *mod = (module *)value;
    pubsub_priv_t *msg = (pubsub_priv_t *)data;

    if (_module_is(mod, RUNNING | PAUSED) &&                         // mod is running or paused
        ((msg->msg.type != USER && msg->msg.sender != &mod->self) || // system messages with sender != this module (avoid sending ourselves system messages produced by us)
        (msg->msg.type == USER &&                                    // it is a publish and mod is subscribed on topic, or it is a broadcast/direct tell message
        (!msg->msg.topic || map_has_key(mod->subscriptions, msg->msg.topic))))) {          
        
        MODULE_DEBUG("Telling a message to %s.\n", mod->name);
        
        if (write(mod->pubsub_fd[1], &msg, sizeof(pubsub_priv_t *)) != sizeof(pubsub_priv_t *)) {
            MODULE_DEBUG("Failed to write message for %s: %s\n", mod->name, strerror(errno));
        } else {
            msg->refs++;
        }
    }
    return MAP_OK;
}

static pubsub_priv_t *create_pubsub_msg(const unsigned char *message, const self_t *sender, const char *topic, 
                               enum msg_type type, const size_t size, const bool autofree) {
    pubsub_priv_t *m = memhook._malloc(sizeof(pubsub_priv_t));
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

static void destroy_pubsub_msg(pubsub_priv_t *pubsub_msg) {
    /* Properly free pubsub msg if its ref count reaches 0 and autofree bit is true */
    if (pubsub_msg->refs == 0 || --pubsub_msg->refs == 0) {
        if (pubsub_msg->autofree) {
            memhook._free((void *)pubsub_msg->msg.message);
        }
        memhook._free(pubsub_msg);
    }
}

static module_ret_code tell_pubsub_msg(pubsub_priv_t *m, module *mod, m_context *c) {
    if (mod) {
        tell_if(m, NULL, mod);
    } else {
        map_iterate(c->modules, tell_if, m);
    }
    
    /* Nobody received our message; destroy it right away */
    if (m->refs == 0) {
        destroy_pubsub_msg(m);
    }
    
    return MOD_OK;
}

static module_ret_code publish_msg(const module *mod, const char *topic, const unsigned char *message, 
                                   const ssize_t size, const bool autofree) {
    MOD_PARAM_ASSERT(message);
    MOD_PARAM_ASSERT(size > 0);
    
    GET_CTX_PRIV((&mod->self));
    
    void *tmp = NULL;
    /* 
     * Only module that registered a topic can publish on the topic.
     * Moreover, a publish can only be made on existent topic.
     */
    if (!topic || ((tmp = map_get(c->topics, topic)) && tmp == mod)) {
        pubsub_priv_t *m = create_pubsub_msg(message, &mod->self, topic, USER, size, autofree);
        return tell_pubsub_msg(m, NULL, c);
    }
    return MOD_ERR;
}

static map_ret_code unsubscribe(void *data, const char *key, void *value) {
    module *mod = (module *)value;
    const char *topic = (const char *)data;

    if (map_has_key(mod->subscriptions, topic)) {
        map_remove(mod->subscriptions, topic);
    }
    return MAP_OK;
}

/** Private API **/

module_ret_code tell_system_pubsub_msg(m_context *c, enum msg_type type, const self_t *sender, const char *topic) {
    pubsub_priv_t *m = create_pubsub_msg(NULL, sender, topic, type, 0, false);
    return tell_pubsub_msg(m, NULL, c);
}

map_ret_code flush_pubsub_msg(void *data, const char *key, void *value) {
    module *mod = (module *)value;
    pubsub_priv_t *mm = NULL;

    while (read(mod->pubsub_fd[0], &mm, sizeof(pubsub_priv_t *)) == sizeof(pubsub_priv_t *)) {
        /* 
         * Actually tell msg ONLY if we are not deregistering module, 
         * ie: we are stopping looping on the context. 
         * Else, just free msg.
         */
        if (!data) {
            MODULE_DEBUG("Flushing pubsub message for module '%s'.\n", mod->name);
            msg_t msg = { .is_pubsub = true, .pubsub_msg = &mm->msg };
            run_pubsub_cb(mod, &msg);
        } else {
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
        destroy_pubsub_msg((pubsub_priv_t *)msg->pubsub_msg);
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
        if (map_put(c->topics, topic, mod, false, false) == MAP_OK) {
            tell_system_pubsub_msg(c, TOPIC_REGISTERED, self, topic);
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
            /* Automatically unsubscribe any module subscribed to topic */
            map_iterate(c->modules, unsubscribe, (void *)topic);
            tell_system_pubsub_msg(c, TOPIC_DEREGISTERED, self, topic);
            return MOD_OK;
        }
    }
    return MOD_ERR;
}

module_ret_code module_subscribe(const self_t *self, const char *topic) {
    MOD_PARAM_ASSERT(topic);
    GET_MOD(self);
    GET_CTX(self);
    
    if (map_has_key(c->topics, topic) &&
        (map_has_key(mod->subscriptions, topic) ||
        /* Store pointer to mod as value, even if it will be unused; this should be a hashset */
        map_put(mod->subscriptions, topic, mod, false, false) == MAP_OK)) {
        
        return MOD_OK;
    }
    return MOD_ERR;
}

module_ret_code module_unsubscribe(const self_t *self, const char *topic) {
    MOD_PARAM_ASSERT(topic);
    GET_MOD(self);
    GET_CTX(self);
    
    if (map_has_key(c->topics, topic) &&
        (!map_has_key(mod->subscriptions, topic) ||
        map_remove(mod->subscriptions, topic) == MAP_OK)) {
        
        return MOD_OK;
    }
    return MOD_ERR;
}

module_ret_code module_tell(const self_t *self, const self_t *recipient, const unsigned char *message, 
                            const ssize_t size, const bool autofree) {
    GET_MOD(self);
    MOD_PARAM_ASSERT(message);
    MOD_PARAM_ASSERT(size > 0);
    MOD_PARAM_ASSERT(recipient);
    /* only same ctx modules can talk */
    MOD_PARAM_ASSERT(self->ctx == recipient->ctx);

    pubsub_priv_t *m = create_pubsub_msg(message, &mod->self, NULL, USER, size, autofree);
    return tell_pubsub_msg(m, recipient->mod, recipient->ctx);
}

module_ret_code module_publish(const self_t *self, const char *topic, const unsigned char *message, 
                               const ssize_t size, const bool autofree) {
    MOD_PARAM_ASSERT(topic);
    GET_MOD(self);
    
    return publish_msg(mod, topic, message, size, autofree);
}

module_ret_code module_broadcast(const self_t *self, const unsigned char *message, 
                                 const ssize_t size, const bool autofree) {
    GET_MOD(self);
    
    return publish_msg(mod, NULL, message, size, autofree);
}
