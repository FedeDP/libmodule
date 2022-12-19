#pragma once

#include "public/module/ctx.h"
#include "public/module/structs/map.h"
#include "public/module/thpool/thpool.h"
#include "globals.h"

#define M_CTX_DEFAULT_EVENTS    64

#define M_CTX_ASSERT(c) \
    M_PARAM_ASSERT(c); \
    M_RET_ASSERT(c->state != M_CTX_ZOMBIE, -EACCES)

typedef struct {
    void *data;                             // Context's poll priv data (depends upon poll_plugin)
    int max_events;                         // Max number of returned events for poll_plugin
} poll_priv_t;

typedef struct {
    uint64_t looping_start_time;
    uint64_t idle_time;
    uint64_t recv_msgs;
    size_t running_modules;
} ctx_stats_t;

/* Ctx states */
typedef enum {
    M_CTX_IDLE,
    M_CTX_LOOPING,
    M_CTX_ZOMBIE,
} m_ctx_states;

typedef struct {
    m_src_tmr_t tmr;
    ev_src_t *src;
} ctx_tick_t;

/* Struct that holds data for context */
/*
 * MEM-REFS for ctx:
 * + 1 because it is registered
 * + 1 for each module registered in the context
 *      (thus it won't be actually destroyed until any module is inside it)
 */
struct _ctx {
    CONST const char *name;
    m_ctx_states state;
    bool quit;                              // Context's quit flag
    uint8_t quit_code;                      // Context's quit code, returned by modules_ctx_loop()
    bool finalized;                         // Whether the context is finalized, ie: no more modules can be registered
    m_log_cb logger;                        // Context's log callback
    m_map_t *modules;                       // Context's modules
    poll_priv_t ppriv;                      // Priv data for poll_plugin implementation
    CONST m_ctx_flags flags;                // Context's flags
    void *fs;                               // FS context handler. Null if unsupported
    ctx_stats_t stats;                      // Context' stats
    m_thpool_t  *thpool;                    // thpool for M_SRC_TYPE_TASK srcs; lazily created
    ctx_tick_t tick;                        // Tick for ctx sending a M_PS_CTX_TICK message
    CONST const void *userdata;             // Context's user defined data
};

int ctx_new(const char *ctx_name, m_ctx_t **c, m_ctx_flags flags, const void *userdata);
m_ctx_t *check_ctx(const char *ctx_name);
void ctx_logger(const m_ctx_t *c, const m_mod_t *mod, const char *fmt, ...);
