#include <assert.h>
#include <pthread.h>
#include "globals.h"

/***********************************************************
 * Code related to main library ctor/dtor + main() symbol. *
 ***********************************************************/

m_map_t *ctx = NULL;
pthread_mutex_t mx = PTHREAD_MUTEX_INITIALIZER;

_public_ _m_ctor0_ _weak_ void m_on_boot(void) {
    M_DEBUG("Booting libmodule.\n");
}

_public_ _weak_ void m_ctx_pre_loop(m_ctx_t *c, int argc, char *argv[]) {
    M_DEBUG("Pre-looping libmodule easy API.\n");
}

_public_ _weak_ void m_ctx_post_loop(m_ctx_t *c, int argc, char *argv[]) {
    M_DEBUG("Post-looping libmodule easy API.\n");
}

/*
 * This is an exported global weak symbol.
 * It means that if a program does not implement any main(int, char *[]),
 * this will be used by default.
 *
 * All it does is looping on default ctx.
 */

_public_ _weak_ int main(int argc, char *argv[]) {
    if (!default_ctx) {
        fprintf(stderr, "No context available.\n");
        return EXIT_FAILURE;
    }
    m_ctx_pre_loop(default_ctx, argc, argv);
    const int ret = m_ctx_loop(default_ctx);
    m_ctx_post_loop(default_ctx, argc, argv);
    m_ctx_deregister(&default_ctx); // default_ctx may be NULL here, if eg: all modules where deregistered. We don't care
    return ret;
}

static _m_ctor1_ void libmodule_init(void) {
    M_DEBUG("Initializing libmodule %d.%d.%d.\n", LIBMODULE_VERSION_MAJ, LIBMODULE_VERSION_MIN, LIBMODULE_VERSION_PAT);
    ctx = m_map_new(0, mem_dtor);
    assert(ctx != NULL);
    pthread_mutex_init(&mx, NULL);
}

static _m_dtor0_ void libmodule_deinit(void) {
    M_DEBUG("Destroying libmodule.\n");
    m_map_free(&ctx);
    pthread_mutex_destroy(&mx);
}

_public_ int m_set_memhook(  void *(*_malloc)(size_t),
                    void *(*_calloc)(size_t, size_t),
                    void (*_free)(void *)) {
    /*
     * Check that we are called from within m_pre_start,
     * when library is not yet initialized
     */
    M_RET_ASSERT(!ctx, -EPERM);
    
    M_PARAM_ASSERT(_malloc);
    M_PARAM_ASSERT(_calloc);
    M_PARAM_ASSERT(_free);
    
    memhook._malloc = _malloc;
    memhook._calloc = _calloc;
    memhook._free = _free;
    return 0;
}
