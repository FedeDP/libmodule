#include "ctx.h"
#include "priv.h"

static _ctor1_ void libmodule_init(void);
static _dtor0_ void libmodule_deinit(void);

m_map_t *ctx = NULL;
memhook_t memhook = { malloc, realloc, calloc, free };
pthread_mutex_t mx = PTHREAD_MUTEX_INITIALIZER;

_public_ void _ctor0_ _weak_ m_pre_start(void) {
    M_DEBUG("Pre-starting libmodule.\n");
}

/*
 * This is an exported global weak symbol.
 * It means that if a program does not implement any main(int, char *[]),
 * this will be used by default.
 *
 * All it does is looping on M_CTX_DEFAULT ctx.
 */
_public_ int _weak_ main(int argc, char *argv[]) {
    ctx_t *c = m_map_get(ctx, M_CTX_DEFAULT);
    return m_ctx_loop(c, M_CTX_MAX_EVENTS);
}

static void libmodule_init(void) {
    M_DEBUG("Initializing libmodule %d.%d.%d.\n", MODULE_VERSION_MAJ, MODULE_VERSION_MIN, MODULE_VERSION_PAT);
    pthread_mutex_init(&mx, NULL);
    ctx = m_map_new(0, m_mem_unref);
}

static void libmodule_deinit(void) {
    M_DEBUG("Destroying libmodule.\n");
    m_map_free(&ctx);
    pthread_mutex_destroy(&mx);
}
