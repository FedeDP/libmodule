#include "modules.h"
#include "poll_priv.h"
#include <string.h>

static _ctor1_ void modules_init(void);
static _dtor0_ void modules_destroy(void);
static void evaluate_new_state(m_context *context);

map_t *ctx;
memalloc_hook memhook;

static void modules_init(void) {
    MODULE_DEBUG("Initializing libmodule %d.%d.%d.\n", MODULE_VERSION_MAJ, MODULE_VERSION_MIN, MODULE_VERSION_PAT);
    modules_set_memalloc_hook(NULL);
    ctx = module_map_new();
}

static void modules_destroy(void) {
    MODULE_DEBUG("Destroying libmodule.\n");
    module_map_free(ctx);
}

module_ret_code modules_set_memalloc_hook(const memalloc_hook *hook) {
    if (hook) {
        MOD_ASSERT(hook->_malloc, "NULL malloc fn.", MOD_ERR);
        MOD_ASSERT(hook->_realloc, "NULL realloc fn.", MOD_ERR);
        MOD_ASSERT(hook->_calloc, "NULL calloc fn.", MOD_ERR);
        MOD_ASSERT(hook->_free, "NULL free fn.", MOD_ERR);

        memhook._malloc = hook->_malloc;
        memhook._realloc = hook->_realloc;
        memhook._calloc = hook->_calloc;
        memhook._free = hook->_free;
    } else {
        memhook._malloc = malloc;
        memhook._realloc = realloc;
        memhook._calloc = calloc;
        memhook._free = free;
    }
    return MOD_OK;
}

module_ret_code modules_ctx_set_logger(const char *ctx_name, const log_cb logger) {
    MOD_ASSERT(logger, "NULL logger.", MOD_ERR);
    GET_CTX(ctx_name);
    
    c->logger = logger;
    return MOD_OK;
}

module_ret_code modules_ctx_loop_events(const char *ctx_name, const int max_events) {
    MOD_ASSERT((max_events > 0), "max_events parameter must be > 0.", MOD_ERR);
    GET_CTX(ctx_name);
    MOD_ASSERT((c->num_fds > 0), "No fds to loop on.", MOD_ERR);
    MOD_ASSERT(!c->looping, "Context already looping.", MOD_ERR);

    if (poll_init_pevents(&c->pevents, max_events) == MOD_OK) {
        c->quit = false;
        c->looping = true;
        c->quit_code = 0;
        c->max_events = max_events;

        /* Tell every module that loop is started */
        tell_system_pubsub_msg(c, LOOP_STARTED, NULL);

        while (!c->quit) {
            int nfds = poll_wait(c->fd, c->max_events, c->pevents);
            for (int i = 0; i < nfds; i++) {
                module_poll_t *p = poll_recv(i, c->pevents);
                if (p) {
                    CTX_GET_MOD(p->self->name, c);

                    msg_t msg;
                    fd_msg_t fd_msg;

                    if (p->fd == mod->pubsub_fd[0]) {
                        /* Received on pubsub interface */
                        *(bool *)&msg.is_pubsub = true;
                        read(p->fd, (void **)&msg.pubsub_msg, sizeof(pubsub_msg_t *));
                    } else {
                        /* Received from FD */
                        *(bool *)&msg.is_pubsub = false;
                        *(int *)&fd_msg.fd = p->fd;
                        fd_msg.userptr = p->userptr;
                        *(fd_msg_t **)&msg.fd_msg = &fd_msg;
                    }

                    mod->hook.recv(&msg, mod->userdata);

                    /* Properly free pubsub msg */
                    if (p->fd == mod->pubsub_fd[0]) {
                        destroy_pubsub_msg((pubsub_msg_t *)msg.pubsub_msg);
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
                }
            }
        }

        /* Tell every module that loop is stopped */
        tell_system_pubsub_msg(c, LOOP_STOPPED, NULL);
        
        /* Flush pubsub msg to avoid memleaks */
        module_map_iterate(c->modules, flush_pubsub_msg, NULL);
        poll_destroy_pevents(&c->pevents, &c->max_events);
        c->looping = false;
        return c->quit_code;
    }
    return MOD_ERR;
}

static void evaluate_new_state(m_context *context) {
    module_map_iterate(context->modules, evaluate_module, NULL);
}

module_ret_code modules_ctx_quit(const char *ctx_name, const uint8_t quit_code) {
    GET_CTX(ctx_name);
    
    if (c->looping) {
        c->quit = true;
        c->quit_code = quit_code;
        return MOD_OK;
    }
    MODULE_DEBUG("Context not looping.\n");
    return MOD_ERR;
}
