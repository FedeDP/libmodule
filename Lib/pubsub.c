#include "module.h"
#include "poll_priv.h"

/** Actor-like PubSub interface **/

static void subscribtions_dtor(void *data);
static bool is_subscribed(mod_t *mod, ps_priv_t *msg);
static mod_map_ret tell_if(void *data, const char *key, void *value);
static ps_priv_t *create_pubsub_msg(const void *message, const self_t *sender, const char *topic, 
                                    ps_msg_type type, const size_t size, const mod_ps_flags flags);
static mod_map_ret tell_global(void *data, const char *key, void *value);
static void ps_msg_dtor(void *data);
static mod_ret tell_pubsub_msg(ps_priv_t *m, mod_t *mod, ctx_t *c);
static mod_ret send_msg(mod_t *mod, mod_t *recipient, 
                        const char *topic, const void *message, 
                        const ssize_t size, const mod_ps_flags flags);

extern mod_ret fs_notify(const msg_t *msg);

static void subscribtions_dtor(void *data) {
    ev_src_t *sub = (ev_src_t *)data;
    regfree(&sub->ps_src.reg);
    if (sub->flags & SRC_DUP) {
        memhook._free((void *)sub->ps_src.topic);
    }
    if (sub->flags & SRC_AUTOFREE) {
        memhook._free((void *)sub->userptr);
    }
}

static bool is_subscribed(mod_t *mod, ps_priv_t *msg) {    
    /* Check if module is directly subscribed to topic */
    ev_src_t *sub = map_get(mod->subscriptions, msg->msg.topic);
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
    mem_ref(sub);
    queue_enqueue(msg->subs, sub);
    /* 
     * Replace user provided pointer with source one, 
     * to allow stack-allocated topic to be used
     */
    msg->msg.topic = sub->ps_src.topic;
    return true;
}

static mod_map_ret tell_if(void *data, const char *key, void *value) {
    mod_t *mod = (mod_t *)value;
    ps_priv_t *msg = (ps_priv_t *)data;

    if (module_is(mod->self, RUNNING | PAUSED) &&                       // mod is running or paused
        ((msg->msg.type != USER && msg->msg.sender != &mod->ref) ||     // system messages with sender != this module (avoid sending ourselves system messages produced by us)
        (msg->msg.type == USER &&                                       // it is a publish and mod is subscribed on topic, or it is a broadcast/direct tell message
        (!msg->msg.topic || is_subscribed(mod, msg))))) {
        
        MODULE_DEBUG("Telling a message to '%s'\n", mod->name);
        
        if (write(mod->pubsub_fd[1], &msg, sizeof(ps_priv_t *)) != sizeof(ps_priv_t *)) {
            MODULE_DEBUG("Failed to write message: %s\n", strerror(errno));
        } else {
            mem_ref(msg);
        }
    }
    return MAP_OK;
}

static ps_priv_t *create_pubsub_msg(const void *message, const self_t *sender, const char *topic, 
                                    ps_msg_type type, const size_t size, const mod_ps_flags flags) {
    ps_priv_t *m = mem_ref_new(sizeof(ps_priv_t), ps_msg_dtor);
    if (m) {
        m->msg.sender = sender;
        if (sender) {
            mem_ref(sender->mod); // keep module alive until message is dispatched
        }
        m->subs = queue_new(mem_unref);
        m->msg.topic = topic;
        m->msg.type = type;
        m->msg.size = size;
        m->flags = flags;
        /* Duplicate data if requested */
        if (m->flags & PS_DUP_DATA) {
            m->flags |= PS_AUTOFREE; // force autofree flag if we duplicated data
            void *new_data = memhook._malloc(size);
            if (new_data) {
                memcpy(new_data, message, size);
                m->msg.data = new_data;
            } else {
                memhook._free(m);
                m = NULL;
            }
        } else {
            m->msg.data = message;
        }
    }
    return m;
}

static mod_map_ret tell_global(void *data, const char *key, void *value) {
    ctx_t *c = (ctx_t *)value;
    ps_priv_t *msg = (ps_priv_t *)data;
    
    map_iterate(c->modules, tell_if, msg);
    return MAP_OK;
}

static void ps_msg_dtor(void *data) {
    ps_priv_t *pubsub_msg = (ps_priv_t *)data;
    
    /* Clear (unref'ng) and destroy subs queue */
    queue_free(&pubsub_msg->subs);
    if (pubsub_msg->flags & PS_AUTOFREE) {
        memhook._free((void *)pubsub_msg->msg.data);
    }
    if (pubsub_msg->msg.sender) {
        mem_unref(pubsub_msg->msg.sender->mod);
    }
}


static mod_ret tell_pubsub_msg(ps_priv_t *m, mod_t *mod, ctx_t *c) {
    MOD_ALLOC_ASSERT(m);
    
    if (mod) {
        tell_if(m, NULL, mod);
    } else if (!(m->flags & PS_GLOBAL)) {
        map_iterate(c->modules, tell_if, m);
    } else {
        map_iterate(ctx, tell_global, m);
    }
    
    /* 
     * If nobody received our message; destroy it right away;
     * else: num of refs for this message == number of modules that received it.
     */
    mem_unref(m);
    return MOD_OK;
}

static mod_ret send_msg(mod_t *mod, mod_t *recipient, 
                        const char *topic, const void *message, 
                        const ssize_t size, const mod_ps_flags flags) {
    MOD_PARAM_ASSERT(message);
    MOD_PARAM_ASSERT(size > 0);
    GET_CTX(mod->self);
    
    mod->stats.sent_msgs++;
    fetch_ms(&mod->stats.last_seen, &mod->stats.action_ctr);
    ps_priv_t *m = create_pubsub_msg(message, &mod->ref, topic, USER, size, flags);
    return tell_pubsub_msg(m, recipient, c);
}

/** Private API **/

mod_ret tell_system_pubsub_msg(mod_t *recipient, ctx_t *c, ps_msg_type type, const self_t *sender, const char *topic) {
    if (sender) {
        fetch_ms(&sender->mod->stats.last_seen, &sender->mod->stats.action_ctr);
    }
    ps_priv_t *m = create_pubsub_msg(NULL, sender, topic, type, 0, 0);
    return tell_pubsub_msg(m, recipient, c);
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
        if (!data && module_is(mod->self, RUNNING)) {
            MODULE_DEBUG("Flushing enqueued pubsub message for module '%s'.\n", mod->name);
            msg_t msg = { .type = TYPE_PS, .ps_msg = &mm->msg };
            run_pubsub_cb(mod, &msg, queue_peek(mm->subs));
        } else {
            MODULE_DEBUG("Destroying enqueued pubsub message for module '%s'.\n", mod->name);
            queue_remove(mm->subs);
            mem_unref(mm);
        }
    }
    return 0;
}

void run_pubsub_cb(mod_t *mod, msg_t *msg, const ev_src_t *src) {
    /* If module is using some different receive function, honor it. */
    recv_cb cb = stack_peek(mod->recvs);
    if (!cb) {
        /* Fallback to module default receive */
        cb = mod->hook.recv;
    }
    
    msg->self = mod->self;
    
    /* Notify underlying fuse fs */
    fs_notify(msg);
        
    /* Finally call user callback */
    cb(msg, src ? src->userptr : NULL);

    if (msg->type == TYPE_PS) {
        ps_priv_t *mm = (ps_priv_t *)msg->ps_msg;
        queue_remove(mm->subs);
        mem_unref(mm);
    }
    
    mod->stats.recv_msgs++;
    fetch_ms(&mod->stats.last_seen, &mod->stats.action_ctr);
}

/** Public API **/

mod_ret module_ref(const self_t *self, const char *name, const self_t **modref) {
    MOD_PARAM_ASSERT(name);
    MOD_PARAM_ASSERT(modref);
    MOD_PARAM_ASSERT(!*modref);

    GET_CTX(self);
    CTX_GET_MOD(name, c);
    
    *modref = &mod->ref;
    return MOD_OK;
}

mod_ret module_register_sub(const self_t *self, const char *topic, mod_src_flags flags, const void *userptr) {
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
            mod->subscriptions = map_new(false, mem_unref);
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
        
        /* Store new sub as ref'd memory */
        ev_src_t *sub = mem_ref_new(sizeof(ev_src_t), subscribtions_dtor);
        MOD_ALLOC_ASSERT(sub);
        
        ps_src_t *ps_src = &sub->ps_src;
        sub->type = TYPE_PS;
        sub->flags = flags;
        sub->userptr = userptr;
        sub->self = self;
        memcpy(&ps_src->reg, &regex, sizeof(regex_t));
        ps_src->topic = sub->flags & SRC_DUP ? mem_strdup(topic) : topic;
        if (map_put(mod->subscriptions, ps_src->topic, sub) == MAP_OK) {
            fetch_ms(&mod->stats.last_seen, &mod->stats.action_ctr);
            return MOD_OK;
        }
    }
    return MOD_ERR;
}

mod_ret module_deregister_sub(const self_t *self, const char *topic) {
    MOD_PARAM_ASSERT(topic);
    GET_MOD(self);
    
    if (map_remove(mod->subscriptions, topic) == MAP_OK) {
        fetch_ms(&mod->stats.last_seen, &mod->stats.action_ctr);
        if (map_length(mod->subscriptions) == 0) {
            map_free(&mod->subscriptions);
        }
        return MOD_OK;
    }
    return MOD_ERR;
}

mod_ret module_tell(const self_t *self, const self_t *recipient, const void *message, 
                    const ssize_t size, const mod_ps_flags flags) {
    GET_MOD(self);
    MOD_PARAM_ASSERT(recipient);
    /* only same ctx modules can talk */
    MOD_PARAM_ASSERT(self->ctx == recipient->ctx);

    /* Eventually cleanup PS_GLOBAL flag */    
    return send_msg(mod, recipient->mod, NULL, message, size, flags & ~PS_GLOBAL);
}

mod_ret module_publish(const self_t *self, const char *topic, const void *message, 
                       const ssize_t size, const mod_ps_flags flags) {
    MOD_PARAM_ASSERT(topic);
    GET_MOD(self);
    
    /* Eventually cleanup PS_GLOBAL flag */    
    return send_msg(mod, NULL, topic, message, size, flags & ~PS_GLOBAL);
}

mod_ret module_broadcast(const self_t *self, const void *message, 
                         const ssize_t size, const mod_ps_flags flags) {
    GET_MOD(self);
    
    return send_msg(mod, NULL, NULL, message, size, flags);
}

mod_ret module_poisonpill(const self_t *self, const self_t *recipient) {
    GET_MOD(self);
    GET_CTX(self);
    MOD_PARAM_ASSERT(module_is(recipient, RUNNING));
    
    return tell_system_pubsub_msg(recipient->mod, c, MODULE_POISONPILL, &mod->ref, NULL);
}
