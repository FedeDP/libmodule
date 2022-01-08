#include "log.h"

#include <stdarg.h>
#include <stdlib.h>

static void libmodule_log_noop(const char *caller, int lineno, const char *fmt, ...) {}

m_logger libmodule_logger = { libmodule_log_noop, libmodule_log_noop, libmodule_log_noop, libmodule_log_noop };

/** Logging functions **/
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

static __attribute__((constructor (111))) void libmodule_log_init(void) {
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

static __attribute__((destructor (110))) void libmodule_deinit(void) {
    if (libmodule_logger.log_file) {
        fclose(libmodule_logger.log_file);
    }
}
