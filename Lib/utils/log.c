#include "log.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>
#include <strings.h>

static void libmodule_log_noop(const char *caller, int lineno, const char *fmt, ...) {}

m_logger libmodule_logger;

/** Logging functions **/
#define X_LOG_LEVEL(level) \
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
X_LOG_LEVELS
#undef X_LOG_LEVEL
/** **/

static inline m_logger_level find_level(const char *level_str) {
    if (!level_str) {
        return -1;
    }
    static const char *lvl_names[] = {
        #define X_LOG_LEVEL(name) #name,
        X_LOG_LEVELS
        #undef X_LOG_LEVEL
    };
    for (int i = 0; i <= sizeof(lvl_names) / sizeof(*lvl_names); i++) {
        if (strcasecmp(level_str, lvl_names[i]) == 0) {
            return i;
        }
    }
    return -1;
}

static __attribute__((constructor (111))) void libmodule_log_init(void) {
    /* Load log level for each context */
    const char *ctx_names[] = {
        #define X_LOG_CTX(name) #name,
        X_LOG_CTXS
        #undef X_LOG_CTX
    };

    // Load fallback global level
    int global_level = find_level(getenv("LIBMODULE_LOG"));
    if (global_level == -1) {
        // Default value
        global_level = ERR;
    }

    char env_name[64];
    // Now load log levels for each context
    for (int i = 0; i < X_LOG_CTX_MAX; i++) {
        // Default noop logger
        libmodule_logger.DEBUG[i] = libmodule_log_noop;
        libmodule_logger.INFO[i] = libmodule_log_noop;
        libmodule_logger.WARN[i] = libmodule_log_noop;
        libmodule_logger.ERR[i] = libmodule_log_noop;

        snprintf(env_name, sizeof(env_name), "LIBMODULE_LOG_%s", ctx_names[i]);
        int log_level = find_level(getenv(env_name));
        if (log_level == -1) {
            log_level = global_level;
        }
        switch (log_level) {
        case DEBUG:
            libmodule_logger.DEBUG[i] = libmodule_log_DEBUG;
        case INFO:
            libmodule_logger.INFO[i] = libmodule_log_INFO;
        case WARN:
            libmodule_logger.WARN[i] = libmodule_log_WARN;
        default:
            libmodule_logger.ERR[i] = libmodule_log_ERR;
            break;
        }
    }

    const char *log_file_path = getenv("LIBMODULE_LOG_OUTPUT");
    if (log_file_path) {
        libmodule_logger.log_file = fopen(log_file_path, "w");
    }
}

static __attribute__((destructor (110))) void libmodule_log_deinit(void) {
    if (libmodule_logger.log_file) {
        fclose(libmodule_logger.log_file);
    }
}
