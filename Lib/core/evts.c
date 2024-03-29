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

_public_ int m_mod_become(m_mod_t *mod, m_evt_cb new_on_evt) {
    M_PARAM_ASSERT(new_on_evt);
    M_MOD_ASSERT_STATE(mod, M_MOD_RUNNING);
    M_MOD_CONSUME_TOKEN(mod);

    return m_stack_push(mod->recvs, new_on_evt);;
}

_public_ int m_mod_unbecome(m_mod_t *mod) {
    M_MOD_ASSERT_STATE(mod, M_MOD_RUNNING);
    M_MOD_CONSUME_TOKEN(mod);

    if (m_stack_pop(mod->recvs) != NULL) {
        return 0;
    }
    return -EINVAL;
}

_public_ int m_mod_stash(m_mod_t *mod, const m_evt_t *evt) {
    M_MOD_ASSERT_STATE(mod, M_MOD_RUNNING);
    M_PARAM_ASSERT(evt);
    M_MOD_CONSUME_TOKEN(mod);
    
    evt_priv_t *priv_evt = (evt_priv_t *)evt;
    m_src_flags prio_flags = 0;
    if (priv_evt->src) {
        prio_flags = priv_evt->src->flags & M_SRC_PRIO_MASK;
    }
    // Cannot stash HIGH priority evts!
    M_RET_ASSERT(!(prio_flags & M_SRC_PRIO_HIGH), -EPERM);

    return m_queue_enqueue(mod->stashed, m_mem_ref((void *)evt));;
}

_public_ ssize_t m_mod_unstash(m_mod_t *mod, size_t len) {
    M_MOD_ASSERT_STATE(mod, M_MOD_RUNNING);
    M_PARAM_ASSERT(len > 0);
    M_MOD_CONSUME_TOKEN(mod);

    m_queue_t *unstashed = m_queue_new(mem_dtor);
    M_ALLOC_ASSERT(unstashed);

    m_itr_foreach(mod->stashed, {
        if (m_idx + 1 == len) {
            memhook._free(m_itr);
            break;
        }
        m_evt_t *evt = m_itr_get(m_itr);
        /*
         * Here evt has 1 ref; call_pubsub_cb() would drop it
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
    M_MOD_CONSUME_TOKEN(mod);
    
    mod->batch.len = len;
    return 0;
}

_public_ int m_mod_set_batch_timeout(m_mod_t *mod, uint64_t timeout_ns) {
    M_MOD_ASSERT(mod);

    // src_deregister and src_register already consume a token

    /* If it was already set, remove old timer */
    if (mod->batch.timer.ns != 0) {
        m_mod_src_deregister_tmr(mod, &mod->batch.timer);
    }
    mod->batch.timer.clock_id = CLOCK_MONOTONIC;
    mod->batch.timer.ns = timeout_ns;
    if (timeout_ns != 0) {
        // If batching by size is disabled
        if (mod->batch.len == 0) {
            // Set a maximum value for batching so that only timed batching will be effective
            mod->batch.len = SIZE_MAX;
        }
        return m_mod_src_register_tmr(mod, &mod->batch.timer, M_SRC_INTERNAL | M_SRC_PRIO_HIGH, &mod->batch);
    }
    return 0;
}
