#include <modules.h>
#include <poll_priv.h>

static _ctor1_ void modules_init(void);
static _dtor0_ void modules_destroy(void);
static void evaluate_new_state(m_context *context);

static void modules_init(void) {
    MODULE_DEBUG("Initializing libmodule %d.%d.%d.\n", MODULE_VERSION_MAJ, MODULE_VERSION_MIN, MODULE_VERSION_PAT);
    ctx = hashmap_new();
    modules_set_memalloc_hook(NULL);
}

static void modules_destroy(void) {
    MODULE_DEBUG("Destroying libmodule.\n");
    hashmap_free(ctx);
}

module_ret_code modules_set_memalloc_hook(memalloc_hook *hook) {
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

module_ret_code modules_ctx_set_logger(const char *ctx_name, log_cb logger) {
    MOD_ASSERT(logger, "NULL logger.", MOD_ERR);
    GET_CTX(ctx_name);
    
    c->logger = logger;
    return MOD_OK;
}

module_ret_code modules_ctx_loop(const char *ctx_name) {
    GET_CTX(ctx_name);
    
    if (c->num_fds > 0 && !c->looping) {
        c->quit = 0;
        c->looping = 1;
        while (!c->quit) {
            int nfds = poll_wait(c->fd, c->num_fds);
        
            MOD_ASSERT((nfds > 0), "Context loop error.", MOD_ERR);
            for (int i = 0; i < nfds; i++) {
                module_poll_t *p = poll_recv(i);
                MOD_ASSERT(p, "Context loop error.", MOD_ERR);
                CTX_GET_MOD(p->self->name, c);
                
                const msg_t msg = { .is_pubsub = 0, .fd = p->fd };
                mod->hook.recv(&msg, mod->userdata);
            }
            evaluate_new_state(c);
        }
        c->looping = 0;
        return MOD_OK;
    }
    MODULE_DEBUG(c->looping ? "Context already looping.\n" : "No events/fds specified.\n");
    return MOD_ERR;
}

static void evaluate_new_state(m_context *context) {
    hashmap_iterate(context->modules, evaluate_module, NULL);
}

module_ret_code modules_ctx_quit(const char *ctx_name) {
    GET_CTX(ctx_name);
    
    if (c->looping) {
        c->quit = 1;
        return MOD_OK;
    }
    MODULE_DEBUG("Context not looping.\n");
    return MOD_ERR;
}
