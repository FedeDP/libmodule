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
    char *log_env;
    char env_name[64];
    bool log_set[X_LOG_CTX_MAX] = {0};
    for (int i = 0; i < X_LOG_CTX_MAX; i++) {
        /* Default values */
        libmodule_logger.DEBUG[i] = libmodule_log_noop;
        libmodule_logger.INFO[i] = libmodule_log_noop;
        libmodule_logger.WARN[i] = libmodule_log_noop;
        libmodule_logger.ERR[i] = libmodule_log_noop;
        
        int log_level = ERR;
        snprintf(env_name, sizeof(env_name), "LIBMODULE_LOG_%s", ctx_names[i]);
        log_env = getenv(env_name);
        if (log_env) {
            log_level = find_level(log_env);
            if (log_level != -1) {
                log_set[i] = true;
            } else {
                // Default value
                log_level = ERR;
            }
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
    
    int log_level = ERR;
    log_env = getenv("LIBMODULE_LOG");
    if (log_env) {
        log_level = find_level(log_env);
        if (log_level == -1) {
            // Default value
            log_level = ERR;
        }
    }
    for (int i = 0; i < X_LOG_CTX_MAX; i++) {
        if (!log_set[i]) {
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
