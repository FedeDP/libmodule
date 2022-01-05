#pragma once

#include <stdlib.h>
#include <regex.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h> // PRIu64
#include <stdio.h>
#include "itr.h"
#include "utils.h"
#include "mem.h"

#define unlikely(x)     __builtin_expect((x),0)
#define _weak_          __attribute__((weak))
#define _public_        __attribute__((visibility("default")))

#define M_DEBUG(...)    libmodule_logger.debug(__func__, __LINE__, __VA_ARGS__)
#define M_INFO(...)     libmodule_logger.info(__func__, __LINE__, __VA_ARGS__)
#define M_WARN(...)     libmodule_logger.warn(__func__, __LINE__, __VA_ARGS__)
#define M_ERR(...)      libmodule_logger.error(__func__, __LINE__, __VA_ARGS__)

#define M_ASSERT(cond, msg, ret)    if (unlikely(!(cond))) { M_DEBUG("%s\n", msg); return ret; }

#define M_RET_ASSERT(cond, ret)     M_ASSERT(cond, "("#cond ") condition failed.", ret) 

#define M_ALLOC_ASSERT(cond)        M_RET_ASSERT(cond, -ENOMEM)
#define M_PARAM_ASSERT(cond)        M_RET_ASSERT(cond, -EINVAL)

#define M_PS_MOD_POISONPILL     "LIBMODULE_MOD_POISONPILL"

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

/* Struct that holds user defined memory functions */
typedef struct {
    void *(*_malloc)(size_t size);
    void *(*_calloc)(size_t nmemb, size_t size);
    void (*_free)(void *ptr);
} m_memhook_t;

/* Global variables are defined in main.c */
extern m_map_t *ctx;
extern m_memhook_t memhook;
extern pthread_mutex_t mx;          // Used to access/modify global ctx map
extern m_logger libmodule_logger;
extern struct _ctx *default_ctx;
