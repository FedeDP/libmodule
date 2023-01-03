#pragma once

#include "public/module/mod.h"
#include "globals.h"

#define M_MOD_CTX(mod) \
    m_ctx_t *c = mod->ctx;

#define M_MOD_ASSERT(mod) \
    M_PARAM_ASSERT(mod); \
    M_RET_ASSERT(!m_mod_is(mod, M_MOD_ZOMBIE), -EACCES); \
    M_RET_ASSERT(mod->ctx == m_ctx(), -EPERM)

#define M_MOD_ASSERT_PERM(mod, perm) \
    M_MOD_ASSERT(mod); \
    M_RET_ASSERT(!(mod->flags & perm), -EPERM)

#define M_MOD_ASSERT_STATE(mod, state) \
    M_MOD_ASSERT(mod); \
    M_RET_ASSERT(m_mod_is(mod, state), -EACCES)
    
#define M_MOD_CONSUME_TOKEN(mod) \
    M_RET_ASSERT(mod->tb.tokens > 0, -EAGAIN) \
    mod->tb.tokens--; \
    fetch_ms(&mod->stats.last_seen, &mod->stats.action_ctr);

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

typedef struct {
    uint16_t rate;
    uint64_t burst;
    uint64_t tokens;
    m_src_tmr_t timer;
} mod_tb_t;

/* Forward declare ctx handler */
typedef struct _ctx m_ctx_t;

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
    CONST m_mod_flags flags;                // Module's flags
    int pubsub_fd[2];                       // In and Out pipe for pubsub msg
    mod_stats_t stats;                      // Module's stats
    CONST m_mod_hook_t hook;                // module's user defined callbacks
    m_stack_t *recvs;                       // Stack of recv functions for module_become/unbecome (stack of funpointers)
    CONST const void *userdata;             // module's user defined data
    void *fs;                               // FS module priv data. NULL if unsupported
    CONST const char *name;                 // module's name
    mod_batch_t batch;                      // Events' batching informations
    mod_tb_t tb;                            // Mod's tockenbucket
    CONST void *dlhandle;                   // Handle for plugin (NULL if not a plugin)
    m_bst_t *srcs[M_SRC_TYPE_END];          // module's event sources
    m_map_t *subscriptions;                 // module's subscriptions (map of ev_src_t*)
    m_queue_t *stashed;                     // module's stashed messages
    m_list_t *bound_mods;                   // modules that are bound to this module's state
    CONST m_ctx_t *ctx;                     // Module's ctx -> even if ctx is threadspecific data, we need to know the context a module was registered into, to avoid user passing modules around to another thread/context
};

int evaluate_module(void *data, const char *key, void *value);
int start(m_mod_t *mod, bool starting);
int stop(m_mod_t *mod, bool stopping);
int mod_deregister(m_mod_t **mod, bool from_user);
void mod_dump(const m_mod_t *mod, bool log_mod, const char *indent);
