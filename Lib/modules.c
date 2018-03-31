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
    
    struct epoll_event pevents[MAX_EVENTS];
    int ret = MOD_OK;
    while (!c->quit) {
        int nfds = epoll_wait(c->epollfd, pevents, c->num_fds, -1);
        if (nfds < 0) {
            ret = MOD_ERR;
            break;
        } else {
            for (int i = 0; i < nfds; i++) {
                if (pevents[i].events & EPOLLIN) {
                    module_poll_t *p = (module_poll_t *)pevents[i].data.ptr;
                    
                    CTX_GET_MOD(p->self->name, c);
                    
                    const msg_t msg = { p->fd, NULL };
                    mod->hook->recv(&msg, mod->userdata);
                }
            }
            evaluate_new_state(c);
        }
    }
    return ret;
}

static void evaluate_new_state(m_context *context) {
    hashmap_iterate(context->modules, evaluate_module, NULL);
}

module_ret_code modules_ctx_quit(const char *ctx_name) {
    GET_CTX(ctx_name);
    
    c->quit = 1;
    return MOD_OK;
}
