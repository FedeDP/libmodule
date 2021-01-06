#include "ctx.h"
#include "priv.h"

static _ctor1_ void libmodule_init(void);
static _dtor0_ void libmodule_deinit(void);

m_map_t *ctx = NULL;
m_memhook_t memhook = { malloc, realloc, calloc, free };
pthread_mutex_t mx = PTHREAD_MUTEX_INITIALIZER;

_public_ void _ctor0_ _weak_ m_pre_start(void) {
    M_DEBUG("Pre-starting libmodule.\n");
}

static void *thread_loop(void *param) {
    m_ctx_t *c = (m_ctx_t *)param;
    /* 
     * Store thread which registered the context;
     * Set current thread instead.
     * Loop on context until it ends.
     * Reset initial thread id.
     */ 
    pthread_t old_thid = m_ctx_get_th(c);
    m_ctx_set_th(c, pthread_self());
    m_ctx_loop(c, M_CTX_MAX_EVENTS);
    m_ctx_set_th(c, old_thid);
    return NULL;
}

/*
 * This is an exported global weak symbol.
 * It means that if a program does not implement any main(int, char *[]),
 * this will be used by default.
 *
 * All it does is looping on any ctx, each on its own thread.
 */
_public_ int _weak_ main(int argc, char *argv[]) {
    pthread_t *threads = NULL;
    if (m_map_length(ctx) > 1) {
        M_DEBUG("Allocating %ld pthreads.\n", m_map_length(ctx));
        threads = calloc(m_map_length(ctx), sizeof(pthread_t));
    }
    m_itr_foreach(ctx, {
        m_ctx_t *c = m_itr_get(itr);
        if (!threads) {
            M_DEBUG("Running in single ctx mode: '%s'\n", c->name);
            m_ctx_loop(c, M_CTX_MAX_EVENTS);
        } else {
            M_DEBUG("Starting '%s' thread\n", c->name);
            pthread_create(&threads[idx], NULL, thread_loop, c);
        }
    })
    
    if (threads) {
        M_DEBUG("Waiting all threads.\n");
        for (int i = 0; i < m_map_length(ctx); i++) {
            pthread_join(threads[i], NULL);
        }
        memhook._free(threads);
    }
    return 0;
}

static void libmodule_init(void) {
    M_DEBUG("Initializing libmodule %d.%d.%d.\n", MODULE_VERSION_MAJ, MODULE_VERSION_MIN, MODULE_VERSION_PAT);
    pthread_mutex_init(&mx, NULL);
    ctx = m_map_new(0, mem_dtor);
}

static void libmodule_deinit(void) {
    M_DEBUG("Destroying libmodule.\n");
    m_map_free(&ctx);
    pthread_mutex_destroy(&mx);
}

/** Public API **/

int m_set_memhook(const m_memhook_t *hook) {
    /* 
     * Check that we are called from within m_pre_start, 
     * when library is not yet initialized 
     */
    if (!ctx) {
        if (hook) {
            M_ASSERT(hook->_malloc, "NULL malloc fn.", -1);
            M_ASSERT(hook->_realloc, "NULL realloc fn.", -1);
            M_ASSERT(hook->_calloc, "NULL calloc fn.", -1);
            M_ASSERT(hook->_free, "NULL free fn.", -1);
            memcpy(&memhook, hook, sizeof(m_memhook_t));
            return 0;
        }
        return -EINVAL;
    }
    return -EPERM;
}
