#pragma once

#ifndef LIBMODULE_CORE_H
    #define LIBMODULE_CORE_H
#endif

#include "cmn.h"

#define M_CTX_MAX_EVENTS   64

/* Context flags */
typedef enum {
    M_CTX_NAME_DUP          = 0x01,         // Should ctx's name be strdupped? (only if a ctx is created during a register)
    M_CTX_NAME_AUTOFREE     = 0x02,         // Should ctx's name be autofreed? (only if a ctx is created during a register)
    M_CTX_PERSIST           = 0x04,         // Prevent ctx automatic destroying when there are no modules in it anymore. With this option, context is kept alive until m_ctx_deregister() is called.
    M_CTX_USERDATA_AUTOFREE = 0x08          // Automatically free ctx userdata upon deregister
} m_ctx_flags;

/* Logger callback */
typedef void (*m_log_cb)(const m_mod_t *ref, const char *fmt, va_list args);

/* Modules interface functions */
_public_ int m_ctx_register(const char *ctx_name, m_ctx_t **c, const m_ctx_flags flags, const void *userdata);
_public_ int m_ctx_deregister(m_ctx_t **c);

_public_ int m_ctx_set_logger(m_ctx_t *c, const m_log_cb logger);
_public_ int m_ctx_loop(m_ctx_t *c, const int max_events);
_public_ int m_ctx_quit(m_ctx_t *c, const uint8_t quit_code);

_public_ int m_ctx_fd(const m_ctx_t *c);
_public_ _pure_ const char *m_ctx_name(const m_ctx_t *c);

_public_ int m_ctx_set_userdata(m_ctx_t *c, const void *userdata);
_public_ const void *m_ctx_get_userdata(const m_ctx_t *c);

_public_ int m_ctx_dispatch(m_ctx_t *c);

_public_ int m_ctx_dump(const m_ctx_t *c);

_public_ size_t m_ctx_trim(m_ctx_t *c, const m_stats_t *thres);

_public_ pthread_t m_ctx_get_th(m_ctx_t *c);
_public_ int m_ctx_set_th(m_ctx_t *c, const pthread_t id);
