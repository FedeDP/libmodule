#include <modules.h>
#include <module_priv.h>

static _ctor1_ void modules_init(void);
static _dtor0_ void modules_destroy(void);
static void evaluate_new_state(m_context *context);

static void modules_init(void) {
    MODULE_DEBUG("Initializing library.\n");
    ctx = hashmap_new();
}

static void modules_destroy(void) {
    MODULE_DEBUG("Destroying library.\n");
    hashmap_free(ctx);
}

module_ret_code modules_ctx_set_logger(const char *ctx_name, log_cb logger) {
    MOD_ASSERT(logger, "NULL logger. Fallbacking to default.", MOD_ERR);
    GET_CTX(ctx_name);
    
    c->logger = logger;
    return MOD_OK;
}

module_ret_code modules_ctx_loop(const char *ctx_name) {
    GET_CTX(ctx_name);
    
#ifdef __linux__
    struct epoll_event pevents[MAX_EVENTS] = {{ 0 }};
#else
    struct kevent pevents[MAX_EVENTS] = {{ 0 }};
#endif
    while (!c->quit) {
#ifdef __linux__
        int nfds = epoll_wait(c->fd, pevents, c->num_fds, -1);
#else
        int nfds = kevent(c->fd, pevents, c->num_fds, NULL, 0, NULL);
#endif
        MOD_ASSERT(nfds > 0, "Context loop error.", MOD_ERR);
        for (int i = 0; i < nfds; i++) {
#ifdef __linux__
            module_poll_t *p = (module_poll_t *)pevents[i].data.ptr;
#else
            module_poll_t *p = (module_poll_t *)pevents[i].udata;
#endif
                
            CTX_GET_MOD(p->self->name, c);
                
            const msg_t msg = { p->fd, NULL };
            mod->hook->recv(&msg, mod->userdata);
        }
        evaluate_new_state(c);
    }
    return MOD_OK;
}

static void evaluate_new_state(m_context *context) {
    hashmap_iterate(context->modules, evaluate_module, NULL);
}

module_ret_code modules_ctx_quit(const char *ctx_name) {
    GET_CTX(ctx_name);
    
    c->quit = 1;
    return MOD_OK;
}
