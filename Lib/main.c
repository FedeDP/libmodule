#include <assert.h>
#include <fs.h>
#include "ctx.h"
#include "priv.h"

m_ctx_t *ctx = NULL;
m_memhook_t memhook = { malloc, calloc, free };

_public_ _m_ctor0_ _weak_ void m_pre_start(void) {
    M_DEBUG("Pre-starting libmodule.\n");
}

_public_ _weak_ void m_ctx_pre_loop(int argc, char *argv[]) {
    m_ctx_set_name(argv[0]);
    bool error = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--name") == 0) {
            if (i + 1 < argc) {
                m_ctx_set_name(argv[++i]);
            } else {
                error = true;
            }
        }
#ifdef WITH_FS
        else if (strcmp(argv[i], "--fsroot") == 0) {
            if (i + 1 < argc) {
                m_ctx_fs_set_root(argv[++i]);
            } else {
                error = true;
            }
        }
#endif
        else {
            error = true;
        }
        if (error) {
            fprintf(stderr, "failed to parse '%s' arg.\n", argv[i]);
            exit(EXIT_FAILURE);
        }
    }
}

/*
 * This is an exported global weak symbol.
 * It means that if a program does not implement any main(int, char *[]),
 * this will be used by default.
 *
 * All it does is looping on ctx.
 */
_public_ _weak_ int main(int argc, char *argv[]) {
    m_ctx_pre_loop(argc, argv);
    return m_ctx_loop();
}

static _m_ctor1_ void libmodule_init(void) {
    M_DEBUG("Initializing libmodule %d.%d.%d.\n", LIBMODULE_VERSION_MAJ, LIBMODULE_VERSION_MIN, LIBMODULE_VERSION_PAT);
    ctx_new(&ctx);
    assert(ctx != NULL);
}

static _m_dtor0_ void libmodule_deinit(void) {
    M_DEBUG("Destroying libmodule.\n");
    m_mem_unref(ctx);
}

/** Public API **/

int m_set_memhook(const m_memhook_t *hook) {
    /*
     * Check that we are called from within m_pre_start,
     * when library is not yet initialized
     */
    M_RET_ASSERT(!ctx, -EPERM);
    M_PARAM_ASSERT(hook);
    M_PARAM_ASSERT(hook->_malloc);
    M_PARAM_ASSERT(hook->_calloc);
    M_PARAM_ASSERT(hook->_free);

    memcpy(&memhook, hook, sizeof(m_memhook_t));
    return 0;
}
