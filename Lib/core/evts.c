#include "evts.h"
#include "ps.h"
#include "ctx.h"

/************************************
 * Code related to events handling. *
 ************************************/

static void evt_dtor(void *data) {
    evt_priv_t *evt = (evt_priv_t *)data;
    m_mem_unref(evt->src);
    /* We use fd_evt as all messages share address inside union */
    m_mem_unref(evt->evt.fd_evt);
}

/** Private API **/

evt_priv_t *new_evt(ev_src_t *src) {
    evt_priv_t *msg = m_mem_new(sizeof(evt_priv_t), evt_dtor);
    if (msg) {
        msg->evt.type = src->type;
        msg->src = m_mem_ref(src);
    }
    return msg;
}

/** Public API **/

/* Must be unref through m_mem_unref() */
_public_ m_mod_t *m_mod_ref(const m_mod_t *mod, const char *name) {
    M_RET_ASSERT(mod, NULL);
    M_RET_ASSERT(name, NULL);
    M_MOD_CTX(mod);

    m_mod_t *m = m_map_get(c->modules, name);
    return m_mem_ref(m);
}

_public_ int m_mod_become(m_mod_t *mod, m_evt_cb new_on_evt) {
    M_PARAM_ASSERT(new_on_evt);
    M_MOD_ASSERT_STATE(mod, M_MOD_RUNNING);

    int ret = m_stack_push(mod->recvs, new_on_evt);
    if (ret == 0) {
        fetch_ms(&mod->stats.last_seen, &mod->stats.action_ctr);
    }
    return ret;
}

_public_ int m_mod_unbecome(m_mod_t *mod) {
    M_MOD_ASSERT_STATE(mod, M_MOD_RUNNING);

    if (m_stack_pop(mod->recvs) != NULL) {
        fetch_ms(&mod->stats.last_seen, &mod->stats.action_ctr);
        return 0;
    }
    return -EINVAL;
}

_public_ int m_mod_stash(m_mod_t *mod, const m_evt_t *evt) {
    M_MOD_ASSERT(mod);
    M_PARAM_ASSERT(evt);
    
    evt_priv_t *priv_evt = (evt_priv_t *)evt;
    m_src_flags prio_flags = 0;
    if (priv_evt->src) {
        prio_flags = priv_evt->src->flags & M_SRC_PRIO_MASK;
    }
    // Cannot stash HIGH priority evts!
    M_RET_ASSERT(!(prio_flags & M_SRC_PRIO_HIGH), -EPERM);

    int ret = m_queue_enqueue(mod->stashed, m_mem_ref((void *)evt));
    if (ret == 0) {
        fetch_ms(&mod->stats.last_seen, &mod->stats.action_ctr);
    }
    return ret;
}

_public_ ssize_t m_mod_unstash(m_mod_t *mod, size_t len) {
    M_MOD_ASSERT(mod);
    M_PARAM_ASSERT(len > 0);

    m_queue_t *unstashed = m_queue_new(mem_dtor);
    M_ALLOC_ASSERT(unstashed);

    m_itr_foreach(mod->stashed, {
        if (m_idx + 1 == len) {
            memhook._free(m_itr);
            break;
        }
        m_evt_t *evt = m_itr_get(m_itr);
        /*
         * Here evt has 1 ref; run_pubsub_cb() would drop it
         * thus invalidating ptr.
         * But m_itr_rm() still needs ptrs!
         * Keep evts alive.
         */
        m_queue_enqueue(unstashed, m_mem_ref(evt));
        m_itr_rm(m_itr);
    });

    const size_t processed = m_queue_len(unstashed);
    call_pubsub_cb(mod, unstashed);
    return processed;
}

_public_ int m_mod_set_batch_size(m_mod_t *mod, size_t len) {
    M_MOD_ASSERT(mod);
    
    mod->batch.len = len;
    return 0;
}

_public_ int m_mod_set_batch_timeout(m_mod_t *mod, uint64_t timeout_ms) {
    M_MOD_ASSERT(mod);

    /* If it was already set, remove old timer */
    if (mod->batch.timer.ms != 0) {
        deregister_src(mod, M_SRC_TYPE_TMR, &mod->batch.timer);
    }
    mod->batch.timer.clock_id = CLOCK_MONOTONIC;
    mod->batch.timer.ms = timeout_ms;
    if (timeout_ms != 0) {
        // If batching by size was disabled
        if (mod->batch.len == 0) {
            // Set a maximum value for batching so that only timed batching will be effective
            mod->batch.len = -1;
        }
        return register_src(mod, M_SRC_TYPE_TMR, &mod->batch.timer, M_SRC_INTERNAL | M_SRC_PRIO_HIGH, NULL);
    }
    return 0;
}