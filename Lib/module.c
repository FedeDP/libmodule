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
#else
#define MODULE_DEBUG (void)
#endif

#define GET_CTX(name) \
    m_context *c = NULL; \
    hashmap_get(ctx, (char *)name, (void **)&c); \
    assert(c);

#define GET_MOD(self) \
    self_t *s = (self_t *)self; \
    m_context *context = NULL; \
    module *mod = NULL; \
    do { \
        hashmap_get(ctx, (char *)s->ctx, (void **)&context); \
        assert(context); \
        hashmap_get(context->modules, (char *)s->name, (void **)&mod); \
        if (!mod) context->on_error("Module not found.", (char *)s->ctx); \
    } while(0)
    
#define CHILDREN_LOOP(f) \
    child_t *tmp = m->children; \
    while (tmp) { \
        GET_MOD(tmp->self); \
        f; \
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
static void init_ctx(const char *ctx_name, m_context **context);
static void destroy_ctx(const char *ctx_name, m_context *context);
static m_context *check_ctx(const char *ctx_name);
static void add_children(module *mod, const void *self);
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

static void init_ctx(const char *ctx_name, m_context **context) {
    MODULE_DEBUG("Creating context '%s'.\n", ctx_name);
    
    *context = malloc(sizeof(m_context));
    assert(*context);
    
    memset(*context, 0, sizeof(m_context));
     
    (*context)->epollfd = epoll_create1(EPOLL_CLOEXEC); 
    assert((*context)->epollfd >= 0);
     
    (*context)->on_error = default_error;
    (*context)->modules = hashmap_new();
    hashmap_put(ctx, (char *)ctx_name, *context);
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

void module_register(const char *name, const char *ctx_name, const void **self, userhook *hook) {
    assert(name);
    assert(ctx_name);
    
    m_context *context = check_ctx(ctx_name);

    MODULE_DEBUG("Registering module %s.\n", name);

    module *tmp = malloc(sizeof(module));
    assert(tmp);
    
    memset(tmp, 0, sizeof(module));
    
    tmp->hook = hook;
    tmp->state = IDLE;
    tmp->self.name = strdup(name);
    tmp->self.ctx = strdup(ctx_name);
    hashmap_put(context->modules, (char *)name, tmp);
    
    *self = &tmp->self;
    evaluate_module(NULL, tmp);
}

void module_deregister(const void **self) {
    self_t *tmp = (self_t *) *self;
    if (tmp) {
        MODULE_DEBUG("Deregistering module %s.\n", tmp->name);
        
        module_stop(tmp);
        GET_MOD(tmp);
        
        mod->hook->destroy();
        hashmap_remove(context->modules, (char *)tmp->name);
        /* Remove context without modules */
        if (hashmap_length(context->modules) == 0) {
            destroy_ctx(tmp->ctx, context);
        }
        free((void *)tmp->name);
        free((void *)tmp->ctx);
        tmp = NULL;
        free(mod);
    }
}

static void add_children(module *mod, const void *self) {
    child_t **tmp = &mod->children;
    while (*tmp) {
        tmp = &(*tmp)->next;
    }
    *tmp = malloc(sizeof(child_t));
    (*tmp)->self = self;
    (*tmp)->next = NULL;
}

void module_binds_to(const void *self, const char *parent) {
    self_t *tmp = (self_t *) self;
    GET_CTX(tmp->ctx);
    
    module *mod = NULL;
    hashmap_get(c->modules, (char *)parent, (void **)&mod);
    assert(mod);
    
    add_children(mod, self);
}

void modules_ctx_loop(const char *ctx_name) {
    GET_CTX(ctx_name);
    
    int size = hashmap_length(c->modules);
    struct epoll_event *pevents = calloc(size, sizeof(struct epoll_event));
    assert(pevents);
        
    while (!c->quit) {
        int nfds = epoll_wait(c->epollfd, pevents, size, -1);
        if (nfds < 0) {
            c->on_error("epoll failed.", ctx_name);
        } else {
            for (int i = 0; i < nfds; i++) {
                if (pevents[i].events & EPOLLIN) {
                    void *self = pevents[i].data.ptr;
                    
                    GET_MOD(self);
                    
                    message_t msg = { mod->fd, NULL, NULL };
                    mod->hook->recv(&msg, mod->userdata);
                }
            }
            evaluate_new_state(c);
        }
    }
    free(pevents);
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

void module_become(const void *self,  recv_cb new_recv) {
    assert(self);
    
    GET_MOD(self);
    mod->hook->recv = new_recv;
}

void module_log(const void *self, const char *fmt, ...) {
    assert(self);
    
    va_list args;
    va_start(args, fmt);
    printf("%s: ", ((self_t *)self)->name);
    vprintf(fmt, args);
    va_end(args);
}

void module_set_userdata(const void *self, const void *userdata) {
    assert(self);
    
    GET_MOD(self);
    mod->userdata = userdata;
}

static void default_error(const char *error_msg, const char *ctx_name) {
    fprintf(stderr, "[%s] Libmodule error: %s\n", ctx_name, error_msg);
    modules_ctx_quit(ctx_name);
}

/** Module state getter **/

int module_is(const void *self, const enum module_states st) {
    assert(self);
    
    GET_MOD(self);
    return mod->state == st;
}

/** Module state setters **/

int module_start(const void *self, int fd) {
    assert(self);
    
    GET_MOD(self);
    mod->fd = fd;
    MODULE_DEBUG("Starting module %s.\n", ((self_t *)self)->name);
    return module_resume(self);
}

int module_pause(const void *self) {
    assert(self);
    
    GET_MOD(self);
    int ret = epoll_ctl(context->epollfd, EPOLL_CTL_DEL, mod->fd, NULL);
    if (!ret) {
        mod->state = PAUSED;
        stop_children(mod);
    }
    return ret;
}

int module_resume(const void *self) {
    assert(self);
    
    GET_MOD(self);
    mod->ev.data.ptr = (void *)self;
    mod->ev.events = EPOLLIN;
    int ret = epoll_ctl(context->epollfd, EPOLL_CTL_ADD, mod->fd, &mod->ev);
    if (!ret) {
        mod->state = RUNNING;
        start_children(mod);

    }
    return ret;
}

int module_stop(const void *self) {
    assert(self);
    
    GET_MOD(self);
    MODULE_DEBUG("Stopping module %s.\n", ((self_t *)self)->name);
    mod->state = IDLE;
    return close(mod->fd); // implicitly calls EPOLL_CTL_DEL
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
