#include "modules.h"
#include "poll_priv.h"
#include <pthread.h>

static void *thread_loop(void *param);
static map_ret_code main_loop(void *data, const char *key, void *value);
static _ctor1_ void modules_init(void);
static _dtor0_ void modules_destroy(void);
static void evaluate_new_state(m_context *c);
static module_ret_code loop_start(m_context *c, const int max_events);
static uint8_t loop_stop(m_context *c);
static inline module_ret_code loop_quit(m_context *c, const uint8_t quit_code);
static int recv_events(m_context *c, int timeout);

map_t *ctx;
memhook_t memhook;

void modules_pre_start(void) {
    MODULE_DEBUG("Pre-starting libmodule.");
}

static void *thread_loop(void *param) {
    const char *key = (const char *)param;
    
    modules_ctx_loop_events(key, MODULES_MAX_EVENTS);
    return NULL;
}

static map_ret_code main_loop(void *data, const char *key, void *value) {
    pthread_t *th = *(pthread_t **)data;
    if (th) {
        static int i = 0;
        pthread_create(&th[i++], NULL, thread_loop, (void *)key);
        return MAP_OK;
    }
    *(char **)data = (char *)key;
    return MAP_ERR;
}

/*
 * This is an exported global weak symbol.
 * It means that if a program does not implement any main(int, char *[]),
 * this will be used by default.
 *
 * All it does is:
 * if ctx_num > 1 -> allocating ctx_num pthreads and each of them will loop on its context
 * else           -> just loops on only ctx on main thread 
 */
int main(int argc, char *argv[]) {
    void *th = NULL;
    
    /* If there is more than 1 registered ctx, alloc as many pthreads as needed */
    if (map_length(ctx) > 1) {
        MODULE_DEBUG("Allocating %ld pthreads.\n", map_length(ctx));
        th = memhook._calloc(map_length(ctx), sizeof(pthread_t));
    }
    
    /*
     * main_loop returns MAP_ERR for single-ctx runs,
     * where we only need a pointer to ctx key.
     * Ugliness warning: passing a void** ptr that is either an array of pthreads
     * or is just a space to point to single-ctx key.
     */
    if (map_iterate(ctx, main_loop, &th) == MAP_ERR) {
        MODULE_DEBUG("Running in single ctx mode: '%s'\n", (const char *)th);
        return modules_ctx_loop_events((const char *)th, MODULES_MAX_EVENTS);
    }
    
    /* If more than 1 ctx is registered, we should join all threads */
    MODULE_DEBUG("Waiting all threads.\n");
    for (int i = 0; i < map_length(ctx); i++) {
        pthread_join(((pthread_t *)th)[i], NULL);
    }
    memhook._free(th);
    return MOD_OK;
}

static void modules_init(void) {
    MODULE_DEBUG("Initializing libmodule %d.%d.%d.\n", MODULE_VERSION_MAJ, MODULE_VERSION_MIN, MODULE_VERSION_PAT);
    modules_set_memhook(NULL);
    ctx = map_new(false, NULL);
}

static void modules_destroy(void) {
    MODULE_DEBUG("Destroying libmodule.\n");
    map_free(ctx);
    
    /* 
     * When module_load() loads any module, they'll get unloaded after modules_destroy(),
     * when they're unlinked from program.
     * Avoid dereferencing a freed ctx map then.
     */
    ctx = NULL; 
}

static void evaluate_new_state(m_context *c) {
    map_iterate(c->modules, evaluate_module, NULL);
}

static module_ret_code loop_start(m_context *c, const int max_events) {
    if (poll_init_pevents(&c->pevents, max_events) == MOD_OK) {
        c->looping = true;
        c->quit = false;
        c->quit_code = 0;
        c->max_events = max_events;

        /* Eventually start any IDLE module */
        evaluate_new_state(c);

        /* Tell every RUNNING module that loop is started */
        tell_system_pubsub_msg(NULL, c, LOOP_STARTED, NULL, NULL);
        return MOD_OK;
    }
    return MOD_ERR;
}

static uint8_t loop_stop(m_context *c) {
    /* Tell every module that loop is stopped */
    tell_system_pubsub_msg(NULL, c, LOOP_STOPPED, NULL, NULL);

    /* Flush pubsub msg to avoid memleaks */
    map_iterate(c->modules, flush_pubsub_msgs, NULL);

    poll_destroy_pevents(&c->pevents, &c->max_events);
    c->looping = false;
    return c->quit_code;
}

static inline module_ret_code loop_quit(m_context *c, const uint8_t quit_code) {
    c->quit = true;
    c->quit_code = quit_code;
    return MOD_OK;
}

static int recv_events(m_context *c, int timeout) {
    int nfds = poll_wait(c->fd, c->max_events, c->pevents, timeout);
    for (int i = 0; i < nfds; i++) {
        module_poll_t *p = poll_recv(i, c->pevents);
        if (p && p->self && p->self->mod) {
            module *mod = p->self->mod;
            
            msg_t msg;
            fd_msg_t fd_msg;
            ps_priv_t *ps_msg;
            
            if (p->fd == mod->pubsub_fd[0]) {
                /* Received on pubsub interface */
                msg.is_pubsub = true;
                if (read(p->fd, (void **)&ps_msg, sizeof(ps_priv_t *)) != sizeof(ps_priv_t *)) {
                    MODULE_DEBUG("Failed to read message for '%s': %s\n", mod->name, strerror(errno));
                    msg.ps_msg = NULL;
                } else {
                    msg.ps_msg = &ps_msg->msg;
                }
            } else {
                /* Received from FD */
                msg.is_pubsub = false;
                fd_msg.fd = p->fd;
                fd_msg.userptr = p->userptr;
                msg.fd_msg = &fd_msg;
            }
            
            if (!msg.is_pubsub || (msg.ps_msg && msg.ps_msg->type != MODULE_POISONPILL)) {
                run_pubsub_cb(mod, &msg);
            } else if (msg.ps_msg) {
                MODULE_DEBUG("PoisonPilling '%s'.\n", mod->name);
                stop(mod, true);
            }
        } else {
            /* Forward error to below handling code */
            errno = ENXIO;
            nfds = -1;
        }
    }
    
    if (nfds > 0) {
        evaluate_new_state(c);
    } else if (nfds < 0) {
        /* Quit and return < 0 only for real errors */
        if (errno != EINTR && errno != EAGAIN) {
            fprintf(stderr, "Ctx '%s' loop error: %s.\n", c->name, strerror(errno));
            loop_quit(c, errno);
        } else {
            nfds = 0;
        }
    }
    return nfds;
}

/** Public API **/

module_ret_code modules_set_memhook(const memhook_t *hook) {
    if (hook) {
        MOD_ASSERT(hook->_malloc, "NULL malloc fn.", MOD_ERR);
        MOD_ASSERT(hook->_realloc, "NULL realloc fn.", MOD_ERR);
        MOD_ASSERT(hook->_calloc, "NULL calloc fn.", MOD_ERR);
        MOD_ASSERT(hook->_free, "NULL free fn.", MOD_ERR);
        memcpy(&memhook, hook, sizeof(memhook_t));
    } else {
        memhook._malloc = malloc;
        memhook._realloc = realloc;
        memhook._calloc = calloc;
        memhook._free = free;
    }
    return MOD_OK;
}

module_ret_code modules_ctx_set_logger(const char *ctx_name, const log_cb logger) {
    MOD_PARAM_ASSERT(logger);
    FIND_CTX(ctx_name);
    
    c->logger = logger;
    return MOD_OK;
}

module_ret_code modules_ctx_loop_events(const char *ctx_name, const int max_events) {
    MOD_PARAM_ASSERT(max_events > 0);
    FIND_CTX(ctx_name);
    MOD_ASSERT(!c->looping, "Context already looping.", MOD_ERR);

    if (loop_start(c, max_events) == MOD_OK) {
        while (!c->quit && c->running_mods) {
            recv_events(c, -1);
        }
        return loop_stop(c);
    }
    return MOD_ERR;
}

module_ret_code modules_ctx_quit(const char *ctx_name, const uint8_t quit_code) {
    FIND_CTX(ctx_name);
    MOD_ASSERT(c->looping, "Context not looping.", MOD_ERR);
       
    return loop_quit(c, quit_code);
}

module_ret_code modules_ctx_get_fd(const char *ctx_name, int *fd) {
    MOD_PARAM_ASSERT(fd);
    FIND_CTX(ctx_name);
    
    *fd = dup(c->fd);
    return MOD_OK;
}

module_ret_code modules_ctx_dispatch(const char *ctx_name, int *ret) {
    MOD_PARAM_ASSERT(ret);
    FIND_CTX(ctx_name);
    
    if (!c->looping) {
        /* Ok, start now */
        *ret = 0;
        return loop_start(c, MODULES_MAX_EVENTS);
    }
    
    if (c->quit || !c->running_mods) {
        /* We are stopping! */
        *ret = loop_stop(c);
        /* 
         * MOD_ERR to let client know it's time to quit.
         * You've called dispatch (for the first time) on a quitted ctx.
         */
        return MOD_ERR; 
    }
    
    /* Recv new events, no timeout */
    *ret = recv_events(c, 0);
    return *ret >= 0 ? MOD_OK : MOD_ERR;
}
