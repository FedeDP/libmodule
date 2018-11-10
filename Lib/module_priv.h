#include <assert.h>
#include "hashmap.h"
#include <stdlib.h>
#include <unistd.h>

#define MOD_ASSERT(cond, msg, ret) if(!cond) { fprintf(stderr, "%s\n", msg); return ret; }

#ifndef NDEBUG
    #define MODULE_DEBUG printf("Libmodule: "); printf
#else
    #define MODULE_DEBUG (void)
#endif

#define GET_CTX(name) \
    MOD_ASSERT(name, "NULL ctx.", MOD_ERR); \
    m_context *c = NULL; \
    hashmap_get(ctx, (char *)name, (void **)&c); \
    MOD_ASSERT(c, "Context not found.", MOD_NO_CTX);

#define CTX_GET_MOD(name, ctx) \
    module *mod = NULL; \
    hashmap_get(ctx->modules, (char *)name, (void **)&mod); \
    MOD_ASSERT(mod, "Module not found.", MOD_NO_MOD);

#define GET_MOD(self) \
    MOD_ASSERT(self, "NULL self handler.", MOD_NO_SELF); \
    GET_CTX(self->ctx); \
    CTX_GET_MOD(self->name, c);

#define GET_MOD_IN_STATE(self, state) \
    GET_MOD(self); \
    if (!module_is(self, state)) { return MOD_WRONG_STATE; }

/* Struct that holds self module informations, static to each module */
struct _self {
    const char *name;                     // module's name
    const char *ctx;                      // module's ctx 
};

typedef struct _poll_t {
    int fd;
    bool autoclose;
    void *ev;
    const void *userptr;
    const self_t *self;                   // ptr needed to map a fd to a self_t in epoll
    struct _poll_t *prev;
} module_poll_t;

/* Struct that holds data for each module */
typedef struct {
    userhook hook;                        // module's user defined callbacks
    const void *userdata;                 // module's user defined data
    enum module_states state;             // module's state
    self_t self;                          // module's info available to external world
    module_poll_t *fds;                   // module's fds to be polled
    map_t subscriptions;                  // module's subscriptions
    int pubsub_fd[2];                     // In and Out pipe for pubsub msg
} module;

typedef struct {
    bool quit;
    uint8_t quit_code;
    bool looping;
    int fd;
    int num_fds;                          // number of fds in this context
    log_cb logger;
    map_t modules;
    void *pevents;
    int max_events;
    map_t topics;
} m_context;

int evaluate_module(void *data, void *m);
module_ret_code tell_system_pubsub_msg(m_context *c, enum msg_type type, const char *topic);
int flush_pubsub_msg(void *data, void *m);
void destroy_pubsub_msg(pubsub_msg_t *m);
char *mem_strdup(const char *s);

extern map_t ctx;
extern memalloc_hook memhook;
