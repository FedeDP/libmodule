#include "mod.h"
#include "poll_priv.h"

/** Actor-like PubSub interface **/

static void subscribtions_dtor(void *data);
static ev_src_t *fetch_sub(mod_t *mod, const char *topic);
static int tell_if(void *data, const char *key, void *value);
static ps_priv_t *alloc_ps_msg(const ps_priv_t *msg, ev_src_t *sub);
static int tell_global(void *data, const char *key, void *value);
static void ps_msg_dtor(void *data);
static int tell_pubsub_msg(ps_priv_t *m, const mod_t *recipient, ctx_t *c);
static int send_msg(mod_t *mod, const mod_t *recipient, const char *topic, 
                    const void *message, const mod_ps_flags flags);

extern int fs_notify(const msg_t *msg);

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

static ev_src_t *fetch_sub(mod_t *mod, const char *topic) {    
    /* Check if module is directly subscribed to topic */
    ev_src_t *sub = m_map_get(mod->subscriptions, topic);
    if (sub) {        
        goto found;
    }
    
    /* Check if any stored subscriptions is a regex that matches topic */
    m_itr_foreach(mod->subscriptions, {
        sub = m_itr_get(itr);
        /* Execute regular expression */
        int ret = regexec(&sub->ps_src.reg, topic, 0, NULL, 0);
        if (!ret) {
            memhook._free(itr);
            goto found;
        }
    });
    return NULL;
    
found:
    return sub;
}

static int tell_if(void *data, const char *key, void *value) {
    mod_t *mod = (mod_t *)value;
    ps_priv_t *msg = (ps_priv_t *)data;
    ev_src_t *sub = NULL;

    if (m_mod_is(mod, RUNNING | PAUSED) &&                              // mod is running or paused
        ((msg->msg.type != USER && msg->msg.sender != mod) ||           // system messages with sender != this module (avoid sending ourselves system messages produced by us)
        (msg->msg.type == USER &&                                       // it is a publish and mod is subscribed on topic, or it is a broadcast/direct tell message
        (!msg->msg.topic || (sub = fetch_sub(mod, msg->msg.topic)))))) {
        
        M_DEBUG("Telling a message to '%s'\n", mod->name);
        ps_priv_t *m = alloc_ps_msg(msg, sub);
        if (m) {
            if (write(mod->pubsub_fd[1], &m, sizeof(ps_priv_t *)) != sizeof(ps_priv_t *)) {
                M_DEBUG("Failed to write message: %s\n", strerror(errno));
                m_mem_unref(msg);
            }
        }
    }
    return 0;
}

static ps_priv_t *alloc_ps_msg(const ps_priv_t *msg, ev_src_t *sub) {
    ps_priv_t *m = m_mem_new(sizeof(ps_priv_t), ps_msg_dtor);
    if (m) {
        memcpy(m, msg, sizeof(ps_priv_t));
        if (m->msg.sender) {
            m_mem_ref((void *)m->msg.sender); // keep module alive until message is dispatched
        }
        m->sub = m_mem_ref(sub);
    }
    return m;
}

static int tell_global(void *data, const char *key, void *value) {
    ctx_t *c = (ctx_t *)value;
    ps_priv_t *msg = (ps_priv_t *)data;
    
    m_map_iterate(c->modules, tell_if, msg);
    return 0;
}

static void ps_msg_dtor(void *data) {
    ps_priv_t *pubsub_msg = (ps_priv_t *)data;
    
    m_mem_unref(pubsub_msg->sub);
    if (pubsub_msg->flags & PS_AUTOFREE) {
        memhook._free((void *)pubsub_msg->msg.data);
    }
    if (pubsub_msg->msg.sender) {
        m_mem_unref((void *)pubsub_msg->msg.sender);
    }
}


static int tell_pubsub_msg(ps_priv_t *m, const mod_t *recipient, ctx_t *c) {    
    if (recipient) {
        tell_if(m, NULL, (mod_t *)recipient);
    } else if (!(m->flags & PS_GLOBAL)) {
        m_map_iterate(c->modules, tell_if, m);
    } else {
        m_map_iterate(ctx, tell_global, m);
    }
    return 0;
}

static int send_msg(mod_t *mod, const mod_t *recipient, const char *topic, 
                    const void *message, const mod_ps_flags flags) {
    M_PARAM_ASSERT(message);
    M_MOD_CTX(mod);
    
    mod->stats.sent_msgs++;
    fetch_ms(&mod->stats.last_seen, &mod->stats.action_ctr);
    ps_priv_t m = { { USER, mod, topic, message }, flags, NULL };
    return tell_pubsub_msg(&m, recipient, c);
}

/** Private API **/

int tell_system_pubsub_msg(const mod_t *recipient, ctx_t *c, ps_msg_type type, mod_t *sender, const char *topic) {
    if (sender) {
        fetch_ms(&sender->stats.last_seen, &sender->stats.action_ctr);
    }
    ps_priv_t m = { { type, sender, topic, NULL }, 0, NULL };
    return tell_pubsub_msg(&m, recipient, c);
}

int flush_pubsub_msgs(void *data, const char *key, void *value) {
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
        if (!data && m_mod_is(mod, RUNNING)) {
            M_DEBUG("Flushing enqueued pubsub message for module '%s'.\n", mod->name);
            msg_t msg = { .type = TYPE_PS, .ps_msg = &mm->msg };
            run_pubsub_cb(mod, &msg, mm->sub);
        } else {
            M_DEBUG("Destroying enqueued pubsub message for module '%s'.\n", mod->name);
            m_mem_unref(mm);
        }
    }
    return 0;
}

void run_pubsub_cb(mod_t *mod, msg_t *msg, const ev_src_t *src) {
    /* If module is using some different receive function, honor it. */
    recv_cb cb = m_stack_peek(mod->recvs);
    if (!cb) {
        /* Fallback to module default receive */
        cb = mod->hook.recv;
    }
    
    msg->self = mod;
    
    /* Notify underlying fuse fs */
    fs_notify(msg);
        
    /* Finally call user callback */
    cb(msg, src ? src->userptr : NULL);

    if (msg->type == TYPE_PS) {
        ps_priv_t *mm = (ps_priv_t *)msg->ps_msg;
        m_mem_unref(mm);
    }
    
    mod->stats.recv_msgs++;
    fetch_ms(&mod->stats.last_seen, &mod->stats.action_ctr);
}

/** Public API **/

int m_mod_ref(const mod_t *mod, const char *name, mod_t **modref) {
    M_MOD_ASSERT(mod);
    M_PARAM_ASSERT(name);
    M_PARAM_ASSERT(modref);
    M_PARAM_ASSERT(!*modref);
    M_CTX_MOD(mod->ctx, name);
    M_PARAM_ASSERT(mod->ctx == m->ctx);
    
    *modref = m_mem_ref(m);
    return 0;
}

int m_mod_unref(const mod_t *mod, mod_t **modref) {
    M_MOD_ASSERT(mod);
    M_PARAM_ASSERT(modref);
    M_PARAM_ASSERT(*modref);
    M_PARAM_ASSERT(mod->ctx == (*modref)->ctx);
    
    m_mem_unref(*modref);
    *modref = NULL;
    return 0;
}

int m_mod_register_sub(mod_t *mod, const char *topic, mod_src_flags flags, const void *userptr) {
    M_MOD_ASSERT(mod);
    M_PARAM_ASSERT(topic);
    
    /* Check if it is a valid regex: compile it */
    regex_t regex;
    int ret = regcomp(&regex, topic, REG_NOSUB);
    if (!ret) {
        M_DEBUG("'%s' is a valid regex.\n", topic);
        
        /* Lazy subscriptions map init */
        if (!mod->subscriptions)  {
            mod->subscriptions = m_map_new(M_MAP_VAL_ALLOW_UPDATE, m_mem_unref);
            M_ALLOC_ASSERT(mod->subscriptions);
        } else {
            ev_src_t *old_sub = m_map_get(mod->subscriptions, topic);
            if (old_sub) {
                if (old_sub->flags == flags) {
                    /* Only update userptr */
                    old_sub->userptr = userptr;
                    return 0;
                }
            }
        }
        
        /* Store new sub as ref'd memory */
        ev_src_t *sub = m_mem_new(sizeof(ev_src_t), subscribtions_dtor);
        M_ALLOC_ASSERT(sub);
        
        ps_src_t *ps_src = &sub->ps_src;
        sub->type = TYPE_PS;
        sub->flags = flags;
        sub->userptr = userptr;
        sub->mod = mod;
        memcpy(&ps_src->reg, &regex, sizeof(regex_t));
        ps_src->topic = sub->flags & SRC_DUP ? mem_strdup(topic) : topic;
        ret = m_map_put(mod->subscriptions, ps_src->topic, sub); // M_MAP_VAL_ALLOW_UPDATE -> this will dtor old elem before updating
        if (ret == 0) {
            fetch_ms(&mod->stats.last_seen, &mod->stats.action_ctr);
        }
    }
    return ret;
}

int m_mod_deregister_sub(mod_t *mod, const char *topic) {
    M_MOD_ASSERT(mod);
    M_PARAM_ASSERT(topic);
    
    int ret = m_map_remove(mod->subscriptions, topic);
    if (ret == 0) {
        fetch_ms(&mod->stats.last_seen, &mod->stats.action_ctr);
        if (m_map_length(mod->subscriptions) == 0) {
            m_map_free(&mod->subscriptions);
        }
    }
    return ret;
}

int m_mod_tell(mod_t *mod, const mod_t *recipient, const void *message, const mod_ps_flags flags) {
    M_MOD_ASSERT(mod);
    M_PARAM_ASSERT(recipient);
    /* only same ctx modules can talk */
    M_PARAM_ASSERT(mod->ctx == recipient->ctx);

    /* Eventually cleanup PS_GLOBAL flag */    
    return send_msg(mod, recipient, NULL, message, flags & ~PS_GLOBAL);
}

int m_mod_publish(mod_t *mod, const char *topic, const void *message, const mod_ps_flags flags) {
    M_MOD_ASSERT(mod);
    M_PARAM_ASSERT(topic);
    
    /* Eventually cleanup PS_GLOBAL flag */    
    return send_msg(mod, NULL, topic, message, flags & ~PS_GLOBAL);
}

int m_mod_broadcast(mod_t *mod, const void *message, const mod_ps_flags flags) {
    M_MOD_ASSERT(mod);
    M_PARAM_ASSERT(message);
    
    return send_msg(mod, NULL, NULL, message, flags);
}

int m_mod_poisonpill(mod_t *mod, const mod_t *recipient) {
    M_MOD_ASSERT(mod);
    M_PARAM_ASSERT(recipient);
    /* only same ctx modules can talk */
    M_PARAM_ASSERT(mod->ctx == recipient->ctx);
    M_PARAM_ASSERT(m_mod_is(recipient, RUNNING));
    M_MOD_CTX(mod);
    
    return tell_system_pubsub_msg(recipient, c, MODULE_POISONPILL, mod, NULL);
}
