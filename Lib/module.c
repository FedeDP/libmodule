#include <module.h> 
#include <string.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <assert.h>
#include <stdarg.h>

#ifdef DEBUG
#define MODULE_DEBUG printf("Libmodule: "); printf
#else
#define MODULE_DEBUG (void)
#endif

/* Struct that holds data for each module */
typedef struct {
    userhook *hook;
    const void *userdata;
    enum module_states state;             // module's state
    self_t self;                          // module's info available to external world
    int fd;                               // file descriptor to be polled
    struct epoll_event ev;
} module;

typedef struct {
    int quit;
    int epollfd;
    error_cb on_error;
    int num_modules;
    module *modules;
} m_context;

static void evaluate_new_state(void);
static void evaluate_module(int idx);
static void default_error(const char *error_msg);

static m_context ctx;

static _ctor0_ void modules_init(void) {
    MODULE_DEBUG("Initializing library.\n");
    memset(&ctx, 0, sizeof(m_context));
    
    ctx.epollfd = epoll_create1(EPOLL_CLOEXEC); 
    assert(ctx.epollfd >= 0);
    
    ctx.on_error = default_error;
}

static _dtor0_ void modules_destroy(void) {
    MODULE_DEBUG("Destroying library.\n");
    free(ctx.modules);
    close(ctx.epollfd);
}

void module_register(const char *name, const self_t **self, userhook *hook) {
    assert(name);

    MODULE_DEBUG("Registering module %s.\n", name);
    
    int idx = ctx.num_modules;
    
    module *tmp = realloc(ctx.modules, (idx + 1) * sizeof(module));
    assert(tmp);
    
    ctx.modules = tmp;
    ctx.modules[idx].hook = hook;
    ctx.modules[idx].state = IDLE;
    ctx.modules[idx].self.name = strdup(name);
    ctx.modules[idx].self.id = idx;
    ctx.num_modules++;
    
    *self = &ctx.modules[idx].self;
    evaluate_module(idx);
}

void module_deregister(const self_t *self) {
    if (self) {
        MODULE_DEBUG("Deregistering module %s.\n", self->name);
        // FIXME: when using linked list / hashmap, remove this node from modules
        module_stop(self);
        ctx.modules[self->id].hook->destroy();
        ctx.modules[self->id].state = DESTROYED;
        free((void *)self->name);
    }
}

void modules_loop(void) {
    struct epoll_event *pevents = calloc(ctx.num_modules, sizeof(struct epoll_event));
    assert(pevents);
        
    while (!ctx.quit) {
        int nfds = epoll_wait(ctx.epollfd, pevents, ctx.num_modules, -1);
        if (nfds < 0) {
            ctx.on_error("epoll failed.");
        } else {
            for (int i = 0; i < nfds; i++) {
                if (pevents[i].events & EPOLLIN) {
                    self_t *self = (self_t *) pevents[i].data.ptr;
                    message_t msg = { ctx.modules[self->id].fd, NULL, NULL };
                    ctx.modules[self->id].hook->recv(&msg, ctx.modules[self->id].userdata);
                }
            }
            evaluate_new_state();
        }
    }
    free(pevents);
}

void modules_quit(void) {
    ctx.quit = 1;
}

void modules_on_error(error_cb on_error) {
    ctx.on_error = on_error;
}

static void evaluate_new_state(void) {
    for (int i = 0; i < ctx.num_modules; i++) {
        evaluate_module(i);
    }
}

static void evaluate_module(int idx) {
    if (module_is(&ctx.modules[idx].self, IDLE) 
        && ctx.modules[idx].hook->evaluate()) {
        
        int fd = ctx.modules[idx].hook->init();
        module_start(&ctx.modules[idx].self, fd);
    }
}

void module_become(const self_t *self,  recv_cb new_recv) {
    assert(self);
    
    ctx.modules[self->id].hook->recv = new_recv;
}

void module_log(const self_t *self, const char *fmt, ...) {
    assert(self);
    
    va_list args;
    va_start(args, fmt);
    printf("%s: ", self->name);
    vprintf(fmt, args);
    va_end(args);
}

void module_set_userdata(const self_t *self, const void *userdata) {
    assert(self);
    
    ctx.modules[self->id].userdata = userdata;
}

static void default_error(const char *error_msg) {
    fprintf(stderr, "Libmodule error: %s\n", error_msg);
}

/** Module state getter **/

int module_is(const self_t *self, const enum module_states s) {
    assert(self);
    
    return ctx.modules[self->id].state == s;
}

/** Module state setters **/

int module_start(const self_t *self, int fd) {
    assert(self);
    
    ctx.modules[self->id].fd = fd;
    MODULE_DEBUG("Starting module %s.\n", self->name);
    return module_resume(self);
}

int module_pause(const self_t *self) {
    assert(self);
    
    int ret = epoll_ctl(ctx.epollfd, EPOLL_CTL_DEL, ctx.modules[self->id].fd, NULL);
    if (!ret) {
        ctx.modules[self->id].state = PAUSED;
    }
    return ret;
}

int module_resume(const self_t *self) {
    assert(self);
    
    ctx.modules[self->id].ev.data.ptr = (void *)self;
    ctx.modules[self->id].ev.events = EPOLLIN;
    int ret = epoll_ctl(ctx.epollfd, EPOLL_CTL_ADD, ctx.modules[self->id].fd, &ctx.modules[self->id].ev);
    if (!ret) {
        ctx.modules[self->id].state = STARTED;
    }
    return ret;
}

int module_stop(const self_t *self) {
    assert(self);
    
    MODULE_DEBUG("Stopping module %s.\n", self->name);
    ctx.modules[self->id].state = IDLE;
    return close(ctx.modules[self->id].fd); // implicitly calls EPOLL_CTL_DEL
}
