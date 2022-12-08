#include "ps.h"
#include "fs.h"
#include "evts.h"
#include "ctx.h"

/******************************************
 * Code related to Actor-like PubSub API. *
 ******************************************/

static void subscribtions_dtor(void *data);
static ev_src_t *fetch_sub(m_mod_t *mod, const char *topic);
static inline bool is_system_message(const char *topic);
static int tell_if(void *data, const char *key, void *value);
static ps_priv_t *alloc_ps_msg(const ps_priv_t *msg, ev_src_t *sub);
static void ps_msg_dtor(void *data);
static void tell_subscribers(void *data, void *value);
static int tell_pubsub_msg(ps_priv_t *m, const m_mod_t *recipient, m_ctx_t *c);
static int send_msg(m_mod_t *mod, const m_mod_t *recipient, const char *topic, 
                    const void *message, m_ps_flags flags);

static void subscribtions_dtor(void *data) {
    ev_src_t *sub = (ev_src_t *)data;
    regfree(&sub->ps_src.reg);
    if (sub->flags & M_SRC_DUP) {
        memhook._free((void *)sub->ps_src.topic);
    }
    if (sub->flags & M_SRC_AUTOFREE) {
        memhook._free((void *)sub->userptr);
    }
}

static ev_src_t *fetch_sub(m_mod_t *mod, const char *topic) {
    /* Check if module is directly subscribed to topic */
    ev_src_t *sub = m_map_get(mod->subscriptions, topic);
    if (sub) {
        goto found;
    }
    
    /* Check if any stored subscriptions is a regex that matches topic */
    m_itr_foreach(mod->subscriptions, {
        sub = m_itr_get(m_itr);
        /* Execute regular expression */
        int ret = regexec(&sub->ps_src.reg, topic, 0, NULL, 0);
        if (!ret) {
            memhook._free(m_itr);
            goto found;
        }
    });
    return NULL;
    
found:
    return sub;
}

static inline bool is_system_message(const char *topic) {
    return topic && strncmp(topic, "LIBMODULE_", strlen("LIBMODULE_")) == 0;
}

/* 
 * Note: we cannot use m_mod_is() here as this function may be called
 * from another ctx when M_PS_GLOBAL is set on a broadcast message,
 * and it would fail because it is being called by different thread.
 */
static int tell_if(void *data, const char *key, void *value) {
    m_mod_t *mod = (m_mod_t *)value;
    ps_priv_t *msg = (ps_priv_t *)data;
    ev_src_t *sub = msg->msg.topic ? (ev_src_t *)key : NULL;                 // key is indeed a subscription when we are publishing (check tell_subscribers()) !!

    if (mod->state & (M_MOD_RUNNING | M_MOD_PAUSED) &&                       // mod is running or paused
        (!msg->msg.topic || sub)) {                                          // it is a publish and mod is subscribed on topic, or it is a broadcast/direct tell message

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
        m->msg.sender = m_mem_ref((void *)m->msg.sender); // keep module alive until message is dispatched
        m->sub = sub;
    }
    return m;
}

static void ps_msg_dtor(void *data) {
    ps_priv_t *pubsub_msg = (ps_priv_t *)data;
    
    if (pubsub_msg->flags & M_PS_AUTOFREE) {
        memhook._free((void *)pubsub_msg->msg.data);
    }
    if (pubsub_msg->msg.sender) {
        m_mem_unref((void *)pubsub_msg->msg.sender);
    }
}

static void tell_subscribers(void *data, void *value) {
    m_ctx_t *c = (m_ctx_t *)value;
    ps_priv_t *msg = (ps_priv_t *)data;
    
    m_itr_foreach(c->modules, {
        m_mod_t *mod = m_itr_get(m_itr);
        ev_src_t *sub = NULL;
        
        if (m_mod_is(mod, M_MOD_RUNNING | M_MOD_PAUSED) && (sub = fetch_sub(mod, msg->msg.topic))) {
            tell_if(msg, (char *)sub, mod);
        }
    });
}

static int tell_pubsub_msg(ps_priv_t *m, const m_mod_t *recipient, m_ctx_t *c) {
    if (recipient) { // it is a direct tell
        tell_if(m, NULL, (m_mod_t *)recipient);
    } else {
        /* Broadcast messages */
        if (!m->msg.topic) {
            m_map_iterate(c->modules, tell_if, m);
        } else {
            tell_subscribers(m, c);
        }
    }
    return 0;
}

static int send_msg(m_mod_t *mod, const m_mod_t *recipient, const char *topic, 
                    const void *message, m_ps_flags flags) {
    M_PARAM_ASSERT(message);

    mod->stats.sent_msgs++;
    ps_priv_t m = { { false, mod, topic, message }, flags, NULL };
    return tell_pubsub_msg(&m, recipient, mod->ctx);
}

/** Private API **/

int tell_system_pubsub_msg(const m_mod_t *recipient, m_ctx_t *c, m_mod_t *sender, const char *topic) {
    if (sender) {
        // A module sent a M_PS_MOD_POISONPILL message to another, or it was stopped
        sender->stats.sent_msgs++;
    }
    ps_priv_t m = { { true, sender, topic, NULL }, 0, NULL };
    return tell_pubsub_msg(&m, recipient, c);
}

int flush_pubsub_msgs(void *data, const char *key, void *value) {
    m_mod_t *mod = (m_mod_t *)value;
    ps_priv_t *mm = NULL;

    const bool stopping_mod = key == NULL;
    
    m_queue_t *flushed = m_queue_new(mem_dtor);
    if (!flushed) {
        M_WARN("Failed to create flushing queue.\n");
    }

    while (mod->pubsub_fd[0] != -1 &&
        read(mod->pubsub_fd[0], &mm, sizeof(ps_priv_t *)) == sizeof(ps_priv_t *)) {
        /*
         * Actually tell msg ONLY if we are not stopping the module,
         * ie: we are stopping looping on the context.
         * Else, just free msg.
         */
        if (!stopping_mod && m_mod_is(mod, M_MOD_RUNNING)) {
            M_DEBUG("Flushing enqueued pubsub message for module '%s'.\n", mod->name);
            evt_priv_t *msg = new_evt(mm->sub);
            if (msg && flushed) {
                msg->evt.ps_evt = &mm->msg;
                m_queue_enqueue(flushed, msg);
                continue;
            }
        }
        M_DEBUG("Destroying enqueued pubsub message for module '%s'.\n", mod->name);
        m_mem_unref(mm);
    }
    call_pubsub_cb(mod, flushed);
    
    /* 
     * If we are stopping the ctx loop,
     * advise fuse fs that the ctx is stoppped
     * and all clients must be notified and freed.
     */
    if (!stopping_mod) {
        fs_ctx_stopped(mod);
    }
    return 0;
}

void call_pubsub_cb(m_mod_t *mod, m_queue_t *evts) {
    if (m_queue_len(evts) == 0) {
        goto end;
    }
    
    /* If module is using some different receive function, honor it. */
    m_evt_cb cb = m_stack_peek(mod->recvs);
    if (!cb) {
        /* Fallback to module default receive */
        cb = mod->hook.on_evt;
    }
    
    /* Notify underlying fuse fs */
    fs_notify(mod, evts);
    
    /* Finally call user callback */
    cb(mod, evts);
    
    mod->stats.recv_msgs += m_queue_len(evts);
    
    fetch_ms(&mod->stats.last_seen, NULL);
    
end:
    /* Destroy events */
    m_queue_free(&evts);
}

/** Public API **/

_public_ int m_mod_ps_subscribe(m_mod_t *mod, const char *topic, m_src_flags flags, const void *userptr) {
    M_MOD_ASSERT_PERM(mod, M_MOD_DENY_SUB);
    M_PARAM_ASSERT(topic);
    M_SRC_ASSERT_PRIO_FLAGS();
    M_MOD_CONSUME_TOKEN(mod);

    /* Check if it is a valid regex: compile it */
    regex_t regex;
    int ret = regcomp(&regex, topic, REG_NOSUB);
    if (ret == 0) {
        M_DEBUG("'%s' is a valid regex.\n", topic);
        
        /* Lazy subscriptions map init */
        if (!mod->subscriptions)  {
            mod->subscriptions = m_map_new(M_MAP_VAL_ALLOW_UPDATE, mem_dtor);
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
        sub->type = M_SRC_TYPE_PS;
        sub->flags = flags;
        sub->userptr = userptr;
        sub->mod = mod;
        memcpy(&ps_src->reg, &regex, sizeof(regex_t));
        ps_src->topic = sub->flags & M_SRC_DUP ? mem_strdup(topic) : topic;
        ret = m_map_put(mod->subscriptions, ps_src->topic, sub); // M_MAP_VAL_ALLOW_UPDATE -> this will dtor old elem before updating
    }
    return ret;
}

_public_ int m_mod_ps_unsubscribe(m_mod_t *mod, const char *topic) {
    M_MOD_ASSERT_PERM(mod, M_MOD_DENY_SUB);
    M_PARAM_ASSERT(topic);
    M_MOD_CONSUME_TOKEN(mod);
    
    int ret = m_map_remove(mod->subscriptions, topic);
    if (ret == 0) {
        if (m_map_len(mod->subscriptions) == 0) {
            m_map_free(&mod->subscriptions);
        }
    }
    return ret;
}

_public_ int m_mod_ps_tell(m_mod_t *mod, const m_mod_t *recipient, const void *message, m_ps_flags flags) {
    M_MOD_ASSERT_PERM(mod, M_MOD_DENY_PUB);
    M_PARAM_ASSERT(recipient);
    /* only same ctx modules can talk */
    M_PARAM_ASSERT(mod->ctx == recipient->ctx);
    M_MOD_CONSUME_TOKEN(mod);

    return send_msg(mod, recipient, NULL, message, flags);
}

_public_ int m_mod_ps_publish(m_mod_t *mod, const char *topic, const void *message, m_ps_flags flags) {
    M_MOD_ASSERT_PERM(mod, M_MOD_DENY_PUB);
    M_RET_ASSERT(!is_system_message(topic), -EPERM);
    M_MOD_CONSUME_TOKEN(mod);
    
    return send_msg(mod, NULL, topic, message, flags);
}

_public_ int m_mod_ps_poisonpill(m_mod_t *mod, const m_mod_t *recipient) {
    M_MOD_ASSERT_PERM(mod, M_MOD_DENY_PUB);
    M_PARAM_ASSERT(recipient);
    /* only same ctx modules can talk */
    M_PARAM_ASSERT(mod->ctx == recipient->ctx);
    M_PARAM_ASSERT(m_mod_is(recipient, M_MOD_RUNNING));
    M_MOD_CONSUME_TOKEN(mod);

    return tell_system_pubsub_msg(recipient, mod->ctx, mod, M_PS_MOD_POISONPILL);
}
