#include <assert.h>
#include <pthread.h>
#include "globals.h"
#include "public/module/mod_easy.h"

/***********************************************************
 * Code related to main library ctor/dtor + main() symbol. *
 ***********************************************************/

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
    m_ctx_t *c = m_ctx();
    if (!c) {
        M_ERR("No context available.\n");
        return EXIT_FAILURE;
    }
    m_ctx_pre_loop(c, argc, argv);
    const int ret = m_ctx_loop(c);
    m_ctx_post_loop(c, argc, argv);
    m_ctx_deregister(&c); // default_ctx may be NULL here, if eg: all modules where deregistered. We don't care
    return ret;
}

void mem_dtor(void *src) {
    m_mem_unref(src);
}

_public_ int m_set_memhook(  void *(*_malloc)(size_t),
                    void *(*_calloc)(size_t, size_t),
                    void (*_free)(void *)) {
    M_PARAM_ASSERT(_malloc);
    M_PARAM_ASSERT(_calloc);
    M_PARAM_ASSERT(_free);

    memhook._malloc = _malloc;
    memhook._calloc = _calloc;
    memhook._free = _free;
    return 0;
}
