#pragma once

#include <stdio.h>
#include <errno.h>
#include <assert.h>

#define unlikely(x)     __builtin_expect((x),0)
#define _weak_          __attribute__((weak))
#define _public_        __attribute__((visibility("default")))

#define X_LOG_LEVELS \
    X_LOG_LEVEL(ERR) \
    X_LOG_LEVEL(WARN) \
    X_LOG_LEVEL(INFO) \
    X_LOG_LEVEL(DEBUG)

#define X_LOG_CTXS \
    X_LOG_CTX(MEM) \
    X_LOG_CTX(STRUCTS) \
    X_LOG_CTX(THPOOL) \
    X_LOG_CTX(CORE) \
    X_LOG_CTX(OTHER)

typedef enum {
#define X_LOG_LEVEL(name) name,
    X_LOG_LEVELS
#undef X_LOG_LEVEL
    X_LOG_LEVEL_MAX
} m_logger_level;

typedef enum {
#define X_LOG_CTX(name) name,
    X_LOG_CTXS
#undef X_LOG_CTX
    X_LOG_CTX_MAX
} m_logger_context;

typedef struct {
#define X_LOG_LEVEL(name) void (*name[X_LOG_CTX_MAX])(const char *caller, int lineno, const char *fmt, ...);
    X_LOG_LEVELS
#undef X_LOG_LEVEL
    FILE *log_file;
} m_logger;

#ifndef LIBMODULE_LOG_CTX
    #define LIBMODULE_LOG_CTX OTHER
#endif
#define M_DEBUG(...)    libmodule_logger.DEBUG[LIBMODULE_LOG_CTX](__func__, __LINE__, __VA_ARGS__)
#define M_INFO(...)     libmodule_logger.INFO[LIBMODULE_LOG_CTX](__func__, __LINE__, __VA_ARGS__)
#define M_WARN(...)     libmodule_logger.WARN[LIBMODULE_LOG_CTX](__func__, __LINE__, __VA_ARGS__)
#define M_ERR(...)      libmodule_logger.ERR[LIBMODULE_LOG_CTX](__func__, __LINE__, __VA_ARGS__)

#ifndef NDEBUG
    #define M_ASSERT(cond, msg, ret)    if (unlikely(!(cond))) { M_DEBUG("%s\n", msg); return ret; }
#else
    #define M_ASSERT(cond, msg, ret)    assert(cond);
#endif
#define M_RET_ASSERT(cond, ret)     M_ASSERT(cond, "("#cond ") condition failed.", ret) 

#define M_ALLOC_ASSERT(cond)        M_RET_ASSERT(cond, -ENOMEM)
#define M_PARAM_ASSERT(cond)        M_RET_ASSERT(cond, -EINVAL)

extern m_logger libmodule_logger;
