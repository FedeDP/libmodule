#include "mod.h"
#include "ctx.h"
#include "fs_priv.h"

#define M_CTX_DEFAULT  "libmodule"

static void ctx_dtor(void *data);
static void default_logger(const m_mod_t *mod, const char *fmt, va_list args);
static int loop_start(m_ctx_t *c, int max_events);
static uint8_t loop_stop(m_ctx_t *c);
static inline int loop_quit(m_ctx_t *c, uint8_t quit_code);
static int recv_events(m_ctx_t *c, int timeout);
static int m_ctx_loop_events(int max_events);

static void ctx_dtor(void *data) {
    M_DEBUG("Destroying context.\n");
    m_ctx_t *context = (m_ctx_t *)data;
    m_map_free(&context->modules);
    poll_destroy(&context->ppriv);
    memhook._free(context->ppriv.data);
    memhook._free(context->fs_root);
    memhook._free(context->name);
}

static void default_logger(const m_mod_t *mod, const char *fmt, va_list args) {
    if (mod) {
        printf("[%s]|%s|: ", ctx->name, mod->name);
    }
    vprintf(fmt, args);
}

static int loop_start(m_ctx_t *c, int max_events) {
    c->ppriv.max_events = max_events;
    int ret = poll_init(&c->ppriv);
    if (ret == 0) {
        /* Initialize fuse fs if requested */
        if (c->fs_root && strlen(c->fs_root)) {
            int fs_ret = fs_init(c);
            if (fs_ret != 0) {
                M_WARN("Failed to initialize fuse fs: %s\n", strerror(-fs_ret));
            }
        }

        fetch_ms(&c->stats.looping_start_time, NULL);
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
    c->stats.looping_start_time = 0;
    c->stats.recv_msgs = 0;
    c->stats.idle_time = 0;

    return c->quit_code;
}

static inline int loop_quit(m_ctx_t *c, uint8_t quit_code) {
    c->quit = true;
    c->quit_code = quit_code;
    return 0;
}

static int recv_events(m_ctx_t *c, int timeout) {
    static uint64_t last_time_called;

    if (c->stats.recv_msgs == 0) {
        // First time entering: (re)start counter
        fetch_ms(&last_time_called, NULL);
    }

    int err;
    int recved = 0;

    errno = 0;
    const int nfds = poll_wait(&c->ppriv, timeout);
    err = errno; // store any errno happened in poll_wait

    // Store idling time stat
    uint64_t now;
    fetch_ms(&now, NULL);
    c->stats.idle_time += now - last_time_called;

    for (int i = 0; i < nfds && !err; i++) {
        ev_src_t *p = poll_recv(&c->ppriv, i);
        if (p && p->type == M_SRC_TYPE_FD && p->mod == NULL) {
            /* Received from fuse */
            fs_process(c);
            recved++;
            continue;
        }
        
        if (p && p->mod) {
            m_mod_t *mod = p->mod;
            m_evt_t *msg = new_evt(p->type);
            if (msg) {
                M_DEBUG("'%s' received %u type msg.\n", mod->name, msg->type);
                switch (msg->type) {
                case M_SRC_TYPE_FD:
                    /* Received from FD */
                    msg->fd_evt = m_mem_new(sizeof(*msg->fd_evt), NULL);
                    msg->fd_evt->fd = p->fd_src.fd;
                    break;
                case M_SRC_TYPE_PS: {
                    ps_priv_t *ps_msg;
                    /* Received on pubsub interface */
                    if (read(p->fd_src.fd, (void **)&ps_msg, sizeof(ps_priv_t *)) != sizeof(ps_priv_t *)) {
                        M_DEBUG("Failed to read message: %s\n", strerror(errno));
                    } else {
                        msg->ps_evt = &ps_msg->msg;
                        p = ps_msg->sub;            // Use real event source, ie: topic subscription if any
                    }
                    break;
                }
                case M_SRC_TYPE_SGN:
                    msg->sgn_evt = m_mem_new(sizeof(*msg->sgn_evt), NULL);
                    if (poll_consume_sgn(&c->ppriv, i, p, msg->sgn_evt) == 0) {
                        msg->sgn_evt->signo = p->sgn_src.sgs.signo;
                    }
                    break;
                case M_SRC_TYPE_TMR:
                    msg->tmr_evt = m_mem_new(sizeof(*msg->sgn_evt), NULL);
                    if (poll_consume_tmr(&c->ppriv, i, p,  msg->tmr_evt) == 0) {
                        msg->tmr_evt->ms = p->tmr_src.its.ms;
                    }
                    break;
                case M_SRC_TYPE_PATH:
                    msg->path_evt = m_mem_new(sizeof(*msg->path_evt), NULL);
                    if (poll_consume_pt(&c->ppriv, i, p, msg->path_evt) == 0) {
                        msg->path_evt->path = p->path_src.pt.path;
                    }
                    break;
                case M_SRC_TYPE_PID:
                    msg->pid_evt = m_mem_new(sizeof(*msg->pid_evt), NULL);
                    if (poll_consume_pid(&c->ppriv, i, p, msg->pid_evt) == 0) {
                        msg->pid_evt->pid = p->pid_src.pid.pid;
                    }
                    break;
                case M_SRC_TYPE_TASK:
                    msg->task_evt = m_mem_new(sizeof(*msg->task_evt), NULL);
                    if (poll_consume_task(&c->ppriv, i, p, msg->task_evt) == 0) {
                        msg->task_evt->tid = p->task_src.tid.tid;
                        pthread_join(p->task_src.th, NULL);
                    }
                    break;
                case M_SRC_TYPE_THRESH:
                    msg->thresh_evt = m_mem_new(sizeof(*msg->thresh_evt), NULL);
                    if (poll_consume_thresh(&c->ppriv, i, p, msg->thresh_evt) == 0) {
                        msg->thresh_evt->id = p->thresh_src.thr.id;
                        msg->thresh_evt->inactive_ms = p->thresh_src.alarm.inactive_ms;
                        msg->thresh_evt->activity_freq = p->thresh_src.alarm.activity_freq;
                    }
                    break;
                default:
                    M_DEBUG("Unmanaged src %d.\n", msg->type);
                    break;
                }
            }
            err = errno; // Store any errno happend while consuming events

            if (err == 0) {
                recved++;
                if (msg->type != M_SRC_TYPE_PS || msg->ps_evt->type != M_PS_MOD_POISONPILL) {
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
            err = EAGAIN;
        }
    }

    if (recved > 0 && err == 0) {
        m_map_iterate(c->modules, evaluate_module, NULL);
        ctx->stats.recv_msgs += recved;
    } else if (err) {
        /* Quit and return < 0 only for real errors */
        if (err != EINTR && err != EAGAIN) {
            fprintf(stderr, "Ctx '%s' loop error: %s.\n", c->name, strerror(err));
            loop_quit(c, err);
            recved = -1; // error!
        }
    }

    fetch_ms(&last_time_called, NULL);
    return recved;
}

static int m_ctx_loop_events(int max_events) {
    M_PARAM_ASSERT(max_events > 0);
    M_ASSERT(!ctx->looping, "Context already looping.", -EINVAL);

    int ret = loop_start(ctx, max_events);
    if (ret == 0) {
        while (!ctx->quit && ctx->stats.running_modules > 0) {
            recv_events(ctx, -1);
        }
        return loop_stop(ctx);
    }
    return ret;
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
    
    if (ctx->quit || ctx->stats.running_modules == 0) {
        /* We are stopping! */
        return loop_stop(ctx);
    }

    /* Recv new events, no timeout */
    return recv_events(ctx, 0);
}

_public_ int m_ctx_dump(void) {
    ctx_logger(ctx, NULL, "{\n");
    
    ctx_logger(ctx, NULL, "\t\"Name\": \"%s\",\n", ctx->name);
    if (ctx->fs_root && strlen(ctx->fs_root)) {
        ctx_logger(ctx, NULL, "\t\t\"Fs_root\": \"%s\",\n", ctx->fs_root);
    }
    ctx_logger(ctx, NULL, "\t\"State\": {\n");
    ctx_logger(ctx, NULL, "\t\t\"Quit\": %d,\n", ctx->quit);
    ctx_logger(ctx, NULL, "\t\t\"Looping\": %d,\n", ctx->looping);
    ctx_logger(ctx, NULL, "\t},\n");

    uint64_t now;
    fetch_ms(&now, NULL);
    uint64_t total_looping_time = now - ctx->stats.looping_start_time;
    uint64_t total_busy_time = total_looping_time - ctx->stats.idle_time;
    ctx_logger(ctx, NULL, "\t\"Stats\": {\n");
    ctx_logger(ctx, NULL, "\t\t\"Looping_since\": %" PRIu64 ",\n", ctx->stats.looping_start_time);
    ctx_logger(ctx, NULL, "\t\t\"Total_looping_time\": %" PRIu64 ",\n", total_looping_time);
    ctx_logger(ctx, NULL, "\t\t\"Total_idle_time\": %" PRIu64 ",\n", ctx->stats.idle_time);
    ctx_logger(ctx, NULL, "\t\t\"Total_busy_time\": %" PRIu64 ",\n", total_busy_time);
    ctx_logger(ctx, NULL, "\t\t\"Recv_events\": %" PRIu64 ",\n", ctx->stats.recv_msgs);
    ctx_logger(ctx, NULL, "\t\t\"Action_freq\": %Lf\n", (long double)ctx->stats.recv_msgs / total_looping_time);
    ctx_logger(ctx, NULL, "\t\t\"Running_modules\": %" PRIu64 ",\n", ctx->stats.running_modules);
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

_public_ _pure_ int m_ctx_stats(m_ctx_stats_t *stats) {
    M_PARAM_ASSERT(ctx->looping);
    M_PARAM_ASSERT(stats);

    uint64_t now;
    fetch_ms(&now, NULL);

    stats->recv_msgs = ctx->stats.recv_msgs;
    stats->num_modules = m_map_length(ctx->modules);
    stats->running_modules = ctx->stats.running_modules;
    stats->total_idle_time = ctx->stats.idle_time;
    stats->total_looping_time = now - ctx->stats.looping_start_time;

    uint64_t active_time = stats->total_looping_time - stats->total_idle_time;
    stats->activity_freq = (double)active_time / stats->total_looping_time;
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
