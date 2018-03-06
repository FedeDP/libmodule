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
} module;

typedef struct {
    int quit;
    int epollfd;
    int num_modules;
    module *modules;
    struct epoll_event *events;
} state_t;

static int modules_add_descriptors(void);
static void evaluate_new_state(void);

static state_t st;

static __attribute__((constructor)) void modules_init(void) {
    MODULE_DEBUG("Initializing libmodules library.\n");
    memset(&st, 0, sizeof(state_t));
}

static __attribute__((destructor)) void modules_destroy(void) {
    MODULE_DEBUG("Destroying libmodules library.\n");
    for (int i = 0; i < st.num_modules; i++) {
        close(st.modules[i].fd); // will implicitly call EPOLL_CTL_DEL
        st.modules[i].hook->destroy();
        free((void *)st.modules[i].self.name);
    }
    free(st.modules);
    free(st.events);
}

const self_t *module_set_self(const char *name, userhook *hook) {
    int idx = st.num_modules;
    st.modules = realloc(st.modules, (idx + 1) * sizeof(module));
    st.modules[idx].hook = hook;
    st.modules[idx].state = IDLE;
    st.modules[idx].self.name = strdup(name);
    st.modules[idx].self.id = idx;
    st.num_modules++;
    
    return &st.modules[idx].self;
}

static int modules_add_descriptors(void) {
    int err = 0;
    for (int i = 0; i < st.num_modules; i++) {
        st.events[i].data.ptr = &st.modules[i];
        st.events[i].events = EPOLLIN;
        if (epoll_ctl(st.epollfd, EPOLL_CTL_ADD, st.modules[i].fd, &st.events[i]) != 0 ) {
            err++;
        }
    }
    return err;
}

// use epoll: https://www.ulduzsoft.com/2014/01/select-poll-epoll-practical-difference-for-system-architects/
// https://github.com/millken/c-example/blob/master/epoll-example.c
// https://www.safaribooksonline.com/library/view/linux-system-programming/0596009585/ch04s02.html
void modules_loop(void) {
    st.epollfd = epoll_create1(EPOLL_CLOEXEC); 
    assert(st.epollfd >= 0);
    
    st.events = calloc(st.num_modules, sizeof(struct epoll_event));
    assert(st.events);
    
    if (modules_add_descriptors() == 0) {
        struct epoll_event *pevents = calloc(st.num_modules, sizeof(struct epoll_event));
        assert(pevents);
        
        while (!st.quit) {
            int nfds = epoll_wait(st.epollfd, st.events, st.num_modules, -1);
            if (nfds <= 0) {
                fprintf(stderr, "epoll failed.\n");
            } else {
                for (int i = 0; i < nfds; i++) {
                    if (pevents[i].events & EPOLLIN) {
                        void (*pollCb)(void) = pevents[i].data.ptr;
                        pollCb();
                    }
                }
                evaluate_new_state();
            }
        }
        free(pevents);
    } else {
        fprintf(stderr, "modules_add_descriptors failed.\n");
    }
    close(st.epollfd);
}

static void evaluate_new_state(void) {
    for (int i = 0; i < st.num_modules; i++) {
        if (module_is(&st.modules[i].self, IDLE) 
            && st.modules[i].hook->stateChange()) {
            st.modules[i].hook->init();
        }
    }
}

userhook *const module_get_hook(const self_t *self) {
    return st.modules[self->id].hook;
}

/** Module state getters **/

int module_is(const self_t *self, const enum module_states s) {
    return st.modules[self->id].state == s;
}

/** Module state setters **/

void module_pause(const self_t *self) {
    module_log("paused.\n");
    st.modules[self->id].state = PAUSED;
    epoll_ctl(st.epollfd, EPOLL_CTL_DEL, st.modules[self->id].fd, NULL);
}

void module_resume(const self_t *self) {
    module_log("resumed.\n");
    st.modules[self->id].state = STARTED;
    epoll_ctl(st.epollfd, EPOLL_CTL_ADD, st.modules[self->id].fd, &st.events[self->id]);
}
