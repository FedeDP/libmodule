#pragma once

#include "public/module/mod.h"
#include "public/module/mem.h"
#include "priv.h"

#define M_MOD_CTX(mod)    m_ctx_t *c = mod->ctx;

#define M_MOD_ASSERT(mod) \
    M_PARAM_ASSERT(mod); \
    M_RET_ASSERT(!m_mod_is(mod, M_MOD_ZOMBIE), -EACCES)

#define M_MOD_ASSERT_PERM(mod, perm) \
    M_MOD_ASSERT(mod); \
    M_RET_ASSERT(!(mod->flags & perm), -EPERM)

#define M_MOD_ASSERT_STATE(mod, state) \
    M_MOD_ASSERT(mod); \
    M_RET_ASSERT(m_mod_is(mod, state), -EACCES)

typedef struct {
    uint64_t registration_time;
    uint64_t last_seen;
    uint64_t action_ctr;
    uint64_t sent_msgs;
    uint64_t recv_msgs;
} mod_stats_t;

typedef struct {
    size_t len;
    m_src_tmr_t timer;
    m_queue_t *events;
} mod_batch_t;

/* Struct that holds data for each module */
/*
 * MEM-REFS for mod:
 * + 1 because it is registered
 * + 1 for each m_mod_ref() called on it (included m_mod_register() when mod_ref != NULL)
 * + 1 for each PS message sent (ie: message's sender is a new reference for sender)
 * + 1 for each fs open() call on it
 * Moreover, a reference is held while retrieving an event for the module and calling its on_evt() cb,
 * to avoid user calling m_mod_deregister() and invalidating the pointer.
 */
struct _mod {
    m_mod_states state;                     // module's state
    m_mod_flags flags;                      // Module's flags
    int pubsub_fd[2];                       // In and Out pipe for pubsub msg
    mod_stats_t stats;                      // Module's stats
    m_mod_hook_t hook;                      // module's user defined callbacks
    m_stack_t *recvs;                       // Stack of recv functions for module_become/unbecome (stack of funpointers)
    const void *userdata;                   // module's user defined data
    void *fs;                               // FS module priv data. NULL if unsupported
    const char *name;                       // module's name
    mod_batch_t batch;                      // Events' batching informations
    void *dlhandle;                         // For plugins
    const char *plugin_path;                // Filesystem path for plugins
    m_bst_t *srcs[M_SRC_TYPE_END];          // module's event sources
    m_map_t *subscriptions;                 // module's subscriptions (map of ev_src_t*)
    m_queue_t *stashed;                     // module's stashed messages
    m_ctx_t *ctx;                           // Module's ctx
};

int evaluate_module(void *data, const char *key, void *value);
int start(m_mod_t *mod, bool starting);
int stop(m_mod_t *mod, bool stopping);
int mod_deregister(m_mod_t **mod, bool from_user);
