#pragma once

#include <stdlib.h>
#include "map.h"
#include "stack.h"

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
    m_context *c = map_get(ctx, (char *)name); \
    MOD_ASSERT(c, "Context not found.", MOD_NO_CTX);

#define GET_CTX_PRIV(self)  m_context *c = self->ctx
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
    module *mod = map_get(ctx->modules, (char *)name); \
    MOD_ASSERT(mod, "Module not found.", MOD_NO_MOD);

#define GET_MOD_PRIV(self) module *mod = self->mod
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

typedef struct _module module;
typedef struct _context m_context;

/* Struct that holds self module informations, static to each module */
struct _self {
    module *mod;                            // self's mod
    m_context *ctx;                         // self's ctx
    bool is_ref;                            // is this a reference?
};

/* List that matches fds with selfs */
typedef struct _poll_t {
    int fd;
    bool autoclose;
    void *ev;
    const void *userptr;
    const struct _self *self;               // ptr needed to map a fd to a self_t in epoll
    struct _poll_t *prev;
} module_poll_t;

/* Struct that holds pubsub messaging, private. It keeps reference count. */
typedef struct {
    ps_msg_t msg;
    uint64_t refs;
    bool autofree;
} ps_priv_t;

/* Struct that holds data for each module */
struct _module {
    userhook hook;                          // module's user defined callbacks
    stack_t *recvs;                         // Stack of recv functions for module_become/unbecome
    const void *userdata;                   // module's user defined data
    enum module_states state;               // module's state
    const char *name;                       // module's name
    module_poll_t *fds;                     // module's fds to be polled
    map_t *subscriptions;                   // module's subscriptions
    int pubsub_fd[2];                       // In and Out pipe for pubsub msg
    self_t self;                            // pointer to self (and thus context)
};

/* Struct that holds data for each context */
struct _context {
    const char *name;                       // Context's name'
    bool quit;                              // Context's quit flag
    uint8_t quit_code;                      // Context's quit code, returned by modules_ctx_loop()
    bool looping;                           // Whether context is looping
    int fd;                                 // Context's epoll/kqueue fd
    log_cb logger;                          // Context's log callback
    map_t *modules;                         // Context's modules
    void *pevents;                          // Context's polled events structs
    int max_events;                         // Max number of returned events for epoll/kqueue
    size_t running_mods;                    // Number of RUNNING modules in context
};

/* Defined in module.c */
_pure_ bool _module_is(const module *mod, const enum module_states st);
map_ret_code evaluate_module(void *data, const char *key, void *value);
module_ret_code start(module *mod, const bool start);
module_ret_code stop(module *mod, const bool stop);

/* Defined in pubsub.c */
module_ret_code tell_system_pubsub_msg(module *mod, m_context *c, enum msg_type type, 
                                       const self_t *sender, const char *topic);
map_ret_code flush_pubsub_msgs(void *data, const char *key, void *value);
void run_pubsub_cb(module *mod, msg_t *msg);

/* Defined in priv.c */
char *mem_strdup(const char *s);

extern map_t *ctx;
extern memhook_t memhook;
