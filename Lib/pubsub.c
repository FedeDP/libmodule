#include "module.h"
#include "poll_priv.h"

/** Actor-like PubSub interface **/

static int tell_if(void *data, void *m);
static pubsub_msg_t *create_pubsub_msg(const unsigned char *message, const self_t *sender, const char *topic, enum msg_type type, const size_t size);
static module_ret_code tell_pubsub_msg(pubsub_msg_t *m, module *mod, m_context *c);
static module_ret_code publish_msg(const module *mod, const char *topic, 
                                   const unsigned char *message, const ssize_t size);

static int tell_if(void *data, void *m) {
    module *mod = (module *)m;
    const pubsub_msg_t *msg = (pubsub_msg_t *)data;

    /* 
     * Only if mod is actually running or paused and 
     * it is a SYSTEM message or
     * topic is null or this module is subscribed to topic 
     */
    if (_module_is(mod, RUNNING | PAUSED) && (msg->type != USER || !msg->topic || 
        map_has_key(mod->subscriptions, msg->topic))) {
        
        MODULE_DEBUG("Telling a message to %s.\n", mod->name);
        
        pubsub_msg_t *mm = create_pubsub_msg(msg->message, msg->sender, msg->topic, msg->type, msg->size);
        if (!mm || write(mod->pubsub_fd[1], &mm, sizeof(pubsub_msg_t *)) != sizeof(pubsub_msg_t *)) {
            MODULE_DEBUG("Failed to write message for %s: %s\n", mod->name, strerror(errno));
        }
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

static module_ret_code tell_pubsub_msg(pubsub_msg_t *m, module *mod, m_context *c) {
    if (mod) {
        tell_if(m, mod);
    } else {
        map_iterate(c->modules, tell_if, m);
    }
    return MOD_OK;
}

static module_ret_code publish_msg(const module *mod, const char *topic, 
                                   const unsigned char *message, const ssize_t size) {
    MOD_PARAM_ASSERT(message);
    MOD_PARAM_ASSERT(size > 0);
    
    GET_CTX_PURE((&mod->self));
    
    void *tmp = NULL;
    /* 
     * Only module that registered a topic can publish on the topic.
     * Moreover, a publish can only be made on existent topic.
     */
    if (!topic || ((tmp = map_get(c->topics, topic)) && tmp == mod)) {
        pubsub_msg_t m = { .topic = topic, .message = message, .sender = &mod->self, .type = USER, .size = size };
        return tell_pubsub_msg(&m, NULL, c);
    }
    return MOD_ERR;
}

/** Private API **/

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
            const msg_t msg = { .is_pubsub = true, .pubsub_msg = mm };
            run_pubsub_cb(mod, &msg);
        }
        destroy_pubsub_msg(mm);
    }
    return 0;
}

void destroy_pubsub_msg(pubsub_msg_t *m) {
    memhook._free((char *)m->topic);
    memhook._free(m);
}


void run_pubsub_cb(module *mod, const msg_t *msg) {
    /* If module is using some different receive function, honor it. */
    recv_cb cb = stack_peek(mod->recvs);
    if (!cb) {
        /* Fallback to module default receive */
        cb = mod->hook.recv;
    }
    cb(msg, mod->userdata);
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
        if (map_put(c->topics, topic, mod, true, false) == MAP_OK) {
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
        if (map_put(mod->subscriptions, topic, mod, true, false) == MAP_OK) {
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

module_ret_code module_tell(const self_t *self, const self_t *recipient, const unsigned char *message, 
                            const ssize_t size) {
    GET_MOD(self);
    MOD_PARAM_ASSERT(message);
    MOD_PARAM_ASSERT(size > 0);
    MOD_PARAM_ASSERT(recipient);
    /* only same ctx modules can talk */
    MOD_PARAM_ASSERT(self->ctx == recipient->ctx);

    pubsub_msg_t m = { .topic = NULL, .message = message, .sender = &mod->self, .type = USER, .size = size };
    return tell_pubsub_msg(&m, recipient->mod, recipient->ctx);
}

module_ret_code module_publish(const self_t *self, const char *topic, const unsigned char *message, const ssize_t size) {
    MOD_PARAM_ASSERT(topic);
    GET_MOD(self);
    
    return publish_msg(mod, topic, message, size);
}

module_ret_code module_broadcast(const self_t *self, const unsigned char *message, const ssize_t size) {
    GET_MOD(self);
    
    return publish_msg(mod, NULL, message, size);
}
