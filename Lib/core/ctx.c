#include "public/module/thpool/thpool.h"
#include "ps.h"
#include "poll.h"
#include "fs.h"
#include "evts.h"

/**************************************
 * Code related to contexts handling. *
 **************************************/

static pthread_key_t key;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;

static void make_key(void);
static void ctx_dtor(void *data);
static void default_logger(const m_mod_t *mod, const char *fmt, va_list args);
static int loop_start(m_ctx_t *c, int max_events);
static uint8_t loop_stop(m_ctx_t *c);
static inline int loop_quit(m_ctx_t *c, uint8_t quit_code);
static void push_evt(m_mod_t *mod, evt_priv_t *evt);
static int recv_events(m_ctx_t *c, int timeout);
static int m_ctx_loop_events(m_ctx_t *c, int max_events);
static int ctx_destroy_mods(void *data, const char *key, void *value);
static ev_src_t *process_tick(ev_src_t *this, m_ctx_t *c, int idx, evt_priv_t *evt);
static int ctx_new(const char *ctx_name, m_ctx_flags flags, const void *userdata);

static void make_key(void) {
    pthread_key_create(&key, NULL);
}

static void ctx_dtor(void *data) {
    m_ctx_t *context = (m_ctx_t *)data;
    M_DEBUG("Ctx '%s' dtor.\n", context->name);

    deregister_ctx_src(context, &context->tick.src);
    m_map_free(&context->modules);
    poll_destroy(&context->ppriv);
    memhook._free(context->ppriv.data);
    fs_destroy(context);

    if (context->flags & M_CTX_NAME_AUTOFREE) {
        memhook._free((void *)context->name);
    }

    if (context->flags & M_CTX_USERDATA_AUTOFREE) {
        memhook._free((void *)context->userdata);
    }
}

static void default_logger(const m_mod_t *mod, const char *fmt, va_list args) {
    if (mod) {
        printf("[%s]|%s|: ", mod->ctx->name, mod->name);
    }
    vprintf(fmt, args);
}

static int loop_start(m_ctx_t *c, int max_events) {
    c->ppriv.max_events = max_events;
    int ret = poll_init(&c->ppriv);
    if (ret == 0) {
        /* Start fuse fs if requested */
        int fs_ret = fs_start(c);
        if (fs_ret != 0) {
            M_WARN("Failed to initialize fuse fs: %s\n", strerror(-fs_ret));
        }

        fetch_ms(&c->stats.looping_start_time, NULL);
        c->state = M_CTX_LOOPING;
        c->quit = false;
        c->quit_code = 0;
        
        /* Eventually start any IDLE module */
        m_iterate(c->modules, evaluate_module, NULL);

        /* Publish loop started system message */
        tell_system_pubsub_msg(NULL, c, NULL, M_PS_CTX_STARTED);
        
        /* Start the tick source right now! */
        if (c->tick.src) {
            poll_set_new_evt(&c->ppriv, c->tick.src, ADD);
        }
    }
    return ret;
}

static uint8_t loop_stop(m_ctx_t *c) {
    c->state = M_CTX_IDLE;
    
    /* Publish loop stopped system message */
    tell_system_pubsub_msg(NULL, c, NULL, M_PS_CTX_STOPPED);
    
    /* Flush pubsub msg to avoid memleaks */
    m_iterate(c->modules, flush_pubsub_msgs, NULL);
    
    /* Stop FS */
    fs_stop(c);

    /* Stop the tick source right now! */
    if (c->tick.src) {
        poll_set_new_evt(&c->ppriv, c->tick.src, RM);
    }
    
    poll_clear(&c->ppriv);

    /* Destroy thpool eventually waiting on currently running tasks */
    m_thpool_free(&c->thpool, false);

    c->ppriv.max_events = 0;
    c->stats.looping_start_time = 0;
    c->stats.recv_msgs = 0;
    c->stats.idle_time = 0;

    int ret = c->quit_code;
    
    /*
     * ctx cannot be deregistered while looping,
     * thus even if all modules were deregistered during the loop,
     * and last module's tried to call m_ctx_deregister(), it returned -EPERM.
     * Gracefully deregister it now.
     */
    if (m_map_len(c->modules) == 0 && !(c->flags & M_CTX_PERSIST)) {
        m_ctx_deregister();
    }
    return ret;
}

static int loop_quit(m_ctx_t *c, uint8_t quit_code) {
    c->quit = true;
    c->quit_code = quit_code;
    return 0;
}

/*
 * We will call current on_evt callback when either:
 * * a HIGH priority message is received
 * * a NORM priority message is received and number of batched events reached number of requested batch size
 * * an internal (timer) message is received, that means that batch timer is elapsed
 */
static void push_evt(m_mod_t *mod, evt_priv_t *evt) {
    ev_src_t *src = evt->src;
    m_evt_t *msg = &evt->evt;
    
    const bool is_internal = src && (src->flags & M_SRC_INTERNAL);
    bool force = false;

    /*
     * If it is an internal event, unref it as
     * it does not need to be recved by user;
     * else, push it onto the message queue.
     */
    if (is_internal) {
        const bool is_batch_timer = src->userptr == &mod->batch;
        const bool is_tb_timer = src->userptr == &mod->tb;
        
        m_mem_unref(evt);
        /* When batch timer elapses, force flush events to user */
        force = is_batch_timer;

        /* If this is the tokenbucket timer, refill one token */
        if (is_tb_timer) {
            if (mod->tb.tokens < mod->tb.burst) {
                mod->tb.tokens++;
            }
        }
    } else {
        m_queue_enqueue(mod->batch.events, evt);
        /*
            * if src == NULL, do not touch msg->userdata as it is already set to correct value:
            * * it will be NULL when called from ctx loop or flush_pubsub_msgs
            * * it will already be valued when called by m_mod_unstash API
            */
        if (src) {
            msg->userdata = src->userptr;
            /* When receiving a high priority message, flush immediately */
            if (src->flags & M_SRC_PRIO_HIGH) {
                force = true;
            } else if (src->flags & M_SRC_PRIO_LOW) {
                /*
                 * Always batch a low priority message with subsequent ones,
                 * even if batching is disabled
                 */
                return;
            }
        }
    }
    
    if (m_queue_len(mod->batch.events) == 0) {
        return;
    }
    
    /*
     * If we reached the batch len, or pubsub_cb was called by 
     * M_SRC_INTERNAL timer (meaning that batching time has elapsed),
     * run the pubsub callback!
     */
    if (force ||
        m_queue_len(mod->batch.events) >= mod->batch.len) {

        /*
         * Avoid the user changing the list of batched events while parsing them,
         * eg: by calling deregister/stop on the module.
         */
        m_queue_t *evts = mod->batch.events;
        mod->batch.events = m_queue_new(mem_dtor);
    
        call_pubsub_cb(mod, evts);
    }
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
        if (p) {
            M_ASSERT(p->process);
            if (!p->mod) {
                // It is a ctx priv event
                p = p->process(p, c, i, NULL);
                recved++;
                continue;
            }

            /*
             * Keep a reference on mod, to avoid that
             * a m_mod_deregister() call by user callback
             * invalidates our pointer.
             */
            m_mod_t *mod = p->mod;
            evt_priv_t *evt = new_evt(p);
            m_evt_t *msg = NULL;
            if (evt) {
                msg = &evt->evt;
                fetch_ms(&msg->ts, NULL);
                M_INFO("'%s' received %u type evt.\n", mod->name, msg->type);
                p = p->process(p, c, i, evt);
            }
            err = errno; // Store any errno that happened while consuming events
            bool msg_consumed = false;

            if (err == 0) {
                /* 
                 * Remove the source if it was a oneshot event.
                 * NOTE: this will reduce refs counter for evt->src to just 1,
                 * ie: it stays alive because it is needed by an evt
                 */
                if (p && p->flags & M_SRC_ONESHOT) {
                    if (p->type != M_SRC_TYPE_PS) {
                        m_bst_remove(mod->srcs[p->type], p);
                    } else {
                        m_map_remove(mod->subscriptions, p->ps_src.topic);
                    }
                }
                
                /*
                 * All messages share same address inside union.
                 * In this case, check that any message was actually received,
                 * and it was from a know source type.
                 */
                if (msg->fd_evt) {
                    recved++;
                    if (msg->type != M_SRC_TYPE_PS || !msg->ps_evt->topic || strcmp(msg->ps_evt->topic, M_PS_MOD_POISONPILL)) {
                        push_evt(mod, evt);
                        msg_consumed = true;
                    } else {
                        M_INFO("PoisonPilling '%s'.\n", mod->name);
                        stop(mod, true);
                    }
                }
            }

            if (!msg_consumed) {
                /*
                 * Unref the evt if it wasn't consumed 
                 * by user callback to avoid memleaks,
                 */
                m_mem_unref(evt);
            }
        } else {
            /* Forward error to below handling code */
            err = EAGAIN;
            M_WARN("Received message without proper source: src -> %p\n", p);
        }
    }

    if (recved > 0 && err == 0) {
        m_iterate(c->modules, evaluate_module, NULL);
        c->stats.recv_msgs += recved;
    } else if (err) {
        /* Quit and return < 0 only for real errors */
        if (err != EINTR && err != EAGAIN) {
            M_ERR("Ctx '%s' loop error: %s.\n", c->name, strerror(err));
            loop_quit(c, err);
            recved = -1; // error!
        }
    }

    fetch_ms(&last_time_called, NULL);
    return recved;
}

static int m_ctx_loop_events(m_ctx_t *c, int max_events) {
    M_PARAM_ASSERT(max_events > 0);
    M_LOG_ASSERT(c->state == M_CTX_IDLE, "Context already looping.", -EINVAL);

    int ret = loop_start(c, max_events);
    if (ret == 0) {
        while (!c->quit && c->stats.running_modules > 0) {
            recv_events(c, -1);
        }
        return loop_stop(c);
    }
    return ret;
}

static int ctx_destroy_mods(void *data, const char *key, void *value) {
    m_mod_t *m = (m_mod_t *)value;
    return mod_deregister(&m, false);
}

static ev_src_t *process_tick(ev_src_t *this, m_ctx_t *c, int idx, evt_priv_t *evt) {
    poll_consume_tmr(&c->ppriv, idx, this, NULL);
    tell_system_pubsub_msg(NULL, c, NULL, M_PS_CTX_TICK);
    return this;
}

static int ctx_new(const char *ctx_name, m_ctx_flags flags, const void *userdata) {
    M_DEBUG("Creating context '%s'.\n", ctx_name);
    
    m_ctx_t *new_ctx = m_mem_new(sizeof(m_ctx_t), ctx_dtor);
    M_ALLOC_ASSERT(new_ctx);

    int ret;
    do {
        ret = poll_create(&new_ctx->ppriv);
        if (ret != 0) {
            break;
        }

        new_ctx->flags = flags;
        new_ctx->userdata = userdata;
        new_ctx->logger = default_logger;
        new_ctx->modules = m_map_new(0, mem_dtor);
        if (!new_ctx->modules) {
            break;
        }

        if (new_ctx->flags & M_CTX_NAME_DUP) {
            new_ctx->flags |= M_CTX_NAME_AUTOFREE;
            new_ctx->name = mem_strdup(ctx_name);
        } else {
            new_ctx->name = ctx_name;
        }
        ret = fs_create(new_ctx);
        if (ret != 0) {
            break;
        }

        ret = pthread_setspecific(key, new_ctx);
    } while (false);
    
    if (ret != 0) {
        m_mem_unref(new_ctx);
    }
    return ret;
}

/** Private API **/

m_ctx_t *m_ctx(void) {
    m_ctx_t *c = pthread_getspecific(key);
    if (c && c->curr_mod) {
        M_RET_ASSERT(!(c->curr_mod->flags & M_MOD_DENY_CTX), NULL);
    }
    return c;
}

void inline ctx_logger(const m_ctx_t *c, const m_mod_t *mod, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    c->logger(mod, fmt, args);
    va_end(args);
}

/** Public API **/

_public_ int m_ctx_register(const char *ctx_name, m_ctx_flags flags, const void *userdata) {
    M_PARAM_ASSERT(str_not_empty(ctx_name));
    
    pthread_once(&key_once, make_key);
    if (pthread_getspecific(key)) {
        return -EEXIST;
    }
    return ctx_new(ctx_name, flags, userdata);
}

_public_ int m_ctx_deregister(void) {
    M_CTX_ASSERT();
    M_PARAM_ASSERT(c->state == M_CTX_IDLE);

    int ret = pthread_setspecific(key, NULL);
    if (ret == 0) {
        m_iterate(c->modules, ctx_destroy_mods, NULL);
        m_mem_unref(c);
    }
    return ret;
}

_public_ int m_ctx_set_logger(m_log_cb logger) {
    M_CTX_ASSERT();
    M_PARAM_ASSERT(logger);
    
    c->logger = logger;
    return 0;
}

_public_ int m_ctx_loop(void) {
    M_CTX_ASSERT();
    return m_ctx_loop_events(c, M_CTX_DEFAULT_EVENTS);
}

_public_ int m_ctx_quit(uint8_t quit_code) {
    M_CTX_ASSERT();
    M_LOG_ASSERT(c->state == M_CTX_LOOPING, "Context not looping.", -EINVAL);
       
    return loop_quit(c, quit_code);
}

_public_ int m_ctx_fd(void) {
    M_CTX_ASSERT();

    return dup(poll_get_fd(&c->ppriv));
}

_public_ int m_ctx_dispatch(void) {
    M_CTX_ASSERT();

    if (c->state == M_CTX_IDLE) {
        /* Ok, start now */
        return loop_start(c, M_CTX_DEFAULT_EVENTS);
    }
    
    if (c->quit || c->stats.running_modules == 0) {
        /* We are stopping! */
        return loop_stop(c);
    }

    /* Recv new events, no timeout */
    return recv_events(c, 0);
}

_public_ int m_ctx_dump(void) {
    M_CTX_ASSERT();

    ctx_logger(c, NULL, "{\n");
    ctx_logger(c, NULL, "\t\"Name\": \"%s\",\n", c->name);
    ctx_logger(c, NULL, "\t\"Flags\": \"%#x\",\n", c->flags);
    ctx_logger(c, NULL, "\t\"UP\": \"%p\",\n", c->userdata);
    ctx_logger(c, NULL, "\t\"Tick\": \"%" PRIu64 ",\n", c->tick.tmr.ns);
    ctx_logger(c, NULL, "\t\"State\": {\n");
    ctx_logger(c, NULL, "\t\t\"Quit\": %d,\n", c->quit);
    ctx_logger(c, NULL, "\t\t\"Looping\": %d\n", c->state == M_CTX_LOOPING);
    ctx_logger(c, NULL, "\t},\n");

    uint64_t now;
    fetch_ms(&now, NULL);
    uint64_t total_looping_time = now - c->stats.looping_start_time;
    uint64_t total_busy_time = total_looping_time - c->stats.idle_time;
    ctx_logger(c, NULL, "\t\"Stats\": {\n");
    ctx_logger(c, NULL, "\t\t\"Looping_since\": %" PRIu64 ",\n", c->stats.looping_start_time);
    ctx_logger(c, NULL, "\t\t\"Looping_time\": %" PRIu64 ",\n", total_looping_time);
    ctx_logger(c, NULL, "\t\t\"Idle_time\": %" PRIu64 ",\n", c->stats.idle_time);
    ctx_logger(c, NULL, "\t\t\"Busy_time\": %" PRIu64 ",\n", total_busy_time);
    ctx_logger(c, NULL, "\t\t\"Recv_events\": %" PRIu64 ",\n", c->stats.recv_msgs);
    ctx_logger(c, NULL, "\t\t\"Action_freq\": %lf,\n", (double)c->stats.recv_msgs / total_looping_time);
    ctx_logger(c, NULL, "\t\t\"Modules\": %lu,\n", m_ctx_len());
    ctx_logger(c, NULL, "\t\t\"Running_modules\": %lu\n", c->stats.running_modules);
    ctx_logger(c, NULL, "\t},\n");

    ctx_logger(c, NULL, "\t\"Modules\": [\n");
    m_itr_foreach(c->modules, {
        const m_mod_t *mod = m_itr_get(m_itr);
        mod_dump(mod, false, "\t");
    });
    ctx_logger(c, NULL, "\t]\n");
    ctx_logger(c, NULL, "}\n");
    return 0;
}

_public_ int m_ctx_stats(OUT m_ctx_stats_t *stats) {
    M_CTX_ASSERT();
    M_PARAM_ASSERT(c->state == M_CTX_LOOPING);
    M_PARAM_ASSERT(stats);

    uint64_t now;
    fetch_ms(&now, NULL);

    stats->recv_msgs = c->stats.recv_msgs;
    stats->num_modules = m_ctx_len();
    stats->running_modules = c->stats.running_modules;
    stats->total_idle_time = c->stats.idle_time;
    stats->total_looping_time = now - c->stats.looping_start_time;

    uint64_t active_time = stats->total_looping_time - stats->total_idle_time;
    stats->activity_freq = (double)active_time / stats->total_looping_time;
    return 0;
}

_public_ const char *m_ctx_name(void) {
    M_CTX();
    M_RET_ASSERT(c, NULL);

    return c->name;
}

_public_ const void *m_ctx_userdata(void) {
    M_CTX();
    M_RET_ASSERT(c, NULL);

    return c->userdata;
}

_public_ ssize_t m_ctx_len(void) {
    M_CTX_ASSERT();
    
    return m_map_len(c->modules);
}

_public_ int m_ctx_finalize(void) {
    M_CTX_ASSERT();
    
    c->finalized = true;
    return 0;
}

_public_ int m_ctx_set_tick(uint64_t ns) {
    M_CTX_ASSERT();

    deregister_ctx_src(c, &c->tick.src);
    if (ns != 0) {
        c->tick.tmr.clock_id = CLOCK_MONOTONIC;
        c->tick.tmr.ns = ns;
        c->tick.src = register_ctx_src(c, M_SRC_TYPE_TMR, process_tick, &c->tick);
    }
    return 0;
}
