#include <assert.h>
#include <pthread.h>
#include "globals.h"
#include "public/module/mod_easy.h"

/***********************************************************
 * Code related to main library ctor/dtor + main() symbol. *
 ***********************************************************/

m_map_t *ctxs_map = NULL;
pthread_mutex_t ctxs_mx = PTHREAD_MUTEX_INITIALIZER;

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
    m_ctx_t *c = m_ctx_ref(M_CTX_DEFAULT);
    if (!c) {
        M_ERR("No context available.\n");
        return EXIT_FAILURE;
    }
    m_ctx_pre_loop(c, argc, argv);
    const int ret = m_ctx_loop(c);
    m_ctx_post_loop(c, argc, argv);
    m_ctx_deregister(&c); // default_ctx may be NULL here, if eg: all modules where deregistered. We don't care
    m_mem_unrefp((void **)&c);
    return ret;
}

static _m_ctor1_ void libmodule_init(void) {
    M_INFO("Initializing libmodule %d.%d.%d.\n", LIBMODULE_VERSION_MAJ, LIBMODULE_VERSION_MIN, LIBMODULE_VERSION_PAT);
    ctxs_map = m_map_new(0, mem_dtor);
    assert(ctxs_map != NULL);
    pthread_mutex_init(&ctxs_mx, NULL);
}

static _m_dtor0_ void libmodule_deinit(void) {
    M_INFO("Destroying libmodule.\n");
    m_map_free(&ctxs_map);
    pthread_mutex_destroy(&ctxs_mx);
}

void mem_dtor(void *src) {
    m_mem_unref(src);
}

_public_ int m_set_memhook(  void *(*_malloc)(size_t),
                    void *(*_calloc)(size_t, size_t),
                    void (*_free)(void *)) {
    /*
     * Check that we are called from within m_on_boot,
     * when library is not yet initialized
     */
    M_RET_ASSERT(!ctxs_map, -EPERM);
    
    M_PARAM_ASSERT(_malloc);
    M_PARAM_ASSERT(_calloc);
    M_PARAM_ASSERT(_free);
    
    memhook._malloc = _malloc;
    memhook._calloc = _calloc;
    memhook._free = _free;
    return 0;
}
