#pragma once

#include <stdlib.h>
#include <regex.h>
#include <string.h>
#include <errno.h>
#include "mem.h"
#include "thpool.h"
#include "itr.h"

#ifndef NDEBUG
    #define M_DEBUG printf("Libmodule @ %s:%d| ", __func__, __LINE__); printf
#else
    #define M_DEBUG (void)
#endif

#define M_ASSERT(cond, msg, ret)    if (!(cond)) { M_DEBUG("%s\n", msg); return ret; }

#define M_RET_ASSERT(cond, ret)     M_ASSERT(cond, "("#cond ") condition failed.", ret) 

#define M_ALLOC_ASSERT(cond)        M_RET_ASSERT(cond, -ENOMEM);
#define M_PARAM_ASSERT(cond)        M_RET_ASSERT(cond, -EINVAL);
#define M_TH_ASSERT(ctx)            M_RET_ASSERT(ctx->th_id == pthread_self(), -EPERM);

#define M_CTX_ASSERT(c) \
    M_PARAM_ASSERT(c); \
    M_TH_ASSERT(c);

#define M_MOD_ASSERT(mod) \
    M_PARAM_ASSERT(mod); \
    M_TH_ASSERT(mod->ctx); \
    M_RET_ASSERT(!m_mod_is(mod, ZOMBIE), -EACCES);
    
#define M_MOD_ASSERT_STATE(mod, state) \
    M_MOD_ASSERT(mod); \
    M_RET_ASSERT(m_mod_is(mod, state), -EACCES);
    
#define M_MOD_CTX(mod)    ctx_t *c = mod->ctx;
    
#define M_CTX_MOD(ctx, name) \
    mod_t *m = m_map_get(ctx->modules, (char *)name); \
    M_RET_ASSERT(m, -ENOENT);

typedef struct _src ev_src_t;

/* Struct that holds fds to self_t mapping for poll plugin */
typedef struct {
    int fd;
} fd_src_t;

/* Struct that holds timers to self_t mapping for poll plugin */
typedef struct {
#ifdef __linux__
    fd_src_t f;
#endif
    mod_tmr_t its;
} tmr_src_t;

/* Struct that holds signals to self_t mapping for poll plugin */
typedef struct {
#ifdef __linux__
    fd_src_t f;
#endif
    mod_sgn_t sgs;
} sgn_src_t;

/* Struct that holds paths to self_t mapping for poll plugin */
typedef struct {
    fd_src_t f; // in kqueue EVFILT_VNODE: open(path) is needed. Thus a fd is needed too.
    mod_path_t pt;
} path_src_t;

/* Struct that holds pids to self_t mapping for poll plugin */
typedef struct {
#ifdef __linux__
    fd_src_t f;
#endif
    mod_pid_t pid;
} pid_src_t;

/* Struct that holds task to self_t mapping for poll plugin */
typedef struct {
#ifdef __linux__
    fd_src_t f;
#endif
    mod_task_t tid;
    pthread_t th;
    int retval;
} task_src_t;

/* Struct that holds pubsub subscriptions source data */
typedef struct {
    regex_t reg;
    const char *topic;
} ps_src_t;

/* Struct that holds generic "event source" data */
struct _src {
    union {
        ps_src_t    ps_src;
        fd_src_t    fd_src;
        tmr_src_t   tmr_src;
        sgn_src_t   sgn_src;
        path_src_t  path_src;
        pid_src_t   pid_src;
        task_src_t  task_src;
    };
    m_src_types type;
    m_src_flags flags;
    void *ev;                               // poll plugin defined data structure
    mod_t *mod;                             // ptr needed to map an event source to a self_t in poll_plugin
    const void *userptr;
};

/* Struct that holds pubsub messaging, private. It keeps reference count */
typedef struct {
    ps_msg_t msg;
    m_ps_flags flags;
    ev_src_t *sub;
} ps_priv_t;

typedef struct {
    void *data;                             // Context's poll priv data (depends upon poll_plugin)
    int max_events;                         // Max number of returned events for poll_plugin
} poll_priv_t;

typedef struct {
    uint64_t registration_time;
    uint64_t last_seen;
    uint64_t action_ctr;
    uint64_t sent_msgs;
    uint64_t recv_msgs;
} mod_stats_t;

/* Struct that holds data for each module */
struct _mod {
    mod_states state;                       // module's state
    m_mod_flags flags;                      // Module's flags
    int pubsub_fd[2];                       // In and Out pipe for pubsub msg
    mod_stats_t stats;                      // Modules' stats
    userhook_t hook;                        // module's user defined callbacks
    m_stack_t *recvs;                       // Stack of recv functions for module_become/unbecome (stack of funpointers)
    const void *userdata;                   // module's user defined data
    void *fs;                               // FS module priv data. NULL if unsupported
    const char *name;                       // module's name
    const char *local_path;                 // For runtime loaded modules: path of module
    m_bst_t *srcs[M_SRC_TYPE_END];          // module's event sources
    m_map_t *subscriptions;                 // module's subscriptions (map of ev_src_t*)
    ctx_t *ctx;                             // Module's ctx
};

/* Struct that holds data for each context */
struct _ctx {
    const char *name;                       // Context's name'
    bool looping;                           // Whether context is looping
    bool quit;                              // Context's quit flag
    uint8_t quit_code;                      // Context's quit code, returned by modules_ctx_loop()
    log_cb logger;                          // Context's log callback
    m_map_t *modules;                       // Context's modules
    poll_priv_t ppriv;                      // Priv data for poll_plugin implementation
    m_ctx_flags flags;                      // Context's flags
    pthread_t th_id;                        // Main context's thread
    void *fs;                               // FS context handler. Null if unsupported
};

/* Defined in module.c */
int evaluate_module(void *data, const char *key, void *value);
int start(mod_t *mod, const bool starting);
int stop(mod_t *mod, const bool stopping);

/* Defined in context.c */
ctx_t *check_ctx(const char *ctx_name);
void ctx_logger(const ctx_t *c, const mod_t *mod, const char *fmt, ...);

/* Defined in pubsub.c */
int tell_system_pubsub_msg(const mod_t *recipient, ctx_t *c, ps_msg_type type, 
                           mod_t *sender, const char *topic);
int flush_pubsub_msgs(void *data, const char *key, void *value);
void run_pubsub_cb(mod_t *mod, msg_t *msg, const ev_src_t *src);

/* Defined in utils.c */
char *mem_strdup(const char *s);
void fetch_ms(uint64_t *val, uint64_t *ctr);

/* Defined in map.c */
void *map_peek(const m_map_t *m);

/* Gglobal variables are defined in modules.c */
extern m_map_t *ctx;
extern memhook_t memhook;
extern pthread_mutex_t mx;
