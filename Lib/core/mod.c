#include "ps.h"
#include "src.h"
#include "fs.h"
#include "ctx.h"
#include "poll.h"

/***************************************
 * Code related to generic module API. *
 ***************************************/

enum mod_hook { MOD_EVAL, MOD_START, MOD_STOP };

static void module_dtor(void *data);
static int _pipe(m_mod_t *mod);
static int init_pubsub_fd(m_mod_t *mod);
static int manage_srcs(m_mod_t *mod, m_ctx_t *c, int flag, bool stop);
static void reset_module(m_mod_t *mod);
static int optional_hook(m_mod_t *mod, enum mod_hook req_hook);

static void module_dtor(void *data) {
    m_mod_t *mod = (m_mod_t *)data;
    M_MOD_CTX(mod);
    if (mod) {
        m_mem_unref(mod->ctx);
        
        if (mod->plugin_path) {
            m_map_remove(c->plugins, mod->plugin_path);
        }
        
        m_map_free(&mod->subscriptions);
        m_stack_free(&mod->recvs);
        m_queue_free(&mod->stashed);
        m_queue_free(&mod->batch.events);
        for (int i = 0; i < M_SRC_TYPE_END; i++) {
            m_bst_free(&mod->srcs[i]);
        }
        
        if (mod->flags & M_MOD_NAME_AUTOFREE) {
            memhook._free((void *)mod->name);
        }
        
        if (mod->flags & M_MOD_USERDATA_AUTOFREE) {
            memhook._free((void *)mod->userdata);
        }
    }
}

static int _pipe(m_mod_t *mod) {
    int ret = pipe(mod->pubsub_fd);
    if (ret == 0) {
        for (int i = 0; i < 2; i++) {
            int flags = fcntl(mod->pubsub_fd[i], F_GETFL, 0);
            if (flags == -1) {
                flags = 0;
            }
            fcntl(mod->pubsub_fd[i], F_SETFL, flags | O_NONBLOCK);
            fcntl(mod->pubsub_fd[i], F_SETFD, FD_CLOEXEC);
        }
    }
    return ret;
}

static int init_pubsub_fd(m_mod_t *mod) {
    if (_pipe(mod) == 0) {
        fd_src_t fd_src = {0};
        fd_src.fd = mod->pubsub_fd[0];
        if (register_src(mod, M_SRC_TYPE_PS, &fd_src, M_SRC_FD_AUTOCLOSE | M_SRC_PRIO_HIGH, NULL) == 0) {
            return 0;
        }
        close(mod->pubsub_fd[0]);
        close(mod->pubsub_fd[1]);
        mod->pubsub_fd[0] = -1;
        mod->pubsub_fd[1] = -1;
    }
    return -errno;
}

static int manage_srcs(m_mod_t *mod, m_ctx_t *c, int flag, bool stop) {
    int ret = 0;
    
    fetch_ms(&mod->stats.last_seen, &mod->stats.action_ctr);
    for (int i = 0; i < M_SRC_TYPE_END; i++) {
        m_itr_foreach(mod->srcs[i], {
            ev_src_t *t = m_itr_get(m_itr);
            if (flag == RM && stop) {
                if (t->type == M_SRC_TYPE_PS) {
                    /*
                    * Free all unread pubsub msg for this module.
                    */
                    flush_pubsub_msgs(NULL, NULL, mod);
                }
                ret = m_itr_rm(m_itr);
            } else {
                ret = poll_set_new_evt(&c->ppriv, t, flag);
                
                /* For type task: create task thread now */
                if (ret == 0 && t->type == M_SRC_TYPE_TASK && flag == ADD) {
                    ret = start_task(c, t);
                }
            }
        });
    }
    return ret;
}

static void reset_module(m_mod_t *mod) {
    if (mod->pubsub_fd[1] != -1) {
        close(mod->pubsub_fd[1]);
        mod->pubsub_fd[0] = -1;
        mod->pubsub_fd[1] = -1;
    }
    m_map_clear(mod->subscriptions);
    m_stack_clear(mod->recvs);
    m_queue_clear(mod->stashed);
    m_queue_clear(mod->batch.events);
    mod->batch.len = 0;
    memset(&mod->batch.timer, 0, sizeof(m_src_tmr_t));
}

static int optional_hook(m_mod_t *mod, enum mod_hook req_hook) {
    bool bool_ret = true;
    int ret;

    M_MEM_LOCK(mod, {
        switch (req_hook) {
        case MOD_START:
            if (mod->hook.on_start) {
                bool_ret = mod->hook.on_start(mod);
            }
            break;
        case MOD_STOP:
            if (mod->hook.on_stop) {
                mod->hook.on_stop(mod);
            }
            break;
        case MOD_EVAL:
            if (mod->hook.on_eval) {
                bool_ret = mod->hook.on_eval(mod);
            }
            break;
        default:
            break;
        }
        
        ret = bool_ret ? 0 : -1;
        if (m_mod_is(mod, M_MOD_ZOMBIE)) {
            // m_mod_deregister was called
            ret = -ENOENT;
        }
    });
    return ret;
}

/** Private API **/

int evaluate_module(void *data, const char *key, void *value) {
    m_mod_t *mod = (m_mod_t *)value;

    /* Check thresholds, if any is set and mod is running */
    if (m_mod_is(mod, M_MOD_RUNNING)) {
        uint64_t curr_ms;
        fetch_ms(&curr_ms, NULL);

        m_itr_foreach(mod->srcs[M_SRC_TYPE_THRESH], {
            ev_src_t *src = (ev_src_t *)m_itr_get(m_itr);
            m_src_thresh_t *thr = &src->thresh_src.thr;
            m_src_thresh_t *alarm = &src->thresh_src.alarm;

            if (thr->activity_freq != 0) {
                uint64_t active_time = curr_ms - mod->stats.registration_time;
                double activity_freq = (double) mod->stats.action_ctr / active_time;
                if (activity_freq >= thr->activity_freq) {
                    alarm->activity_freq = activity_freq;
                }
            }

            if (thr->inactive_ms != 0) {
                uint64_t inactive_ms = curr_ms - mod->stats.last_seen;
                if (inactive_ms >= thr->inactive_ms) {
                    alarm->inactive_ms = inactive_ms;
                }
            }
            if (alarm->activity_freq != 0 || alarm->inactive_ms != 0) {
                poll_notify_userevent(&mod->ctx->ppriv, src);
            }
        })
    }

    /* Check if module should be started */
    int ret = 0;
    if (m_mod_is(mod, M_MOD_IDLE)) {
        ret = optional_hook(mod, MOD_EVAL);
        if (ret == 0) {
            start(mod, true);
        }
    }
    return ret;
}

int start(m_mod_t *mod, bool starting) {
    static const char *errors[] = { "Failed to resume module.", "Failed to start module." };

    /* 
     * Starting module for the first time
     * or after it was stopped.
     * Properly add back its pubsub fds.
     */
    if (starting) {
        /* THIS IS NOT A RESUME */
        int ret = init_pubsub_fd(mod);
        if (ret != 0) {
            return ret;
        }
    }

    M_MOD_CTX(mod);
    int ret = manage_srcs(mod, c, ADD, false);
    M_ASSERT(!ret, errors[starting], ret);
    
    mod->state = M_MOD_RUNNING;
    c->stats.running_modules++;

    /* Call module on_start() callback only if module is being (re)started */
    if (starting) {
        ret = optional_hook(mod, MOD_START);
    }
    
    switch (ret) {
    case 0:
        M_DEBUG("%s '%s'.\n", starting ? "Started" : "Resumed", mod->name);
        tell_system_pubsub_msg(NULL, c, mod, M_PS_MOD_STARTED);
        break;
    case -1:
        /* on_start() hook returned false, we need to stop this module right away */
        stop(mod, true);
        ret = 0;
        break;
    case -ENOENT:
        // module was deregistered in on_start() hook
    default:
        break;
    }
    return ret;
}

int stop(m_mod_t *mod, bool stopping) {
    static const char *errors[] = { "Failed to pause module.", "Failed to stop module." };
    M_MOD_CTX(mod);

    int ret = manage_srcs(mod, c, RM, stopping);
    M_ASSERT(!ret, errors[stopping], ret);

    if (m_mod_is(mod, M_MOD_RUNNING)) {
        c->stats.running_modules--;
    }
    mod->state = stopping ? M_MOD_STOPPED : M_MOD_PAUSED;

    /*
     * When module gets stopped, its write-end pubsub fd is closed too 
     * Read-end is already closed by manage_fds().
     * Moreover, its subscriptions are cleared.
     * 
     * Finally, on_stop() callback is called.
     */
    if (stopping) {
        reset_module(mod);
        ret = optional_hook(mod, MOD_STOP);
    }
    
    switch (ret) {
    case -ENOENT:
        // module was deregistered in on_stop() hook
        break;
    default:
        M_DEBUG("%s '%s'.\n", stopping ? "Stopped" : "Paused", mod->name);
        tell_system_pubsub_msg(NULL, c, mod, M_PS_MOD_STOPPED);
        ret = 0;
        break;
    }
    return ret;
}

int mod_deregister(m_mod_t **mod, bool from_user) {
    M_PARAM_ASSERT(mod);
    M_MOD_ASSERT((*mod));
    
    m_mod_t *m = *mod;
    M_MOD_CTX(m);
    
    if ((m->flags & M_MOD_PERSIST) && c->state == M_CTX_LOOPING) {
        return -EPERM;
    }
    
    M_DEBUG("Deregistering module '%s'.\n", m->name);
    
    int ret = 0;
    M_MEM_LOCK(m, {
        /* Remove the module from the context */
        ret = m_map_remove(c->modules, m->name);
        
        if (ret == 0) {
            /* Stop module */
            stop(m, true);
            m->state = M_MOD_ZOMBIE;
            
            /* Free FS internal data */
            fs_cleanup(m);
            
            if (from_user) {
                /* Ok; now unref the user reference */
                m_mem_unrefp((void **)mod);
            }
            
            /*
             * Destroy context if it is not looping and
             * it has no more modules in it and is not a persistent ctx
             */
            if (c->state == M_CTX_IDLE && m_map_len(c->modules) == 0 && !(c->flags & M_CTX_PERSIST)) {
                ret = m_ctx_deregister(&c);
            }
        }
    });
    return ret;
}

/** Public API **/

_public_ int m_mod_register(const char *name, m_ctx_t *c, m_mod_t **mod_ref, const m_mod_hook_t *hook,
                            m_mod_flags flags, const void *userdata) {
    M_PARAM_ASSERT(str_not_empty(name));
    M_PARAM_ASSERT(hook);
    /* Mandatory callback */
    M_PARAM_ASSERT(hook->on_evt);
    
    /* NULL ctx means using default ctx */
    if (!c) {
        c = default_ctx;
        if (!c) {
            ctx_new(M_CTX_DEFAULT, &c, 0, 0, NULL);
        }
    }
    M_ALLOC_ASSERT(c);
    
    if (c->finalized) {
        return -EPERM;
    }

    int ret;
    m_mod_t *old_mod = m_map_get(c->modules, name);
    if (old_mod) {
        if (!(old_mod->flags & M_MOD_ALLOW_REPLACE)) {
            M_DEBUG("Module with same name already registered in context.");
            return -EEXIST;
        }
        ret = mod_deregister(&old_mod, false);
        if (ret != 0) {
            return ret;
        }
    }

    M_DEBUG("Registering module '%s'.\n", name);
    
    m_mod_t *mod = m_mem_new(sizeof(m_mod_t), module_dtor);
    M_ALLOC_ASSERT(mod);

    mod->ctx = m_mem_ref(c);
    
    mod->flags = flags | c->mod_flags;
    if (flags & M_MOD_NAME_DUP) {
        mod->flags |= M_MOD_NAME_AUTOFREE;
    }
    mod->userdata = userdata;
    
    ret = -ENOMEM;
    /* Let us gladly jump out with break on error */
    do {
        mod->name = flags & M_MOD_NAME_DUP ? mem_strdup(name) : name;
        
        for (int i = 0; i < M_SRC_TYPE_END; i++) {
            if (init_src(mod, i) != 0) {
                break;
            }
        }
        
        mod->recvs = m_stack_new(NULL);
        if (!mod->recvs) {
            break;
        }

        mod->stashed = m_queue_new(mem_dtor);
        if (!mod->stashed) {
            break;
        }
        
        mod->batch.events = m_queue_new(mem_dtor);
        if (!mod->batch.events) {
            break;
        }
        
        if (m_map_put(c->modules, mod->name, mod) == 0) {
            memcpy(&mod->hook, hook, sizeof(m_mod_hook_t));
            mod->state = M_MOD_IDLE;
            
            if (mod_ref) {
                *mod_ref = m_mem_ref(mod);
            }
            
            mod->pubsub_fd[0] = -1;
            mod->pubsub_fd[1] = -1;
            
            fetch_ms(&mod->stats.registration_time, NULL);
            return 0;
        }
    } while (false);
    
    m_mem_unref(mod);
    return ret;
}

_public_ int m_mod_deregister(m_mod_t **mod) {
    return mod_deregister(mod, true);
}

_public_ __attribute__((format (printf, 2, 3))) int m_mod_log(const m_mod_t *mod, const char *fmt, ...) {
    M_MOD_ASSERT(mod);
    M_MOD_CTX(mod);

    va_list args;
    va_start(args, fmt);
    c->logger(mod, fmt, args);
    va_end(args);
    return 0;
}

_public_ const void *m_mod_userdata(const m_mod_t *mod) {
    M_RET_ASSERT(mod, NULL);
    M_RET_ASSERT(!m_mod_is(mod, M_MOD_ZOMBIE), NULL);
    
    return mod->userdata;
}

_public_ const char *m_mod_name(const m_mod_t *mod_self) {
    M_RET_ASSERT(mod_self, NULL);

    return mod_self->name;
}

/** Module state getters **/

_public_ bool m_mod_is(const m_mod_t *mod, m_mod_states st) {
    M_RET_ASSERT(mod, false);

    return mod->state & st;
}

_public_ m_mod_states m_mod_state(const m_mod_t *mod) {
    M_PARAM_ASSERT(mod);

    return mod->state;
}

_public_ int m_mod_dump(const m_mod_t *mod) {
    M_MOD_ASSERT(mod);
    M_MOD_CTX(mod);

    ctx_logger(c, mod, "{\n");
    ctx_logger(c, mod, "\t\"Name\": \"'%s\",\n", mod->name);
    ctx_logger(c, mod, "\t\"State\": \"%#x\",\n", mod->state);
    ctx_logger(c, mod, "\t\"Flags\": \"%#x\",\n", mod->flags);
    ctx_logger(c, mod, "\t\"UP\": \"%p\",\n", mod->userdata);

    ctx_logger(c, mod, "\t\"Stats\": {\n");
    ctx_logger(c, mod, "\t\t\"Reg_time\": %" PRIu64 ",\n", mod->stats.registration_time);
    ctx_logger(c, mod, "\t\t\"Num_srcs\": %lu,\n", m_mod_src_len(mod, M_SRC_TYPE_END));
    ctx_logger(c, mod, "\t\t\"Sent_msgs\": %" PRIu64 ",\n", mod->stats.sent_msgs);
    ctx_logger(c, mod, "\t\t\"Recv_msgs\": %" PRIu64 ",\n", mod->stats.recv_msgs);
    ctx_logger(c, mod, "\t\t\"Last_seen\": %" PRIu64 ",\n", mod->stats.last_seen);
    ctx_logger(c, mod, "\t\t\"Num_actions\": %" PRIu64 ",\n", mod->stats.action_ctr);

    uint64_t curr_ms;
    fetch_ms(&curr_ms, NULL);
    uint64_t active_time = curr_ms - mod->stats.registration_time;
    ctx_logger(c, mod, "\t\t\"Action_freq\": %lf\n", (double)mod->stats.action_ctr / active_time);
    ctx_logger(c, mod, "\t},\n");
    
    ctx_logger(c, mod, "\t\"Subs\": [\n");
    m_itr_foreach(mod->subscriptions, {
        ev_src_t *sub = m_itr_get(m_itr);
        if (sub->flags & M_SRC_INTERNAL) {
            continue;
        }
        ctx_logger(c, mod, "\t{\n");
        ctx_logger(c, mod, "\t\t\"Flags\": \"%#x\"\n", sub->flags);
        ctx_logger(c, mod, "\t\t\"UP\": \"%p\",\n", sub->userptr);
        ctx_logger(c, mod, "\t\t\"Topic\": \"%s\"\n", sub->ps_src.topic);
        ctx_logger(c, mod, "\t}%c\n", m_idx + 1 < m_map_len(mod->subscriptions) ? ',' : ' ');
    });
    ctx_logger(c, mod, "\t],\n");
    
    /* Skip internal fds (M_SRC_TYPE_PS and M_SRC_INTERNAL flag) */
    for (int k = M_SRC_TYPE_FD; k < M_SRC_TYPE_END; k++) {
        ctx_logger(c, mod, "\t\"%s\": [\n", src_names[k]);
        m_itr_foreach(mod->srcs[k], {
            ev_src_t *t = m_itr_get(m_itr);
            if (t->flags & M_SRC_INTERNAL) {
                continue;
            }
            
            ctx_logger(c, mod, "\t{\n");
            ctx_logger(c, mod, "\t\t\"Flags\": \"%#x\",\n", t->flags);
            ctx_logger(c, mod, "\t\t\"UP\": \"%p\",\n", t->userptr);

            switch (t->type) {
            case M_SRC_TYPE_FD:
                ctx_logger(c, mod, "\t\t\"FD\": %d\n", t->fd_src.fd);
                break;
            case M_SRC_TYPE_SGN:
                ctx_logger(c, mod, "\t\t\"SGN\": %d\n", t->sgn_src.sgs.signo);
                break;
            case M_SRC_TYPE_TMR:
                ctx_logger(c, mod, "\t\t\"TMR_MS\": %lu,\n", t->tmr_src.its.ms);
                ctx_logger(c, mod, "\t\t\"TMR_CID\": %d\n", t->tmr_src.its.clock_id);
                break;
            case M_SRC_TYPE_PATH:
                ctx_logger(c, mod, "\t\t\"PATH\": \"%s\",\n", t->path_src.pt.path);
                ctx_logger(c, mod, "\t\t\"EV\": \"%#x\"\n", t->path_src.pt.events);
                break;
            case M_SRC_TYPE_PID:
                ctx_logger(c, mod, "\t\t\"PID\": %d,\n", t->pid_src.pid.pid);
                ctx_logger(c, mod, "\t\t\"EV\": %u\n", t->pid_src.pid.events);
                break;
            case M_SRC_TYPE_TASK:
                ctx_logger(c, mod, "\t\t\"TID\": %d\n", t->task_src.tid.tid);
                break;
            case M_SRC_TYPE_THRESH:
                ctx_logger(c, mod, "\t\t\"INACTIVE_MS\": %d,\n", t->thresh_src.thr.inactive_ms);
                ctx_logger(c, mod, "\t\t\"ACTIVITY_FREQ\": %d\n", t->thresh_src.thr.activity_freq);
                break;
            default:
                break;
            }

            ctx_logger(c, mod, "\t}%c\n", m_idx + 1 < m_bst_len(mod->srcs[k]) ? ',' : ' ');
        });
         ctx_logger(c, mod, "\t]%c\n", (k < M_SRC_TYPE_END - 1) ? ',' : ' ');
    }
    ctx_logger(c, mod, "}\n");
    return 0;
}

_public_ int m_mod_stats(const m_mod_t *mod, m_mod_stats_t *stats) {
    M_MOD_ASSERT(mod);
    M_PARAM_ASSERT(stats);
    
    uint64_t now;
    fetch_ms(&now, NULL);
    uint64_t registered_time = now - mod->stats.registration_time;
    stats->inactive_ms = now - mod->stats.last_seen;
    stats->activity_freq = ((double)mod->stats.action_ctr) / registered_time;
    stats->recv_msgs = mod->stats.recv_msgs;
    stats->sent_msgs = mod->stats.sent_msgs;
    return 0;
}

_public_ m_ctx_t *m_mod_ctx(const m_mod_t *mod) {
    M_RET_ASSERT(mod, NULL);
    M_RET_ASSERT(!m_mod_is(mod, M_MOD_ZOMBIE), NULL);
    M_RET_ASSERT(!(mod->flags & M_MOD_DENY_CTX), NULL);

    return mod->ctx;
}

/** Module state setters **/

_public_ int m_mod_start(m_mod_t *mod) {
    M_MOD_ASSERT_STATE(mod, M_MOD_IDLE | M_MOD_STOPPED);
    
    return start(mod, true);
}

_public_ int m_mod_pause(m_mod_t *mod) {
    M_MOD_ASSERT_STATE(mod, M_MOD_RUNNING);
    
    return stop(mod, false);
}

_public_ int m_mod_resume(m_mod_t *mod) {
    M_MOD_ASSERT_STATE(mod, M_MOD_PAUSED);
    
    return start(mod, false);
}

_public_ int m_mod_stop(m_mod_t *mod) {
    M_MOD_ASSERT_STATE(mod, M_MOD_RUNNING | M_MOD_PAUSED);
    
    return stop(mod, true);
}

_public_ int m_mod_set_batch_size(m_mod_t *mod, size_t len) {
    M_MOD_ASSERT(mod);
    
    mod->batch.len = len;
    return 0;
}

_public_ int m_mod_set_batch_timeout(m_mod_t *mod, uint64_t timeout_ms) {
    M_MOD_ASSERT(mod);

    /* If it was already set, remove old timer */
    if (mod->batch.timer.ms != 0) {
        deregister_src(mod, M_SRC_TYPE_TMR, &mod->batch.timer);
    }
    mod->batch.timer.clock_id = CLOCK_MONOTONIC;
    mod->batch.timer.ms = timeout_ms;
    if (timeout_ms != 0) {
        // If batching by size was disabled
        if (mod->batch.len == 0) {
            // Set a maximum value for batching so that only timed batching will be effective
            mod->batch.len = -1;
        }
        return register_src(mod, M_SRC_TYPE_TMR, &mod->batch.timer, M_SRC_INTERNAL | M_SRC_PRIO_HIGH, NULL);
    }
    return 0;
}
