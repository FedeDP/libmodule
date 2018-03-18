#include <module.h> 
#include <string.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <assert.h>
#include <stdarg.h>
#include <hashmap.h>

#ifndef NDEBUG
#define MODULE_DEBUG printf("Libmodule: "); printf
#define MOD_ASSERT(cond, msg, ret) assert(cond)
#else
#define MODULE_DEBUG (void)
#define MOD_ASSERT(cond, msg, ret) if(!cond) { fprintf(stderr, "%s\n", msg); return ret; }
#endif

#define GET_CTX(name) \
    m_context *c = NULL; \
    hashmap_get(ctx, (char *)name, (void **)&c); \
    MOD_ASSERT(c, "Context not found.", MOD_NO_CTX);
    
#define CTX_GET_MOD(name, ctx) \
    module *mod = NULL; \
    hashmap_get(ctx->modules, (char *)name, (void **)&mod); \
    MOD_ASSERT(mod, "Module not found.", MOD_NO_MOD);

#define GET_MOD(self) \
    MOD_ASSERT(self, "NULL self handler.", MOD_NO_SELF); \
    GET_CTX(self->ctx) \
    CTX_GET_MOD(self->name, c)
    
#define GET_MOD_IN_STATE(self, state) \
    GET_MOD(self); \
    if (!module_is(self, state)) { return MOD_WRONG_STATE; }
    
#define CHILDREN_LOOP(f) \
    child_t *tmp = m->children; \
    while (tmp) { \
        GET_MOD(tmp->self); \
        f; \
        tmp = tmp->next; \
    }

/* Struct that holds self module informations, static to each module */
struct _self {
    const char *name;                     // module's name
    const char *ctx;                      // module's ctx 
};

typedef struct child {
    const self_t *self;                   // module's name
    struct child *next;                   // module's ctx 
} child_t;

/* Struct that holds data for each module */
typedef struct {
    userhook *hook;                       // module's user defined callbacks
    const void *userdata;                 // module's user defined data
    enum module_states state;             // module's state
    self_t self;                          // module's info available to external world
    int fd;                               // file descriptor to be polled
    map_t subscriptions;                  // module's subscriptions
    struct epoll_event ev;                // module's epoll event struct
    child_t *children;                    // list of children modules
} module;

typedef struct {
    int quit;
    int epollfd;
    map_t modules;
} m_context;

static _ctor0_ void modules_init(void);
static _dtor0_ void modules_destroy(void);
static module_ret_code init_ctx(const char *ctx_name, m_context **context);
static void destroy_ctx(const char *ctx_name, m_context *context);
static m_context *check_ctx(const char *ctx_name);
static module_ret_code add_children(module *mod, const void *self);
static void evaluate_new_state(m_context *context);
static int evaluate_module(void *data, void *m);
static int add_subscription(module *mod, const char *topic);
static int tell_if(void *data, void *m);
static module_ret_code start_children(module *m);
static module_ret_code stop_children(module *m);

static map_t ctx;

static void modules_init(void) {
    MODULE_DEBUG("Initializing library.\n");
    ctx = hashmap_new();
}

static void modules_destroy(void) {
    MODULE_DEBUG("Destroying library.\n");
    hashmap_free(ctx);
}

static module_ret_code init_ctx(const char *ctx_name, m_context **context) {
    MODULE_DEBUG("Creating context '%s'.\n", ctx_name);
    
    *context = malloc(sizeof(m_context));
    MOD_ASSERT(*context, "Failed to malloc.", MOD_ERR);
    
    **context = (m_context) {0};
         
    (*context)->epollfd = epoll_create1(EPOLL_CLOEXEC); 
    MOD_ASSERT(((*context)->epollfd >= 0), "Failed to create epollfd.", MOD_ERR);
     
    (*context)->modules = hashmap_new();
    if ((*context)->modules && hashmap_put(ctx, (char *)ctx_name, *context) == MAP_OK) {
        return MOD_OK;
    }
    
    destroy_ctx(ctx_name, *context);
    *context = NULL;
    return MOD_ERR;
}

static void destroy_ctx(const char *ctx_name, m_context *context) {
    MODULE_DEBUG("Destroying context '%s'.\n", ctx_name);
    hashmap_free(context->modules);
    close(context->epollfd);
    free(context);
    hashmap_remove(ctx, (char *)ctx_name);
}

static m_context *check_ctx(const char *ctx_name) {
    m_context *context = NULL;
    hashmap_get(ctx, (char *)ctx_name, (void **)&context); \
    if (!context) {
        init_ctx(ctx_name, &context);
    }
    return context;
}

module_ret_code module_register(const char *name, const char *ctx_name, const self_t **self, userhook *hook) {
    MOD_ASSERT(name, "NULL module name.", MOD_ERR);
    MOD_ASSERT(ctx_name, "NULL context name.", MOD_ERR);
    
    m_context *context = check_ctx(ctx_name);
    MOD_ASSERT(context, "Failed to create context.", MOD_ERR);
    
    module *mod = NULL;
    hashmap_get(context->modules, (char *)name, (void **)&mod);
    if (mod) {
        *self = NULL;
        MODULE_DEBUG("Module %s already registered in context %s.\n", name, ctx_name);
        return MOD_ERR;
    }
    
    MODULE_DEBUG("Registering module %s.\n", name);
    
    mod = malloc(sizeof(module));
    MOD_ASSERT(mod, "Failed to malloc.", MOD_ERR);
    
    *mod = (module) {0};
    if (hashmap_put(context->modules, (char *)name, mod) == MAP_OK) {        
        mod->hook = hook;
        mod->state = IDLE;
        mod->self.name = strdup(name);
        mod->self.ctx = strdup(ctx_name);
        mod->subscriptions = hashmap_new();
        *self = &mod->self;        
        evaluate_module(NULL, mod);        
        return MOD_OK;
    }
    free(mod);
    return MOD_ERR;
}

module_ret_code module_deregister(const self_t **self) {
    self_t *tmp = (self_t *) *self;
    
    GET_MOD(tmp);
    
    MODULE_DEBUG("Deregistering module %s.\n", tmp->name);
        
    module_stop(tmp);
        
    mod->hook->destroy();
    /* Remove the module from the context */
    hashmap_remove(c->modules, (char *)tmp->name);
    /* Remove context without modules */
    if (hashmap_length(c->modules) == 0) {
        destroy_ctx(tmp->ctx, c);
    }
    
    hashmap_free(mod->subscriptions);
    free((void *)tmp->name);
    free((void *)tmp->ctx);
    tmp = NULL;
    free(mod);
    return MOD_OK;
}

static module_ret_code add_children(module *mod, const void *self) {
    child_t **tmp = &mod->children;
    while (*tmp) {
        tmp = &(*tmp)->next;
    }
    *tmp = malloc(sizeof(child_t));
    if (*tmp) {
        (*tmp)->self = self;
        (*tmp)->next = NULL;
        return MOD_OK;
    }
    return MOD_ERR;
}

module_ret_code module_binds_to(const self_t *self, const char *parent) {
    self_t *tmp = (self_t *) self;
    GET_CTX(tmp->ctx);
    
    module *mod = NULL;
    hashmap_get(c->modules, (char *)parent, (void **)&mod);
    MOD_ASSERT(mod, "Parent module not found.", MOD_NO_PARENT);
    
    return add_children(mod, self);
}

module_ret_code modules_ctx_loop(const char *ctx_name) {
    GET_CTX(ctx_name);
    
    int size = hashmap_length(c->modules);
    struct epoll_event *pevents = calloc(size, sizeof(struct epoll_event));
    MOD_ASSERT(pevents, "Failed to malloc.", MOD_ERR);
        
    int ret = MOD_OK;
    while (!c->quit) {
        int nfds = epoll_wait(c->epollfd, pevents, size, -1);
        if (nfds < 0) {
            ret = MOD_ERR;
            break;
        } else {
            for (int i = 0; i < nfds; i++) {
                if (pevents[i].events & EPOLLIN) {
                    self_t *self = (self_t *)pevents[i].data.ptr;
                    
                    CTX_GET_MOD(self->name, c);
                    
                    msg_t msg = { mod->fd, NULL };
                    mod->hook->recv(&msg, mod->userdata);
                }
            }
            evaluate_new_state(c);
        }
    }
    free(pevents);
    return ret;
}

module_ret_code modules_ctx_quit(const char *ctx_name) {
    GET_CTX(ctx_name);
    
    c->quit = 1;
    return MOD_OK;
}

static void evaluate_new_state(m_context *context) {
    hashmap_iterate(context->modules, evaluate_module, NULL);
}

static int evaluate_module(void *data, void *m) {
    module *mod = (module *)m;
    if (module_is(&mod->self, IDLE) 
        && mod->hook->evaluate()) {
            
        int fd = mod->hook->init();
        module_start(&mod->self, fd);
    }
    return MAP_OK;
}

module_ret_code module_become(const self_t *self,  recv_cb new_recv) {    
    GET_MOD_IN_STATE(self, RUNNING);
    
    mod->hook->recv = new_recv;
    return MOD_OK;
}

module_ret_code module_log(const self_t *self, const char *fmt, ...) {
    MOD_ASSERT(self, "Module not found.", MOD_NO_MOD);
    
    va_list args;
    va_start(args, fmt);
    printf("%s: ", ((self_t *)self)->name);
    vprintf(fmt, args);
    va_end(args);
    return MOD_OK;
}

module_ret_code module_set_userdata(const self_t *self, const void *userdata) {    
    GET_MOD(self);
    
    mod->userdata = userdata;
    return MOD_OK;
}

module_ret_code module_update_fd(const self_t *self, int new_fd, int close_old) {
    GET_MOD_IN_STATE(self, RUNNING);
    
    /* De-register this fd from epoll */
    int ret = epoll_ctl(c->epollfd, EPOLL_CTL_DEL, mod->fd, NULL);
    if (!ret) {
        if (close_old) {
            close(mod->fd);
        }
    
        mod->fd = new_fd;
        /* Register new fd */
        ret = epoll_ctl(c->epollfd, EPOLL_CTL_ADD, mod->fd, &mod->ev);
        if (!ret) {
            return MOD_OK;
        }
    }
    return MOD_ERR;
}

/** Actor-like interface **/

static module_ret_code add_subscription(module *mod, const char *topic) {
    void *tmp = NULL;
    if (hashmap_get(mod->subscriptions, (char *)topic, (void **)&tmp) == MAP_MISSING) {
        /* Store pointer to mod as value, even if it will be unused; this should be a hashset */
        if (hashmap_put(mod->subscriptions, (char *)topic, mod) == MAP_OK) {
            return MOD_OK;
        }
    }
    return MOD_ERR;
}

module_ret_code module_subscribe(const self_t *self, const char *topic) {
    GET_MOD(self);
    
    return add_subscription(mod, topic);
}

static int tell_if(void *data, void *m) {
    module *mod = (module *)m;
    msg_t *msg = (msg_t *)data;
    void *tmp = NULL;

    /* 
     * Only if mod is actually running and 
     * if topic is null or this module is subscribed to topic 
     */
    if (module_is(&mod->self, RUNNING) && (!msg->msg->topic || 
        hashmap_get(mod->subscriptions, (char *)msg->msg->topic, (void **)&tmp) == MAP_OK)) {
        
        MODULE_DEBUG("Telling a message to %s.\n", mod->self.name);
        mod->hook->recv(msg, mod->userdata);
    }
    return MAP_OK;
}

module_ret_code module_tell(const self_t *self, const char *message, const char *recipient) {
    self_t *s = (self_t *)self;
    GET_CTX(s->ctx);
    CTX_GET_MOD(recipient, c);
    
    msg_t msg = { .fd = -1 };
    pubsub_msg_t *tmp = malloc(sizeof(pubsub_msg_t));
    tmp->message = message;
    tmp->sender = s->name;
    tmp->topic = NULL;
    msg.msg = tmp;
    
    tell_if(&msg, mod);
    
    free(tmp);
    return MOD_OK;
}

module_ret_code module_publish(const self_t *self, const char *topic, const char *message) {
    self_t *s = (self_t *)self;
    GET_CTX(s->ctx);
    
    msg_t msg = { .fd = -1 };
    pubsub_msg_t *tmp = malloc(sizeof(pubsub_msg_t));
    tmp->message = message;
    tmp->sender = s->name;
    tmp->topic = topic;
    msg.msg = tmp;
    
    hashmap_iterate(c->modules, tell_if, &msg);
    
    free(tmp);
    return MOD_OK;
}

/** Module state getter **/

int module_is(const self_t *self, const enum module_states st) {
    GET_MOD(self);
    
    return mod->state & st;
}

/** Module state setters **/

module_ret_code module_start(const self_t *self, int fd) {        
    GET_MOD_IN_STATE(self, IDLE);
    
    mod->fd = fd;
    MODULE_DEBUG("Starting module %s.\n", ((self_t *)self)->name);
    return module_resume(self);
}

module_ret_code module_pause(const self_t *self) {
    GET_MOD_IN_STATE(self, RUNNING);
    
    int ret = 0;
    if (mod->fd != MODULE_DONT_POLL) {
        ret = epoll_ctl(c->epollfd, EPOLL_CTL_DEL, mod->fd, NULL);
    }
    if (!ret) {
        mod->state = PAUSED;
        return MOD_OK;
    }
    return MOD_ERR;
}

module_ret_code module_resume(const self_t *self) {
    GET_MOD_IN_STATE(self, IDLE | PAUSED);
    
    int ret = 0;
    if (mod->fd != MODULE_DONT_POLL) {
        mod->ev.data.ptr = (void *)self;
        mod->ev.events = EPOLLIN;
        ret = epoll_ctl(c->epollfd, EPOLL_CTL_ADD, mod->fd, &mod->ev);
    }
    if (!ret) {
        mod->state = RUNNING;
        return MOD_OK;
    }
    return MOD_ERR;
}

module_ret_code module_stop(const self_t *self) {
    GET_MOD_IN_STATE(self, RUNNING);
    
    MODULE_DEBUG("Stopping module %s.\n", ((self_t *)self)->name);
    mod->state = STOPPED;
    if (mod->fd == MODULE_DONT_POLL || close(mod->fd) == 0) { // implicitly calls EPOLL_CTL_DEL
        return MOD_OK;
    }
    return MOD_ERR;
}

static module_ret_code start_children(module *m) {
    CHILDREN_LOOP({
        int fd = mod->hook->init();
        module_start(&mod->self, fd);
    });
    return MOD_OK;
}

static module_ret_code stop_children(module *m) {
    CHILDREN_LOOP({
        module_stop(&mod->self);
    });
    return MOD_OK;
}
