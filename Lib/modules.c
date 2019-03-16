#include "modules.h"
#include "poll_priv.h"

static _ctor1_ void modules_init(void);
static _dtor0_ void modules_destroy(void);
static void evaluate_new_state(m_context *c);
static module_ret_code loop_start(m_context *c, const int max_events);
static int recv_events(m_context *c, int timeout);
static int loop_stop(m_context *c);

map_t *ctx;
memalloc_hook memhook;

static void modules_init(void) {
    MODULE_DEBUG("Initializing libmodule %d.%d.%d.\n", MODULE_VERSION_MAJ, MODULE_VERSION_MIN, MODULE_VERSION_PAT);
    modules_set_memalloc_hook(NULL);
    ctx = map_new();
}

static void modules_destroy(void) {
    MODULE_DEBUG("Destroying libmodule.\n");
    map_free(ctx);
}

static void evaluate_new_state(m_context *c) {
    map_iterate(c->modules, evaluate_module, NULL);
}

static module_ret_code loop_start(m_context *c, const int max_events) {
    if (poll_init_pevents(&c->pevents, max_events) == MOD_OK) {
        c->looping = true;
        c->quit_code = 0;
        c->max_events = max_events;
        
        /* Tell every module that loop is started */
        tell_system_pubsub_msg(c, LOOP_STARTED, NULL);
        return MOD_OK;
    }
    return MOD_ERR;
}

static int recv_events(m_context *c, int timeout) {
    int nfds = poll_wait(c->fd, c->max_events, c->pevents, timeout);
    for (int i = 0; i < nfds; i++) {
        module_poll_t *p = poll_recv(i, c->pevents);
        if (p && p->self && p->self->mod) {
            module *mod = p->self->mod;
            
            msg_t msg;
            fd_msg_t fd_msg;
            
            if (p->fd == mod->pubsub_fd[0]) {
                /* Received on pubsub interface */
                *(bool *)&msg.is_pubsub = true;
                if (read(p->fd, (void **)&msg.pubsub_msg, sizeof(pubsub_msg_t *)) != sizeof(pubsub_msg_t *)) {
                    MODULE_DEBUG("Failed to read message for %s: %s\n", mod->name, strerror(errno));
                    *((pubsub_msg_t **)&msg.pubsub_msg) = NULL;
                }
            } else {
                /* Received from FD */
                *(bool *)&msg.is_pubsub = false;
                *(int *)&fd_msg.fd = p->fd;
                fd_msg.userptr = p->userptr;
                *(fd_msg_t **)&msg.fd_msg = &fd_msg;
            }
            
            if (!msg.is_pubsub || msg.pubsub_msg) {
                run_pubsub_cb(mod, &msg);
                
                /* Properly free pubsub msg */
                if (msg.is_pubsub) {
                    destroy_pubsub_msg((pubsub_msg_t *)msg.pubsub_msg);
                }
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
        if (errno != EINTR && errno != EAGAIN) {
            fprintf(stderr, "Module loop error: %s.\n", strerror(errno));
            c->quit = true;
            c->quit_code = errno;
        } else {
            nfds = 0; // return < 0 only for real errors
        }
    }
    return nfds;
}

static int loop_stop(m_context *c) {
    /* Tell every module that loop is stopped */
    tell_system_pubsub_msg(c, LOOP_STOPPED, NULL);
    
    /* Flush pubsub msg to avoid memleaks */
    map_iterate(c->modules, flush_pubsub_msg, NULL);
    
    poll_destroy_pevents(&c->pevents, &c->max_events);
    c->looping = false;
    c->quit = false;
    return c->quit_code;
}

/** Public API **/

module_ret_code modules_set_memalloc_hook(const memalloc_hook *hook) {
    if (hook) {
        MOD_ASSERT(hook->_malloc, "NULL malloc fn.", MOD_ERR);
        MOD_ASSERT(hook->_realloc, "NULL realloc fn.", MOD_ERR);
        MOD_ASSERT(hook->_calloc, "NULL calloc fn.", MOD_ERR);
        MOD_ASSERT(hook->_free, "NULL free fn.", MOD_ERR);
        memcpy(&memhook, hook, sizeof(memalloc_hook));
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
    MOD_ASSERT(c->num_fds > 0, "No fds to loop on.", MOD_ERR);
    MOD_ASSERT(!c->looping, "Context already looping.", MOD_ERR);

    if (loop_start(c, max_events) == MOD_OK) {
        while (!c->quit) {
            recv_events(c, -1);
        }
        return loop_stop(c);
    }
    return MOD_ERR;
}

module_ret_code modules_ctx_quit(const char *ctx_name, const uint8_t quit_code) {
    FIND_CTX(ctx_name);
    MOD_ASSERT(c->looping, "Context not looping.", MOD_ERR);
       
    c->quit = true;
    c->quit_code = quit_code;
    return MOD_OK;
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
        return loop_start(c, MODULE_MAX_EVENTS);
    }
    
    if (c->quit) {
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
