#include "context.h"
#include "priv.h"

static void *thread_loop(void *param);
static int main_loop(void *data, const char *key, void *value);
static _ctor1_ void libmodule_init(void);
static _dtor0_ void libmodule_destroy(void);

m_map_t *ctx = NULL;
memhook_t memhook = { malloc, realloc, calloc, free };
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
    pthread_mutex_init(&mx, NULL);
    ctx = map_new(0, m_mem_unref);
}

static void libmodule_destroy(void) {
    MODULE_DEBUG("Destroying libmodule.\n");
    map_free(&ctx);
    pthread_mutex_destroy(&mx);
}
