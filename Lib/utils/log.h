#pragma once

#include <stdio.h>
#include <errno.h>
#include <assert.h>

#define unlikely(x)     __builtin_expect((x),0)
#define _weak_          __attribute__((weak))
#define _public_        __attribute__((visibility("default")))

#ifndef NDEBUG
    #define M_ASSERT(cond, msg, ret)    if (unlikely(!(cond))) { M_DEBUG("%s\n", msg); return ret; }
#else
    #define M_ASSERT(cond, msg, ret)    assert(cond);
#endif
#define M_RET_ASSERT(cond, ret)     M_ASSERT(cond, "("#cond ") condition failed.", ret) 

#define M_ALLOC_ASSERT(cond)        M_RET_ASSERT(cond, -ENOMEM)
#define M_PARAM_ASSERT(cond)        M_RET_ASSERT(cond, -EINVAL)

#define M_DEBUG(...)    libmodule_logger.debug(__func__, __LINE__, __VA_ARGS__)
#define M_INFO(...)     libmodule_logger.info(__func__, __LINE__, __VA_ARGS__)
#define M_WARN(...)     libmodule_logger.warn(__func__, __LINE__, __VA_ARGS__)
#define M_ERR(...)      libmodule_logger.error(__func__, __LINE__, __VA_ARGS__)

#define X_LOG \
    X(ERROR) \
    X(WARN) \
    X(INFO) \
    X(DEBUG)

typedef enum {
    #define X(name) name,
    X_LOG
    #undef X
} m_logger_level;

typedef struct {
    void (*error)(const char *caller, int lineno, const char *fmt, ...);
    void (*warn)(const char *caller, int lineno, const char *fmt, ...);
    void (*info)(const char *caller, int lineno, const char *fmt, ...);
    void (*debug)(const char *caller, int lineno, const char *fmt, ...);
    FILE *log_file;
} m_logger;

extern m_logger libmodule_logger;
