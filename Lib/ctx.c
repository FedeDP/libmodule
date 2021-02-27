#include "mod.h"
#include "ctx.h"
#include "fs_priv.h"

static void ctx_dtor(void *data);
static void default_logger(const m_mod_t *mod, const char *fmt, va_list args);
static int loop_start(m_ctx_t *c, const int max_events);
static uint8_t loop_stop(m_ctx_t *c);
static inline int loop_quit(m_ctx_t *c, const uint8_t quit_code);
static int recv_events(m_ctx_t *c, int timeout);

#define M_CTX_DEFAULT_EVENTS   64

static void ctx_dtor(void *data) {
    M_DEBUG("Destroying context.\n");
    m_ctx_t *context = (m_ctx_t *)data;
    m_map_free(&context->modules);
    poll_destroy(&context->ppriv);
    memhook._free(context->fs_root);
    memhook._free(context->name);
}

static void default_logger(const m_mod_t *mod, const char *fmt, va_list args) {
    if (mod) {
        printf("[%s]|%s|: ", ctx->name, mod->name);
    }
    vprintf(fmt, args);
}

static int loop_start(m_ctx_t *c, const int max_events) {
    c->ppriv.max_events = max_events;
    int ret = poll_init(&c->ppriv);
    if (ret == 0) {
        m_mem_ref(c); // Ensure ctx keeps existing while we loop
        
        int fs_ret = fs_init(c);
        if (fs_ret != 0) {
            M_WARN("Failed to initialize fuse fs: %s\n", strerror(-fs_ret));
        }

        c->looping = true;
        c->quit = false;
        c->quit_code = 0;
        
        /* Eventually start any IDLE module */
        m_map_iterate(c->modules, evaluate_module, NULL);

        /* Tell every RUNNING module that loop is started */
        tell_system_pubsub_msg(NULL, c, M_PS_CTX_STARTED, NULL, NULL);
    }
    return ret;
}

static uint8_t loop_stop(m_ctx_t *c) {
    /* Tell every module that loop is stopped */
    tell_system_pubsub_msg(NULL, c, M_PS_CTX_STOPPED, NULL, NULL);

    /* Flush pubsub msg to avoid memleaks */
    m_map_iterate(c->modules, flush_pubsub_msgs, NULL);
    
    /* Destroy FS */
    fs_end(c);
    
    poll_clear(&c->ppriv);
    c->ppriv.max_events = 0;
    c->looping = false;
    
    int ret = c->quit_code;
    
    m_mem_unref(c);
    return ret;
}

static inline int loop_quit(m_ctx_t *c, const uint8_t quit_code) {
    c->quit = true;
    c->quit_code = quit_code;
    return 0;
}

static int recv_events(m_ctx_t *c, int timeout) {    
    errno = 0;
    int recved = 0;
    const int nfds = poll_wait(&c->ppriv, timeout);    
    for (int i = 0; i < nfds; i++) {
        ev_src_t *p = poll_recv(&c->ppriv, i);
        if (p && p->type == M_SRC_TYPE_FD && p->mod == NULL) {
            /* Received from fuse */
            fs_process(c);
            recved++;
            continue;
        }
        
        if (p && p->mod) {
            m_mod_t *mod = p->mod;

            errno = 0;
            m_evt_t *msg = new_evt(p->type);
            if (msg) {
                M_DEBUG("'%s' received %u type msg.\n", mod->name, msg->type);
                switch (msg->type) {
                case M_SRC_TYPE_FD:
                    /* Received from FD */
                    msg->fd_msg = m_mem_new(sizeof(msg->fd_msg), NULL);
                    msg->fd_msg->fd = p->fd_src.fd;
                    break;
                case M_SRC_TYPE_PS: {
                    ps_priv_t *ps_msg;
                    /* Received on pubsub interface */
                    if (read(p->fd_src.fd, (void **)&ps_msg, sizeof(ps_priv_t *)) != sizeof(ps_priv_t *)) {
                        M_DEBUG("Failed to read message: %s\n", strerror(errno));
                    } else {
                        msg->ps_msg = &ps_msg->msg;
                        p = ps_msg->sub;            // Use real event source, ie: topic subscription if any
                    }
                    break;
                }
                case M_SRC_TYPE_SGN:
                    msg->sgn_msg = m_mem_new(sizeof(msg->sgn_msg), NULL);
                    if (poll_consume_sgn(&c->ppriv, i, p, msg->sgn_msg) == 0) {
                        msg->sgn_msg->signo = p->sgn_src.sgs.signo;
                    }
                    break;
                case M_SRC_TYPE_TMR:
                    msg->tmr_msg = m_mem_new(sizeof(msg->sgn_msg), NULL);
                    if (poll_consume_tmr(&c->ppriv, i, p,  msg->tmr_msg) == 0) {
                        msg->tmr_msg->ms = p->tmr_src.its.ms;
                    }
                    break;
                case M_SRC_TYPE_PATH:
                    msg->pt_msg = m_mem_new(sizeof(msg->pt_msg), NULL);
                    if (poll_consume_pt(&c->ppriv, i, p, msg->pt_msg) == 0) {
                        msg->pt_msg->path = p->path_src.pt.path;
                    }
                    break;
                case M_SRC_TYPE_PID:
                    msg->pid_msg = m_mem_new(sizeof(msg->pid_msg), NULL);
                    if (poll_consume_pid(&c->ppriv, i, p, msg->pid_msg) == 0) {
                        msg->pid_msg->pid = p->pid_src.pid.pid;
                    }
                    break;
                case M_SRC_TYPE_TASK:
                    msg->task_msg = m_mem_new(sizeof(msg->task_msg), NULL);
                    if (poll_consume_task(&c->ppriv, i, p, msg->task_msg) == 0) {
                        msg->task_msg->tid = p->task_src.tid.tid;
                        pthread_join(p->task_src.th, NULL);
                    }
                    break;
                default:
                    M_DEBUG("Unmanaged src %d.\n", msg->type);
                    break;
                }
            }

            if (errno == 0) {
                recved++;
                if (msg->type != M_SRC_TYPE_PS || msg->ps_msg->type != M_PS_MOD_POISONPILL) {
                    run_pubsub_cb(mod, msg, p);
                } else {
                    M_DEBUG("PoisonPilling '%s'.\n", mod->name);
                    stop(mod, true);
                }
                
                /* Remove it if it was a oneshot event */
                if (p && p->flags & M_SRC_ONESHOT) {
                    if (p->type != M_SRC_TYPE_PS) {
                        m_bst_remove(mod->srcs[p->type], p);
                    } else {
                        m_map_remove(mod->subscriptions, p->ps_src.topic);
                    }
                }
            }
        } else {
            /* Forward error to below handling code */
            errno = EAGAIN;
        }
    }
    
    if (recved > 0 && errno == 0) {
        m_map_iterate(c->modules, evaluate_module, NULL);
    } else if (errno) {
        /* Quit and return < 0 only for real errors */
        if (errno != EINTR && errno != EAGAIN) {
            fprintf(stderr, "Ctx '%s' loop error: %s.\n", c->name, strerror(errno));
            loop_quit(c, errno);
            recved = -1; // error!
        }
    }
    return recved;
}

/** Private API **/

int ctx_new(m_ctx_t **c) {
    M_DEBUG("Creating context.\n");
    *c = m_mem_new(sizeof(m_ctx_t), ctx_dtor);
    M_ALLOC_ASSERT(*c);

    (*c)->logger = default_logger;
    (*c)->modules = m_map_new(0, mem_dtor);
    (*c)->name = mem_strdup(M_CTX_DEFAULT);

    M_ALLOC_ASSERT((*c)->modules);

    int ret = poll_create(&(*c)->ppriv);
    if (ret != 0) {
        m_mem_unref(*c);
        *c = NULL;
    }
    return ret;
}

void inline ctx_logger(const m_ctx_t *c, const m_mod_t *mod, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    c->logger(mod, fmt, args);
    va_end(args);
}

/** Public API **/

_public_ int m_ctx_set_logger(m_log_cb logger) {
    M_PARAM_ASSERT(logger);
    
    ctx->logger = logger;
    return 0;
}

_public_ int m_ctx_loop_events(int max_events) {
    M_PARAM_ASSERT(max_events > 0);
    M_ASSERT(!ctx->looping, "Context already looping.", -EINVAL);

    int ret = loop_start(ctx, max_events);
    if (ret == 0) {
        while (!ctx->quit && m_map_length(ctx->modules) > 0) {
            recv_events(ctx, -1);
        }
        return loop_stop(ctx);
    }
    return ret;
}

_public_ int m_ctx_loop(void) {
    return m_ctx_loop_events(M_CTX_DEFAULT_EVENTS);
}

_public_ int m_ctx_quit(uint8_t quit_code) {
    M_ASSERT(ctx->looping, "Context not looping.", -EINVAL);
       
    return loop_quit(ctx, quit_code);
}

_public_ int m_ctx_fd(void) {
    return dup(poll_get_fd(&ctx->ppriv));
}

_public_ int m_ctx_dispatch(void) {
    if (!ctx->looping) {
        /* Ok, start now */
        return loop_start(ctx, M_CTX_DEFAULT_EVENTS);
    }
    
    if (ctx->quit || m_map_length(ctx->modules) == 0) {
        /* We are stopping! */
        return loop_stop(ctx);
    }
    
    /* Recv new events, no timeout */
    return recv_events(ctx, 0);
}

_public_ int m_ctx_dump(void) {
    ctx_logger(ctx, NULL, "{\n");
    
    ctx_logger(ctx, NULL, "\t\"Name\": \"%s\",\n", ctx->name);
#ifdef WITH_FS
    ctx_logger(ctx, NULL, "\t\t\"Fs_root\": \"%s\",\n", ctx->fs_root);
#endif
    ctx_logger(ctx, NULL, "\t\"State\": {\n");
    ctx_logger(ctx, NULL, "\t\t\"Quit\": %d,\n", ctx->quit);
    ctx_logger(ctx, NULL, "\t\t\"Looping\": %d,\n", ctx->looping);
    ctx_logger(ctx, NULL, "\t\t\"Max_events\": %d\n", ctx->ppriv.max_events);
    ctx_logger(ctx, NULL, "\t},\n");
    
    ctx_logger(ctx, NULL, "\t\"Modules\": [\n");
    int i = 0;
    m_itr_foreach(ctx->modules, {
        const char *mod_name = m_map_itr_get_key(itr);
        const m_mod_t *mod = m_itr_get(itr);
        ctx_logger(ctx, NULL, "\t\t\"%s\": %p%c\n", mod_name, mod, ++i < m_map_length(ctx->modules) ? ',' : ' ');
    });
    ctx_logger(ctx, NULL, "\t]\n");
    ctx_logger(ctx, NULL, "}\n");
    return 0;
}

_public_ const char *m_ctx_get_name(void) {
    return ctx->name;
}

_public_ int m_ctx_set_name(const char *name) {
    M_PARAM_ASSERT(name && strlen(name));
    M_PARAM_ASSERT(!ctx->looping);

    if (ctx->name) {
        memhook._free(ctx->name);
    }
    ctx->name = mem_strdup(name);
    return 0;
}