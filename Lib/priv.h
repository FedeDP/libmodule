#pragma once

#include <stdlib.h>
#include <regex.h>
#include "map.h"
#include "stack.h"
#include "list.h"

#ifndef NDEBUG
    #define MODULE_DEBUG printf("Libmodule @ %s:%d| ", __func__, __LINE__); printf
#else
    #define MODULE_DEBUG (void)
#endif

#define MOD_ASSERT(cond, msg, ret)  if (!(cond)) { MODULE_DEBUG("%s\n", msg); return ret; }

#define MOD_RET_ASSERT(cond, ret)   MOD_ASSERT(cond, "("#cond ") condition failed.", ret) 

#define MOD_ALLOC_ASSERT(cond)      MOD_RET_ASSERT(cond, MOD_NO_MEM);
#define MOD_PARAM_ASSERT(cond)      MOD_RET_ASSERT(cond, MOD_WRONG_PARAM);

/* Finds a ctx inside our global map, given its name */
#define FIND_CTX(name) \
    MOD_ASSERT(name, "NULL ctx.", MOD_ERR); \
    ctx_t *c = map_get(ctx, (char *)name); \
    MOD_ASSERT(c, "Context not found.", MOD_NO_CTX);

#define GET_CTX_PRIV(self)  ctx_t *c = self->ctx
/* 
 * Convenience macro to retrieve self->ctx + doing some checks.
 * Skip reference check for pure functions.
 */
#define _GET_CTX(self, pure) \
    MOD_ASSERT((self), "NULL self handler.", MOD_NO_SELF); \
    MOD_ASSERT(!(self)->is_ref || pure, "Self is a reference object. It does not own module.", MOD_REF_ERR); \
    GET_CTX_PRIV((self)); \
    MOD_ASSERT(c, "Context not found.", MOD_NO_CTX);

/* Convenience macros for exposed API */
#define GET_CTX(self)       _GET_CTX(self, false)
#define GET_CTX_PURE(self)  _GET_CTX(self, true)

/* Convenience macro to retrieve a module from a context, given its name */
#define CTX_GET_MOD(name, ctx) \
    mod_t *mod = map_get(ctx->modules, (char *)name); \
    MOD_ASSERT(mod, "Module not found.", MOD_NO_MOD);

#define GET_MOD_PRIV(self) mod_t *mod = self->mod

/* 
 * Convenience macro to retrieve self->mod + doing some checks.
 * Skip reference check for pure functions.
 */
#define _GET_MOD(self, pure) \
    MOD_ASSERT((self), "NULL self handler.", MOD_NO_SELF); \
    MOD_ASSERT(!(self)->is_ref || pure, "Self is a reference object. It does not own module.", MOD_REF_ERR); \
    GET_MOD_PRIV((self)); \
    MOD_ASSERT(mod, "Module not found.", MOD_NO_MOD);

/* Convenience macros for exposed API */
#define GET_MOD(self)         _GET_MOD(self, false)
#define GET_MOD_PURE(self)    _GET_MOD(self, true)

/*
 * Convenience macro to retrieve self->mod + doing some checks 
 * + checking if its state matches required state 
 */
#define GET_MOD_IN_STATE(self, state) \
    GET_MOD(self); \
    MOD_ASSERT(_module_is(mod, state), "Wrong module state.", MOD_WRONG_STATE);
    
typedef struct _module mod_t;
typedef struct _context ctx_t;

/* Struct that holds self module informations, static to each module */
struct _self {
    mod_t *mod;                             // self's mod
    ctx_t *ctx;                             // self's ctx
    bool is_ref;                            // is this a reference?
};

/* List that holds fds to self_t mapping for epoll/kqueue, ie: socket source data */
typedef struct {
    int fd;                                 // file descriptor polled by main loop
} fd_src_t;

/* List that holds fds to self_t mapping for epoll/kqueue, ie: socket source data */
typedef struct {
    mod_timer_t its;                    
    fd_src_t f;
} timer_src_t;

/* List that holds fds to self_t mapping for epoll/kqueue, ie: socket source data */
typedef struct {
    mod_sgn_t sgs;                         
    fd_src_t f;                                 
} sgn_src_t;

/* Struct that holds pubsub subscriptions source data */
typedef struct {
    regex_t reg;
    const char *topic;
} ps_src_t;

/* Struct that holds generic "event source" data */
typedef struct {
    union {
        ps_src_t ps_src;
        fd_src_t fd_src;
        timer_src_t tm_src;
        sgn_src_t sgn_src;
    };
    mod_src_type type;
    void *ev;                               // poll plugin defined data structure (used by kqueue and epoll)
    const self_t *self;                     // ptr needed to map a fd to a self_t in epoll
    mod_src_flags flags;
    const void *userptr;
} ev_src_t;

/* Struct that holds pubsub messaging, private. It keeps reference count */
typedef struct {
    ps_msg_t msg;
    uint64_t refs;
    bool autofree;
    ev_src_t *sub;
} ps_priv_t;

typedef struct {
    void *data;                             // Context's poll priv data (depends upon poll_plugin)
    int max_events;                         // Max number of returned events for poll_plugin
} poll_priv_t;

/* Struct that holds data for each module */
struct _module {
    userhook_t hook;                        // module's user defined callbacks
    mod_stack_t *recvs;                     // Stack of recv functions for module_become/unbecome (stack of funpointers)
    const void *userdata;                   // module's user defined data
    mod_states state;                       // module's state
    const char *name;                       // module's name
    const char *local_path;                 // For runtime loaded modules: path of module
    mod_list_t *srcs;                        // module's fds to be polled (list of ev_src_t)
    mod_map_t *subscriptions;               // module's subscriptions (map of ev_src_t)
    int pubsub_fd[2];                       // In and Out pipe for pubsub msg
    self_t self;                            // pointer to self (and thus context)
};

/* Struct that holds data for each context */
struct _context {
    const char *name;                       // Context's name'
    bool quit;                              // Context's quit flag
    uint8_t quit_code;                      // Context's quit code, returned by modules_ctx_loop()
    bool looping;                           // Whether context is looping
    log_cb logger;                          // Context's log callback
    mod_map_t *modules;                     // Context's modules
    poll_priv_t ppriv;                      // Priv data for poll_plugin implementation
    size_t running_mods;                    // Number of RUNNING modules in context
};

/* Defined in module.c */
_pure_ bool _module_is(const mod_t *mod, const mod_states st);
mod_map_ret evaluate_module(void *data, const char *key, void *value);
mod_ret start(mod_t *mod, const bool starting);
mod_ret stop(mod_t *mod, const bool stopping);

/* Defined in pubsub.c */
mod_ret tell_system_pubsub_msg(mod_t *mod, ctx_t *c, ps_msg_type type, 
                                       const self_t *sender, const char *topic);
mod_map_ret flush_pubsub_msgs(void *data, const char *key, void *value);
void run_pubsub_cb(mod_t *mod, msg_t *msg, const void *userptr);

/* Defined in priv.c */
char *mem_strdup(const char *s);

/* Defined in map.c */
void *map_peek(const mod_map_t *m);

extern mod_map_t *ctx;
extern memhook_t memhook;
