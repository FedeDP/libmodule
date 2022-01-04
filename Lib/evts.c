#include "priv.h"

/************************************
 * Code related to events handling. *
 ************************************/

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
    // Cannot stash FD evts: it would cause an infinite loop polling on it
    M_PARAM_ASSERT(evt->type != M_SRC_TYPE_FD);

    int ret = m_queue_enqueue(mod->stashed, m_mem_ref((void *)evt));
    if (ret == 0) {
        fetch_ms(&mod->stats.last_seen, &mod->stats.action_ctr);
    }
    return ret;
}

_public_ ssize_t m_mod_unstash(m_mod_t *mod, size_t len) {
    M_MOD_ASSERT(mod);
    M_PARAM_ASSERT(len > 0);

    m_queue_t *unstashed __attribute__((__cleanup__(m_queue_free))) = m_queue_new(mem_dtor);
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
