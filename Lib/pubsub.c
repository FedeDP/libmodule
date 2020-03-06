#include "module.h"
#include "poll_priv.h"     

/** Actor-like PubSub interface **/

static void subscribtions_dtor(void *data);
static bool is_subscribed(mod_t *mod, ps_priv_t *msg);
static mod_map_ret tell_if(void *data, const char *key, void *value);
static ps_priv_t *create_pubsub_msg(const void *message, const self_t *sender, const char *topic, 
                               ps_msg_type type, const size_t size, const bool autofree);
static mod_map_ret tell_global(void *data, const char *key, void *value);
static void pubsub_msg_ref(ps_priv_t *pubsub_msg);
static void pubsub_msg_unref(ps_priv_t *pubsub_msg);
static mod_ret tell_pubsub_msg(ps_priv_t *m, mod_t *mod, ctx_t *c, const bool global);
static mod_ret send_msg(const mod_t *mod, mod_t *recipient, 
                                   const char *topic, const void *message, 
                                   const ssize_t size, const bool autofree, const bool global);

static void subscribtions_dtor(void *data) {
    ev_src_t *sub = (ev_src_t *)data;
    regfree(&sub->ps_src.reg);
    if (sub->flags & SRC_DUP) {
        memhook._free((void *)sub->ps_src.topic);
    }
    if (sub->flags & SRC_AUTOFREE) {
        memhook._free((void *)sub->userptr);
    }
    memhook._free(sub);
}

static bool is_subscribed(mod_t *mod, ps_priv_t *msg) {
    /* Check if module is directly subscribed to topic */
    const ev_src_t *sub = map_get(mod->subscriptions, msg->msg.topic);
    if (sub) {
        goto found;
    }
    
    /* Check if any stored subscriptions is a regex that matches topic */
    mod_map_itr_t *itr = map_itr_new(mod->subscriptions);
    for (; itr; itr = map_itr_next(itr)) {
        sub = map_itr_get_data(itr);

        /* Execute regular expression */
        int ret = regexec(&sub->ps_src.reg, msg->msg.topic, 0, NULL, 0);
        if (!ret) {
            memhook._free(itr);
            goto found;
        }
    }
    return false;
    
found:
    msg->userptr = sub->userptr;
    return true;
}

static mod_map_ret tell_if(void *data, const char *key, void *value) {
    mod_t *mod = (mod_t *)value;
    ps_priv_t *msg = (ps_priv_t *)data;

    if (_module_is(mod, RUNNING | PAUSED) &&                         // mod is running or paused
        ((msg->msg.type != USER && msg->msg.sender != &mod->self) || // system messages with sender != this module (avoid sending ourselves system messages produced by us)
        (msg->msg.type == USER &&                                    // it is a publish and mod is subscribed on topic, or it is a broadcast/direct tell message
        (!msg->msg.topic || is_subscribed(mod, msg))))) {     
        
        MODULE_DEBUG("Telling a message to '%s'\n", mod->name);
        
        if (write(mod->pubsub_fd[1], &msg, sizeof(ps_priv_t *)) != sizeof(ps_priv_t *)) {
            MODULE_DEBUG("Failed to write message: %s\n", strerror(errno));
        } else {
            pubsub_msg_ref(msg);
        }
    }
    return MAP_OK;
}

static ps_priv_t *create_pubsub_msg(const void *message, const self_t *sender, const char *topic, 
                               ps_msg_type type, const size_t size, const bool autofree) {
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

static mod_map_ret tell_global(void *data, const char *key, void *value) {
    ctx_t *c = (ctx_t *)value;
    ps_priv_t *msg = (ps_priv_t *)data;
    
    map_iterate(c->modules, tell_if, msg);
    return MAP_OK;
}

static void pubsub_msg_ref(ps_priv_t *pubsub_msg) {
    pubsub_msg->refs++;
}
    

static void pubsub_msg_unref(ps_priv_t *pubsub_msg) {
    /* Properly free pubsub msg if its ref count reaches 0 and autofree bit is true */
    if (pubsub_msg->refs == 0 || --pubsub_msg->refs == 0) {
        if (pubsub_msg->autofree) {
            memhook._free((void *)pubsub_msg->msg.message);
        }
        memhook._free(pubsub_msg);
    }
}

static mod_ret tell_pubsub_msg(ps_priv_t *m, mod_t *mod, ctx_t *c, const bool global) {
    if (mod) {
        tell_if(m, NULL, mod);
    } else if (!global) {
        map_iterate(c->modules, tell_if, m);
    } else {
        map_iterate(ctx, tell_global, m);
    }
    
    /* Nobody received our message; destroy it right away */
    if (m->refs == 0) {
        pubsub_msg_unref(m);
    }
    
    return MOD_OK;
}

static mod_ret send_msg(const mod_t *mod, mod_t *recipient, 
                                   const char *topic, const void *message, 
                                   const ssize_t size, const bool autofree, const bool global) {
    MOD_PARAM_ASSERT(message);
    MOD_PARAM_ASSERT(size > 0);
    
    GET_CTX_PRIV((&mod->self));
    
    ps_priv_t *m = create_pubsub_msg(message, &mod->self, topic, USER, size, autofree);
    return tell_pubsub_msg(m, recipient, c, global);
}

/** Private API **/

mod_ret tell_system_pubsub_msg(mod_t *mod, ctx_t *c, ps_msg_type type, const self_t *sender, const char *topic) {
    ps_priv_t *m = create_pubsub_msg(NULL, sender, topic, type, 0, false);
    return tell_pubsub_msg(m, mod, c, false);
}

mod_map_ret flush_pubsub_msgs(void *data, const char *key, void *value) {
    mod_t *mod = (mod_t *)value;
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
            msg_t msg = { .type = TYPE_PS, .ps_msg = &mm->msg };
            run_pubsub_cb(mod, &msg, mm->userptr);
        } else {
            MODULE_DEBUG("Destroying enqueued pubsub message for module '%s'.\n", mod->name);
            pubsub_msg_unref(mm);
        }
    }
    return 0;
}

void run_pubsub_cb(mod_t *mod, msg_t *msg, const void *userptr) {
    /* If module is using some different receive function, honor it. */
    recv_cb cb = stack_peek(mod->recvs);
    if (!cb) {
        /* Fallback to module default receive */
        cb = mod->hook.recv;
    }
    cb(msg, userptr);

    if (msg->type == TYPE_PS) {
        pubsub_msg_unref((ps_priv_t *)msg->ps_msg);
    }
}

/** Public API **/

mod_ret module_ref(const self_t *self, const char *name, const self_t **modref) {
    MOD_PARAM_ASSERT(name);
    MOD_PARAM_ASSERT(modref);
    MOD_PARAM_ASSERT(!*modref);

    GET_CTX(self);
    CTX_GET_MOD(name, c);
    
    *modref = &mod->self;
    return MOD_OK;
}

mod_ret module_subscribe(const self_t *self, const char *topic, mod_src_flags flags, const void *userptr) {
    MOD_PARAM_ASSERT(topic);
    GET_MOD(self);
    GET_CTX(self);
    
    /* Check if it is a valid regex: compile it */
    regex_t regex;
    int ret = regcomp(&regex, topic, REG_NOSUB);
    if (!ret) {
        MODULE_DEBUG("'%s' is a valid regex.\n", topic);
        
        /* Lazy subscriptions map init */
        if (!mod->subscriptions)  {
            mod->subscriptions = map_new(false, subscribtions_dtor);
            MOD_ALLOC_ASSERT(mod->subscriptions);
        } else {
            ev_src_t *old_sub = map_get(mod->subscriptions, topic);
            if (old_sub) {
                if (old_sub->flags == flags) {
                    /* Update just userptr. */
                    old_sub->userptr = userptr;
                    return MOD_OK;
                }
                /* Remove old subscription object; requested flags are different! */
                map_remove(mod->subscriptions, topic);
            }
        }
        
        /* Store regex on heap */
        ev_src_t *sub = memhook._calloc(1, sizeof(ev_src_t));
        MOD_ALLOC_ASSERT(sub);
        
        ps_src_t *ps_src = &sub->ps_src;
        sub->type = TYPE_PS;
        sub->flags = flags;
        sub->userptr = userptr;
        memcpy(&ps_src->reg, &regex, sizeof(regex_t));
        ps_src->topic = sub->flags & SRC_DUP ? mem_strdup(topic) : topic;
        if (map_put(mod->subscriptions, ps_src->topic, sub) == MAP_OK) {
            return MOD_OK;
        }
    }
    return MOD_ERR;
}

mod_ret module_unsubscribe(const self_t *self, const char *topic) {
    MOD_PARAM_ASSERT(topic);
    GET_MOD(self);
    
    if (map_has_key(mod->subscriptions, topic) &&
        map_remove(mod->subscriptions, topic) == MAP_OK) {
        
        return MOD_OK;
    }
    return MOD_ERR;
}

mod_ret module_tell(const self_t *self, const self_t *recipient, const void *message, 
                            const ssize_t size, const bool autofree) {
    GET_MOD(self);
    MOD_PARAM_ASSERT(recipient);
    /* only same ctx modules can talk */
    MOD_PARAM_ASSERT(self->ctx == recipient->ctx);

    return send_msg(mod, recipient->mod, NULL, message, size, autofree, false);
}

mod_ret module_publish(const self_t *self, const char *topic, const void *message, 
                               const ssize_t size, const bool autofree) {
    MOD_PARAM_ASSERT(topic);
    GET_MOD(self);
    
    return send_msg(mod, NULL, topic, message, size, autofree, false);
}

mod_ret module_broadcast(const self_t *self, const void *message, 
                                 const ssize_t size, const bool autofree, bool global) {
    GET_MOD(self);
    
    return send_msg(mod, NULL, NULL, message, size, autofree, global);
}

mod_ret module_poisonpill(const self_t *self, const self_t *recipient) {
    GET_MOD(self);
    GET_CTX(self);
    MOD_PARAM_ASSERT(module_is(recipient, RUNNING));
    
    return tell_system_pubsub_msg(recipient->mod, c, MODULE_POISONPILL, &mod->self, NULL);
}

mod_ret module_msg_ref(const self_t *self, const ps_msg_t *msg) {
    /* You can only ref a msg from within a module context */
    MOD_ASSERT((self), "NULL self handler.", MOD_NO_SELF);
    MOD_PARAM_ASSERT(msg);
       
    ps_priv_t *priv_msg = (ps_priv_t *)msg;
    pubsub_msg_ref(priv_msg);
    return MOD_OK;
}
    
mod_ret module_msg_unref(const self_t *self, const ps_msg_t *msg) {
    /* You can only unref a msg from within a module context */
    MOD_ASSERT((self), "NULL self handler.", MOD_NO_SELF);
    MOD_PARAM_ASSERT(msg);
            
    ps_priv_t *priv_msg = (ps_priv_t *)msg;
    pubsub_msg_unref(priv_msg);
    return MOD_OK;
}
