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
#define MOD_ASSERT(cond, msg) assert(cond)
#else
#define MODULE_DEBUG (void)
#define MOD_ASSERT(cond, msg) if(!cond) { fprintf(stderr, "%s\n", msg); return MOD_ERR; }
#endif

#define GET_CTX(name) \
    m_context *c = NULL; \
    hashmap_get(ctx, (char *)name, (void **)&c); \
    MOD_ASSERT(c, "Context not found.");
    
#define CTX_GET_MOD(name, ctx) \
    module *mod = NULL; \
    hashmap_get(ctx->modules, (char *)name, (void **)&mod); \
    MOD_ASSERT(mod, "Module not found.");

#define GET_MOD(self) \
    MOD_ASSERT(self, "NULL self handler."); \
    self_t *s = (self_t *)self; \
    GET_CTX(s->ctx) \
    CTX_GET_MOD(s->name, c)
    
#define CHILDREN_LOOP(f) \
    child_t *tmp = m->children; \
    while (tmp) { \
        GET_MOD(tmp->self); \
        f; \
        tmp = tmp->next; \
    }

/* Struct that holds self module informations, static to each module */
typedef struct {
    const char *name;                     // module's name
    const char *ctx;                      // module's ctx 
} self_t;

typedef struct child {
    const self_t *self;                   // module's name
    struct child *next;                  // module's ctx 
} child_t;

/* Struct that holds data for each module */
typedef struct {
    userhook *hook;                       // module's user defined callbacks
    const void *userdata;                 // module's user defined data
    enum module_states state;             // module's state
    self_t self;                          // module's info available to external world
    int fd;                               // file descriptor to be polled
    struct epoll_event ev;                // module's epoll event struct
    child_t *children;                    // list of children modules
} module;

typedef struct {
    int quit;
    int epollfd;
    error_cb on_error;
    map_t modules;
} m_context;

static _ctor0_ void modules_init(void);
static _dtor0_ void modules_destroy(void);
static int init_ctx(const char *ctx_name, m_context **context);
static void destroy_ctx(const char *ctx_name, m_context *context);
static m_context *check_ctx(const char *ctx_name);
static int add_children(module *mod, const void *self);
static void evaluate_new_state(m_context *context);
static int evaluate_module(void *data, void *m);
static void default_error(const char *error_msg, const char *ctx_name);
static void start_children(module *m);
static void stop_children(module *m);

static map_t ctx;

static void modules_init(void) {
    MODULE_DEBUG("Initializing library.\n");
    ctx = hashmap_new();
}

static void modules_destroy(void) {
    MODULE_DEBUG("Destroying library.\n");
    hashmap_free(ctx);
}

static int init_ctx(const char *ctx_name, m_context **context) {
    MODULE_DEBUG("Creating context '%s'.\n", ctx_name);
    
    *context = malloc(sizeof(m_context));
    MOD_ASSERT(*context, "Failed to malloc.");
    
    **context = (m_context) {0};
         
    (*context)->epollfd = epoll_create1(EPOLL_CLOEXEC); 
    MOD_ASSERT((*context)->epollfd >= 0, "Failed to create epollfd.");
     
    (*context)->on_error = default_error;
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

int module_register(const char *name, const char *ctx_name, const void **self, userhook *hook) {
    MOD_ASSERT(name, "NULL module name.");
    MOD_ASSERT(ctx_name, "NULL context name.");
    
    m_context *context = check_ctx(ctx_name);
    MOD_ASSERT(context, "Failed to create context.");
    
    MODULE_DEBUG("Registering module %s.\n", name);

    module *tmp = malloc(sizeof(module));
    MOD_ASSERT(tmp, "Failed to malloc.");
    
    *tmp = (module) {0};

    tmp->hook = hook;
    tmp->state = IDLE;
    tmp->self.name = strdup(name);
    tmp->self.ctx = strdup(ctx_name);
    if (hashmap_put(context->modules, (char *)name, tmp) == MAP_OK) {
        *self = &tmp->self;
        evaluate_module(NULL, tmp);
        return MOD_OK;
    }
    
    free((void *)tmp->self.name);
    free((void *)tmp->self.ctx);
    free(tmp);
    return MOD_ERR;
}

int module_deregister(const void **self) {
    self_t *tmp = (self_t *) *self;
    
    GET_MOD(tmp);
    
    MODULE_DEBUG("Deregistering module %s.\n", tmp->name);
        
    module_stop(tmp);
        
    mod->hook->destroy();
    hashmap_remove(c->modules, (char *)tmp->name);
    /* Remove context without modules */
    if (hashmap_length(c->modules) == 0) {
        destroy_ctx(tmp->ctx, c);
    }
    free((void *)tmp->name);
    free((void *)tmp->ctx);
    tmp = NULL;
    free(mod);
    return MOD_OK;
}

static int add_children(module *mod, const void *self) {
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

int module_binds_to(const void *self, const char *parent) {
    self_t *tmp = (self_t *) self;
    GET_CTX(tmp->ctx);
    
    module *mod = NULL;
    hashmap_get(c->modules, (char *)parent, (void **)&mod);
    MOD_ASSERT(mod, "Parent module not found.");
    
    return add_children(mod, self);
}

int modules_ctx_loop(const char *ctx_name) {
    GET_CTX(ctx_name);
    
    int size = hashmap_length(c->modules);
    struct epoll_event *pevents = calloc(size, sizeof(struct epoll_event));
    MOD_ASSERT(pevents, "Failed to malloc.");
        
    while (!c->quit) {
        int nfds = epoll_wait(c->epollfd, pevents, size, -1);
        if (nfds < 0) {
            c->on_error("epoll failed.", ctx_name);
        } else {
            for (int i = 0; i < nfds; i++) {
                if (pevents[i].events & EPOLLIN) {
                    self_t *self = (self_t *)pevents[i].data.ptr;
                    
                    CTX_GET_MOD(self->name, c);
                    
                    message_t msg = { mod->fd, NULL, NULL };
                    mod->hook->recv(&msg, mod->userdata);
                }
            }
            evaluate_new_state(c);
        }
    }
    free(pevents);
    return MOD_OK;
}

void modules_ctx_quit(const char *ctx_name) {
    GET_CTX(ctx_name);
    
    c->quit = 1;
}

void modules_ctx_on_error(const char *ctx_name, error_cb on_error) {
    GET_CTX(ctx_name);
    
    c->on_error = on_error;
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

int module_become(const void *self,  recv_cb new_recv) {    
    GET_MOD(self);
    mod->hook->recv = new_recv;
    return MOD_OK;
}

int module_log(const void *self, const char *fmt, ...) {
    MOD_ASSERT(self, "Module not found.");
    
    va_list args;
    va_start(args, fmt);
    printf("%s: ", ((self_t *)self)->name);
    vprintf(fmt, args);
    va_end(args);
    return MOD_OK;
}

int module_set_userdata(const void *self, const void *userdata) {    
    GET_MOD(self);
    mod->userdata = userdata;
    return MOD_OK;
}

static void default_error(const char *error_msg, const char *ctx_name) {
    fprintf(stderr, "[%s] Libmodule error: %s\n", ctx_name, error_msg);
    modules_ctx_quit(ctx_name);
}

/** Module state getter **/

int module_is(const void *self, const enum module_states st) {    
    GET_MOD(self);
    return mod->state == st;
}

/** Module state setters **/

int module_start(const void *self, int fd) {    
    GET_MOD(self);
    mod->fd = fd;
    MODULE_DEBUG("Starting module %s.\n", ((self_t *)self)->name);
    return module_resume(self);
}

int module_pause(const void *self) {
    GET_MOD(self);
    int ret = epoll_ctl(c->epollfd, EPOLL_CTL_DEL, mod->fd, NULL);
    if (!ret) {
        mod->state = PAUSED;
        return MOD_OK;
    }
    return MOD_ERR;
}

int module_resume(const void *self) {    
    GET_MOD(self);
    mod->ev.data.ptr = (void *)self;
    mod->ev.events = EPOLLIN;
    int ret = epoll_ctl(c->epollfd, EPOLL_CTL_ADD, mod->fd, &mod->ev);
    if (!ret) {
        mod->state = RUNNING;
        return MOD_OK;
    }
    return MOD_ERR;
}

int module_stop(const void *self) {    
    GET_MOD(self);
    MODULE_DEBUG("Stopping module %s.\n", ((self_t *)self)->name);
    mod->state = IDLE;
    if (close(mod->fd) == 0) { // implicitly calls EPOLL_CTL_DEL
        return MOD_OK;
    }
    return MOD_ERR;
}

static void start_children(module *m) {
    CHILDREN_LOOP({
        int fd = mod->hook->init();
        module_start(&mod->self, fd);
    });
}

static void stop_children(module *m) {
    CHILDREN_LOOP({
        module_stop(&mod->self);
    });
}
