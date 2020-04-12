#include "module.h"
#include "context.h"
#include "fs_priv.h"
#include "mem.h"
#include <dlfcn.h> // dlopen

static void *thread_loop(void *param);
static int main_loop(void *data, const char *key, void *value);
static _ctor1_ void libmodule_init(void);
static _dtor0_ void libmodule_destroy(void);
static int ctx_new(const char *ctx_name, ctx_t **context, const mod_flags flags);
static void ctx_dtor(void *data);
static void default_logger(const self_t *self, const char *fmt, va_list args);
static int loop_start(ctx_t *c, const int max_events);
static uint8_t loop_stop(ctx_t *c);
static inline int loop_quit(ctx_t *c, const uint8_t quit_code);
static int recv_events(ctx_t *c, int timeout);

m_map_t *ctx = NULL;
memhook_t memhook = {0};
pthread_mutex_t mx = PTHREAD_MUTEX_INITIALIZER;

_public_ void _ctor0_ _weak_ m_pre_start(void) {
    MODULE_DEBUG("Pre-starting libmodule.");
}

static void *thread_loop(void *param) {
    const char *key = (const char *)param;
    
    m_context_loop_events(key, M_CTX_MAX_EVENTS);
    return NULL;
}

static int main_loop(void *data, const char *key, void *value) {
    pthread_t *th = *(pthread_t **)data;
    if (th) {
        static int i = 0;
        pthread_create(&th[i++], NULL, thread_loop, (void *)key);
        return 0;
    }
    *(char **)data = (char *)key;
    return -1;
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
_public_ int _weak_ main(int argc, char *argv[]) {
    void *th = NULL;
    
    /* If there is more than 1 registered ctx, alloc as many pthreads as needed */
    if (map_length(ctx) > 1) {
        MODULE_DEBUG("Allocating %ld pthreads.\n", map_length(ctx));
        th = memhook._calloc(map_length(ctx), sizeof(pthread_t));
    }
    
    /*
     * main_loop returns -1 for single-ctx runs,
     * where we only need a pointer to ctx key.
     * Ugliness warning: passing a void** ptr that is either an array of pthreads
     * or is just a space to point to single-ctx key.
     */
    if (map_iterate(ctx, main_loop, &th) == -1) {
        MODULE_DEBUG("Running in single ctx mode: '%s'\n", (const char *)th);
        return m_context_loop_events((const char *)th, M_CTX_MAX_EVENTS);
    }
    
    /* If more than 1 ctx is registered, we should join all threads */
    MODULE_DEBUG("Waiting all threads.\n");
    for (int i = 0; i < map_length(ctx); i++) {
        pthread_join(((pthread_t *)th)[i], NULL);
    }
    memhook._free(th);
    return 0;
}

static void libmodule_init(void) {
    MODULE_DEBUG("Initializing libmodule %d.%d.%d.\n", MODULE_VERSION_MAJ, MODULE_VERSION_MIN, MODULE_VERSION_PAT);
    /* Check that memhook was not overridden during m_pre_start() */
    if (!memhook._free) {
        m_set_memhook(NULL);
    }
    pthread_mutex_init(&mx, NULL);
    ctx = map_new(false, m_mem_unref);
}

static void libmodule_destroy(void) {
    MODULE_DEBUG("Destroying libmodule.\n");
    map_free(&ctx);
    pthread_mutex_destroy(&mx);
}

static int ctx_new(const char *ctx_name, ctx_t **context, const mod_flags flags) {
    MODULE_DEBUG("Creating context '%s'.\n", ctx_name);
    
    *context = m_mem_new(sizeof(ctx_t), ctx_dtor);
    MOD_ALLOC_ASSERT(*context);
    
    (*context)->flags = flags & ~(uint8_t)-1; // do not store useless module's flags (first byte)
    
    (*context)->logger = default_logger;
    
    (*context)->modules = map_new(false, m_mem_unref);
    
    if ((*context)->flags & CTX_NAME_DUP) {
        (*context)->flags |= CTX_NAME_AUTOFREE;
        (*context)->name = mem_strdup(ctx_name);
    } else {
        (*context)->name = ctx_name;
    }
    
    int ret = -ENOMEM;
    if ((*context)->modules && (*context)->name) {
        ret = map_put(ctx, (*context)->name, *context);
        if (ret == 0) {
            ret = poll_create(&(*context)->ppriv);
            if (ret == 0) {
                return 0;
            }
        }
    }
    
    /* Map_remove automatically unref ctx for us */
    if (map_remove(ctx, (*context)->name) != 0) {
        m_mem_unref(*context);
    }
    *context = NULL;
    return ret;
}

static void ctx_dtor(void *data) {
    ctx_t *context = (ctx_t *)data;
    MODULE_DEBUG("Destroying context '%s'.\n", context->name);
    map_free(&context->modules);
    poll_destroy(&context->ppriv);
    if (context->flags & CTX_NAME_AUTOFREE) {
        memhook._free((void *)context->name);
    }
}

static void default_logger(const self_t *self, const char *fmt, va_list args) {
    if (self) {
        printf("[%s]|%s|: ", self->ctx->name, self->mod->name);
    }
    vprintf(fmt, args);
}

static int loop_start(ctx_t *c, const int max_events) {
    c->ppriv.max_events = max_events;
    int ret = poll_init(&c->ppriv);
    if (ret == 0) {
        m_mem_ref(c); // Ensure ctx keeps existing while we loop
        
        fs_init(c);
        c->looping = true;
        c->quit = false;
        c->quit_code = 0;
        
        /* Eventually start any IDLE module */
        map_iterate(c->modules, evaluate_module, NULL);

        /* Tell every RUNNING module that loop is started */
        tell_system_pubsub_msg(NULL, c, LOOP_STARTED, NULL, NULL);
    }
    return ret;
}

static uint8_t loop_stop(ctx_t *c) {
    /* Tell every module that loop is stopped */
    tell_system_pubsub_msg(NULL, c, LOOP_STOPPED, NULL, NULL);

    /* Flush pubsub msg to avoid memleaks */
    map_iterate(c->modules, flush_pubsub_msgs, NULL);
    
    /* Destroy FS */
    fs_end(c);
    
    poll_clear(&c->ppriv);
    c->ppriv.max_events = 0;
    c->looping = false;
    
    int ret = c->quit_code;
    
    m_mem_unref(c);
    return ret;
}

static inline int loop_quit(ctx_t *c, const uint8_t quit_code) {
    c->quit = true;
    c->quit_code = quit_code;
    return 0;
}

static int recv_events(ctx_t *c, int timeout) {    
    errno = 0;
    int recved = 0;
    const int nfds = poll_wait(&c->ppriv, timeout);    
    for (int i = 0; i < nfds; i++) {
        ev_src_t *p = poll_recv(&c->ppriv, i);
        if (p && p->type == TYPE_FD && p->self == NULL) {
            /* Received from fuse */
            fs_process(c);
            recved++;
            continue;
        }
        
        if (p && p->self && p->self->mod) {
            mod_t *mod = p->self->mod;

            msg_t msg = { .type = p->type };
            fd_msg_t fd_msg;
            tmr_msg_t tm_msg;
            sgn_msg_t sgn_msg;
            pt_msg_t pt_msg;
            pid_msg_t pid_msg;
            ps_priv_t *ps_msg;
            
            MODULE_DEBUG("'%s' received %u type msg.\n", mod->name, msg.type);
            switch (msg.type) {
            case TYPE_FD:
                /* Received from FD */
                fd_msg.fd = p->fd_src.fd;
                msg.fd_msg = &fd_msg;
                break;
            case TYPE_PS:
                /* Received on pubsub interface */
                if (read(p->fd_src.fd, (void **)&ps_msg, sizeof(ps_priv_t *)) != sizeof(ps_priv_t *)) {
                    MODULE_DEBUG("Failed to read message: %s\n", strerror(errno));
                } else {
                    msg.ps_msg = &ps_msg->msg;
                    p = queue_peek(ps_msg->subs);            // Use real event source, ie: topic subscription if any
                }
                break;
            case TYPE_SGN:
                if (poll_consume_sgn(&c->ppriv, i, p, &sgn_msg) == 0) {
                    sgn_msg.signo = p->sgn_src.sgs.signo;
                    msg.sgn_msg = &sgn_msg;
                }
                break;
            case TYPE_TMR:
                if (poll_consume_tmr(&c->ppriv, i, p, &tm_msg) == 0) {
                    tm_msg.ms = p->tmr_src.its.ms;
                    msg.tmr_msg = &tm_msg;
                }
                break;
            case TYPE_PT:
                if (poll_consume_pt(&c->ppriv, i, p, &pt_msg) == 0) {
                    pt_msg.path = p->pt_src.pt.path;
                    msg.pt_msg = &pt_msg;
                }
                break;
            case TYPE_PID:
                if (poll_consume_pid(&c->ppriv, i, p, &pid_msg) == 0) {
                    pid_msg.pid = p->pid_src.pid.pid;
                    msg.pid_msg = &pid_msg;
                }
                break;
            default:
                MODULE_DEBUG("Unmanaged src %d.\n", msg.type);
                break;
            }

            /* 
             * Here we only check if fd_msg != NULL as 
             * all internal *_msg pointers share same memory area (inside union)
             */
            if (msg.fd_msg) {
                recved++;
                if (msg.type != TYPE_PS || msg.ps_msg->type != MODULE_POISONPILL) {
                    run_pubsub_cb(mod, &msg, p);
                } else {
                    MODULE_DEBUG("PoisonPilling '%s'.\n", mod->name);
                    stop(mod, true);
                }
                
                /* Remove it if it was a oneshot event */
                if (p && p->flags & SRC_ONESHOT) {
                    if (p->type != TYPE_PS) {
                        list_remove(mod->srcs, p, NULL);
                    } else {
                        map_remove(mod->subscriptions, p->ps_src.topic);
                    }
                }
            } else {
                errno = EAGAIN;
            }
        } else {
            /* Forward error to below handling code */
            errno = EAGAIN;
        }
    }
    
    if (recved > 0 && errno == 0) {
        map_iterate(c->modules, evaluate_module, NULL);
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

ctx_t *check_ctx(const char *ctx_name, const mod_flags flags) {
    /* 
     * Protect global variables: 
     * in this case, ensure nobody else (any other thread)
     * will create a ctx with same ctx_name at the same time.
     */
    pthread_mutex_lock(&mx);
    
    ctx_t *context = map_get(ctx, ctx_name);
    if (!context) {
        ctx_new(ctx_name, &context, flags);
    }
    
    pthread_mutex_unlock(&mx);
    return context;
}

void inline ctx_logger(const ctx_t *c, const self_t *self, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    c->logger(self, fmt, args);
    va_end(args);
}

/** Public API **/

int m_set_memhook(const memhook_t *hook) {
    /* 
     * Protect global variables: 
     * in this case, ensure memhook is correctly filled
     * when called by multiple threads.
     */
    pthread_mutex_lock(&mx);
    if (hook) {
        MOD_ASSERT(hook->_malloc, "NULL malloc fn.", -1);
        MOD_ASSERT(hook->_realloc, "NULL realloc fn.", -1);
        MOD_ASSERT(hook->_calloc, "NULL calloc fn.", -1);
        MOD_ASSERT(hook->_free, "NULL free fn.", -1);
        memcpy(&memhook, hook, sizeof(memhook_t));
    } else {
        memhook._malloc = malloc;
        memhook._realloc = realloc;
        memhook._calloc = calloc;
        memhook._free = free;
    }
    pthread_mutex_unlock(&mx);
    return 0;
}

int m_context_set_logger(const char *ctx_name, const log_cb logger) {
    MOD_PARAM_ASSERT(logger);
    FIND_CTX(ctx_name);
    
    c->logger = logger;
    return 0;
}

int m_context_loop_events(const char *ctx_name, const int max_events) {
    MOD_PARAM_ASSERT(max_events > 0);
    FIND_CTX(ctx_name);
    MOD_ASSERT(!c->looping, "Context already looping.", -EINVAL);

    int ret = loop_start(c, max_events);
    if (ret == 0) {
        while (!c->quit && map_length(c->modules) > 0) {
            recv_events(c, -1);
        }
        return loop_stop(c);
    }
    return ret;
}

int m_context_quit(const char *ctx_name, const uint8_t quit_code) {
    FIND_CTX(ctx_name);
    MOD_ASSERT(c->looping, "Context not looping.", -EINVAL);
       
    return loop_quit(c, quit_code);
}

int m_context_fd(const char *ctx_name) {
    FIND_CTX(ctx_name);
    
    return dup(poll_get_fd(&c->ppriv));
}

int m_context_dispatch(const char *ctx_name) {
    FIND_CTX(ctx_name);
    
    if (!c->looping) {
        /* Ok, start now */
        return loop_start(c, M_CTX_MAX_EVENTS);
    }
    
    if (c->quit || map_length(c->modules) == 0) {
        /* We are stopping! */
        return loop_stop(c);
    }
    
    /* Recv new events, no timeout */
    return recv_events(c, 0);
}

int m_context_dump(const char *ctx_name) {
    FIND_CTX(ctx_name);
    
    ctx_logger(c, NULL, "{\n");
    
    ctx_logger(c, NULL, "\t\"Name\": \"%s\",\n", ctx_name);
    ctx_logger(c, NULL, "\t\"State\": {\n");
    ctx_logger(c, NULL, "\t\t\"Quit\": %d,\n", c->quit);
    ctx_logger(c, NULL, "\t\t\"Looping\": %d,\n", c->looping);
    ctx_logger(c, NULL, "\t\t\"Max Events\": %d\n", c->ppriv.max_events);
    ctx_logger(c, NULL, "\t},\n");
    
    ctx_logger(c, NULL, "\t\"Modules\": [\n");
    int i = 0;
    for (m_map_itr_t *itr = map_itr_new(c->modules); itr; itr = map_itr_next(itr)) {
        const char *mod_name = map_itr_get_key(itr);
        const mod_t *mod = map_itr_get_data(itr);
        ctx_logger(c, NULL, "\t\t\"%s\": %p%c\n", mod_name, mod, ++i < map_length(c->modules) ? ',' : ' ');
    }
    ctx_logger(c, NULL, "\t]\n");
    ctx_logger(c, NULL, "}\n");
    return 0;
}


int m_context_load(const char *ctx_name, const char *module_path) {
    MOD_PARAM_ASSERT(module_path);
    FIND_CTX(ctx_name);
    
    const int module_size = map_length(c->modules);
    
    void *handle = dlopen(module_path, RTLD_NOW);
    if (!handle) {
        MODULE_DEBUG("Dlopen failed with error: %s\n", dlerror());
        return -errno;
    }
    
    /* 
     * Check that requested module has been created in requested ctx, 
     * by looking at requested ctx number of modules
     */
    if (module_size == map_length(c->modules)) { 
        dlclose(handle);
        return -EPERM;
    }
    
    mod_t *mod = map_peek(c->modules);
    mod->local_path = mem_strdup(module_path);
    return 0;
}

int m_context_unload(const char *ctx_name, const char *module_path) {
    MOD_PARAM_ASSERT(module_path);    
    FIND_CTX(ctx_name);
    
    /* Check if desired module is actually loaded in context */
    bool found = false;
    for (m_map_itr_t *itr = map_itr_new(c->modules); !found && itr; itr = map_itr_next(itr)) {
        mod_t *mod = map_itr_get_data(itr);
        if (mod->local_path && !strcmp(mod->local_path, module_path)) {
            found = true;
            memhook._free(itr);
        }
    }
    
    if (found) {
        void *handle = dlopen(module_path, RTLD_NOLOAD);
        if (handle) {
            dlclose(handle);
            return 0;
        }
        MODULE_DEBUG("Dlopen failed with error: %s\n", dlerror());
        return -errno;
    }
    MODULE_DEBUG("Module loaded from '%s' not found in ctx '%s'.\n", module_path, ctx_name);
    return -ENODEV;
}

size_t m_context_trim(const char *ctx_name, const stats_t *thres) {
    FIND_CTX(ctx_name);
    MOD_PARAM_ASSERT(thres);

    uint64_t curr_ms = 0;
    fetch_ms(&curr_ms, NULL);
    const size_t initial_size = map_length(c->modules);
    for (m_map_itr_t *itr = map_itr_new(c->modules); itr; itr = map_itr_next(itr)) {
        mod_t *mod = map_itr_get_data(itr);

        if (curr_ms - mod->stats.last_seen >= thres->inactive_ms) {
            module_deregister(&mod->self);
        } else {
            const double freq = (double)mod->stats.action_ctr / (curr_ms - mod->stats.registration_time);
            if (freq < thres->activity_freq) {
                module_deregister(&mod->self);
            }
        }
    }
    
    /* Number of deregistered modules */
    return initial_size - map_length(c->modules);
}
