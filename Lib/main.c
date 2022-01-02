#include <assert.h>
#include "ctx.h"
#include "priv.h"

static void libmodule_log_noop(const char *caller, int lineno, const char *fmt, ...);

/***********************************************************
 * Code related to main library ctor/dtor + main() symbol. *
 ***********************************************************/

m_map_t *ctx = NULL;
m_memhook_t memhook = { malloc, calloc, free };
pthread_mutex_t mx = PTHREAD_MUTEX_INITIALIZER;
m_logger libmodule_logger = { libmodule_log_noop, libmodule_log_noop, libmodule_log_noop, libmodule_log_noop };

_public_ _m_ctor0_ _weak_ void m_on_boot(void) {
    M_DEBUG("Booting libmodule.\n");
}

_public_ _weak_ void m_ctx_pre_loop(m_ctx_t *c, int argc, char *argv[]) {
    M_DEBUG("Pre-looping libmodule easy API.\n");
}

_public_ _weak_ void m_ctx_post_loop(m_ctx_t *c, int argc, char *argv[]) {
    M_DEBUG("Post-looping libmodule easy API.\n");
}

/** Logging functions **/
static void libmodule_log_noop(const char *caller, int lineno, const char *fmt, ...) {
}

#define X(level) \
static void libmodule_log_##level(const char *caller, int lineno, const char *fmt, ...) { \
    FILE *out = libmodule_logger.log_file; \
    if (!out) { \
        out = level >= INFO ? stdout : stderr; \
    } \
    fprintf(out, "| %c | %s:%d | ", #level[0], caller, lineno); \
    va_list args; \
    va_start(args, fmt); \
    vfprintf(out, fmt, args); \
    va_end(args); \
}
X_LOG
#undef X
/** **/

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
    
    // Load log level
    const char *log_env = getenv("LIBMODULE_LOG");
    int log_level = ERROR;
    if (log_env) {
        log_level = atoi(log_env); // in case of error, 0 (ie: error only)
    }
    switch (log_level) {
    case DEBUG:
        libmodule_logger.debug = libmodule_log_DEBUG;
    case INFO:
        libmodule_logger.info = libmodule_log_INFO;
    case WARN:
        libmodule_logger.warn = libmodule_log_WARN;
    default:
        libmodule_logger.error = libmodule_log_ERROR;
        break;
    }
    
    const char *log_file_path = getenv("LIBMODULE_LOG_OUTPUT");
    if (log_file_path) {
        libmodule_logger.log_file = fopen(log_file_path, "w");
    }
}

static _m_dtor0_ void libmodule_deinit(void) {
    M_DEBUG("Destroying libmodule.\n");
    m_map_free(&ctx);
    pthread_mutex_destroy(&mx);
    
    if (libmodule_logger.log_file) {
        fclose(libmodule_logger.log_file);
    }
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
