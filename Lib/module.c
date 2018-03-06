#include <module.h> 
#include <string.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <assert.h>

#ifdef DEBUG
#define MODULE_DEBUG printf
#else
#define MODULE_DEBUG (void)
#endif

/* Struct that holds data for each module */
typedef struct {
    userhook *hook;
    enum module_states state;             // module's state
    self_t self;                          // module's info available to external world
    int fd;                               // file descriptor to be polled
    struct epoll_event ev;
} module;

typedef struct {
    int quit;
    int epollfd;
    int num_modules;
    module *modules;
} state_t;

static void evaluate_new_state(void);
static void evaluate_module(int idx);
static int module_start(const self_t *self, int fd);

static state_t st;

static __attribute__((constructor)) void modules_init(void) {
    MODULE_DEBUG("Initializing libmodules library.\n");
    memset(&st, 0, sizeof(state_t));
    
    st.epollfd = epoll_create1(EPOLL_CLOEXEC); 
    assert(st.epollfd >= 0);
}

static __attribute__((destructor)) void modules_destroy(void) {
    MODULE_DEBUG("Destroying libmodules library.\n");
    for (int i = 0; i < st.num_modules; i++) {
        close(st.modules[i].fd); // will implicitly call EPOLL_CTL_DEL
        st.modules[i].hook->destroy();
        free((void *)st.modules[i].self.name);
    }
    free(st.modules);
    close(st.epollfd);
}

const self_t *module_set_self(const char *name, userhook *hook) {
    assert(name);
    assert(hook);
    
    int idx = st.num_modules;
    st.modules = realloc(st.modules, (idx + 1) * sizeof(module));
    st.modules[idx].hook = hook;
    st.modules[idx].state = IDLE;
    st.modules[idx].self.name = strdup(name);
    st.modules[idx].self.id = idx;
    st.num_modules++;
    
    /* Check if module must be started and eventually start it */
    if (!st.modules[idx].hook->check()) {
        evaluate_module(idx);
    }
    
    return &st.modules[idx].self;
}

// use epoll: https://www.ulduzsoft.com/2014/01/select-poll-epoll-practical-difference-for-system-architects/
// https://github.com/millken/c-example/blob/master/epoll-example.c
// https://www.safaribooksonline.com/library/view/linux-system-programming/0596009585/ch04s02.html
void modules_loop(void) {
    struct epoll_event *pevents = calloc(st.num_modules, sizeof(struct epoll_event));
    assert(pevents);
        
    while (!st.quit) {
        int nfds = epoll_wait(st.epollfd, pevents, st.num_modules, -1);
        if (nfds <= 0) {
            fprintf(stderr, "epoll failed.\n");
        } else {
            for (int i = 0; i < nfds; i++) {
                if (pevents[i].events & EPOLLIN) {
                    self_t *self = pevents[i].data.ptr;
                    st.modules[self->id].hook->pollCb(st.modules[self->id].fd);
                }
            }
            evaluate_new_state();
        }
    }
    free(pevents);
}

void modules_quit(void) {
    st.quit = 1;
}

static void evaluate_new_state(void) {
    for (int i = 0; i < st.num_modules; i++) {
        evaluate_module(i);
    }
}

static void evaluate_module(int idx) {
    if (module_is(&st.modules[idx].self, IDLE) 
        && st.modules[idx].hook->stateChange()) {
        
        int fd = st.modules[idx].hook->init();
        module_start(&st.modules[idx].self, fd);
    }
}

static int module_start(const self_t *self, int fd) {
    st.modules[self->id].fd = fd;
    MODULE_DEBUG("Starting module %s on fd %d.\n", self->name, st.modules[self->id].fd);
    return module_resume(self);
}

userhook *const module_get_hook(const self_t *self) {
    return st.modules[self->id].hook;
}

/** Module state getter **/

int module_is(const self_t *self, const enum module_states s) {
    return st.modules[self->id].state == s;
}

/** Module state setters **/

int module_pause(const self_t *self) {
    st.modules[self->id].state = PAUSED;
    int ret = epoll_ctl(st.epollfd, EPOLL_CTL_DEL, st.modules[self->id].fd, NULL);
    if (!ret) {
        st.modules[self->id].state = PAUSED;
    } else {
        // module_disable here
        st.modules[self->id].state = DISABLED;
    }
    return ret;
}

int module_resume(const self_t *self) {
    st.modules[self->id].ev.data.ptr = (void *)self;
    st.modules[self->id].ev.events = EPOLLIN;
    int ret = epoll_ctl(st.epollfd, EPOLL_CTL_ADD, st.modules[self->id].fd, &st.modules[self->id].ev);
    if (!ret) {
        st.modules[self->id].state = STARTED;
    } else {
        // module_disable here
        st.modules[self->id].state = DISABLED;
    }
    return ret;
}

// int module_disable/module_destroy()
