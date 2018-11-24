#include <stdlib.h>
#include "map.h"
#include "stack.h"

#ifndef NDEBUG
    #define MOD_ASSERT(cond, msg, ret) if (!(cond)) { fprintf(stderr, "%s\n", msg); return ret; }
#else
    #define MOD_ASSERT(cond, msg, ret) if (!(cond)) { return ret; }
#endif

#define MOD_ALLOC_ASSERT(cond)  MOD_ASSERT(cond, "Failed to malloc.", MOD_NO_MEM);
#define MOD_PARAM_ASSERT(cond)  MOD_ASSERT(cond, #cond, MOD_WRONG_PARAM);

#ifndef NDEBUG
    #define MODULE_DEBUG printf("Libmodule: "); printf
#else
    #define MODULE_DEBUG (void)
#endif

/* Finds a ctx inside our global map, given its name */
#define FIND_CTX(name) \
    MOD_ASSERT(name, "NULL ctx.", MOD_ERR); \
    m_context *c = map_get(ctx, (char *)name); \
    MOD_ASSERT(c, "Context not found.", MOD_NO_CTX);

/* Convenience macro to retrieve self->ctx + doing some checks */
#define GET_CTX(self) \
    MOD_ASSERT(self, "NULL self handler.", MOD_NO_SELF); \
    m_context *c = self->ctx; \
    MOD_ASSERT(c, "Context not found.", MOD_NO_CTX);

/* Convenience macro to retrieve a module from a context, given its name */
#define CTX_GET_MOD(name, ctx) \
    module *mod = map_get(ctx->modules, (char *)name); \
    MOD_ASSERT(mod, "Module not found.", MOD_NO_MOD);

/* Convenience macro to retrieve self->mod + doing some checks */
#define GET_MOD(self) \
    MOD_ASSERT(self, "NULL self handler.", MOD_NO_SELF); \
    module *mod = self->mod; \
    MOD_ASSERT(mod, "Module not found.", MOD_NO_MOD);

/*
 * Convenience macro to retrieve self->mod + doing some checks 
 * + checking if its state matches required state 
 */
#define GET_MOD_IN_STATE(self, state) \
    GET_MOD(self); \
    MOD_ASSERT(module_is(self, state), "Wrong module state.", MOD_WRONG_STATE);

typedef struct _poll_t {
    int fd;
    bool autoclose;
    void *ev;
    const void *userptr;
    const struct _self *self;                   // ptr needed to map a fd to a self_t in epoll
    struct _poll_t *prev;
} module_poll_t;

/* Struct that holds data for each module */
typedef struct {
    userhook hook;                        // module's user defined callbacks
    stack_t *recvs;
    const void *userdata;                 // module's user defined data
    enum module_states state;             // module's state
    const char *name;                     // module's name
    module_poll_t *fds;                   // module's fds to be polled
    map_t *subscriptions;                 // module's subscriptions
    int pubsub_fd[2];                     // In and Out pipe for pubsub msg
    const struct _self *self;             // pointer to self (and thus context)
} module;

typedef struct {
    const char *name;
    bool quit;
    uint8_t quit_code;
    bool looping;
    int fd;
    int num_fds;
    log_cb logger;
    map_t *modules;
    void *pevents;
    int max_events;
    map_t *topics;
} m_context;

/* Struct that holds self module informations, static to each module */
struct _self {
    module *const mod;                    // self's mod
    m_context *const ctx;                 // self's ctx 
};

int evaluate_module(void *data, void *m);
module_ret_code tell_system_pubsub_msg(m_context *c, enum msg_type type, const char *topic);
int flush_pubsub_msg(void *data, void *m);
void destroy_pubsub_msg(pubsub_msg_t *m);
char *mem_strdup(const char *s);

extern map_t *ctx;
extern memalloc_hook memhook;
