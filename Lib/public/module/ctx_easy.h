#pragma once

#include "ctx.h"

#define M_CTX(ctx_name) \
    _Static_assert(ctx_name != NULL, "NULL ctx name."); \
    static m_ctx_t *m_ctx; \
    static void _ctor2_ m_ctx_ctor(void) { \
        m_ctx_register(ctx_name, &m_ctx, 0, NULL); \
    } \
    static void _dtor1_ m_ctx_dtor(void) { m_ctx_deregister(&m_ctx); }

/* Defines for easy API (with no need bothering with ctx handler) */
#define m_c_set_logger(log)           m_ctx_set_logger(m_ctx, log)
#define m_c_loop()                    m_ctx_loop(m_ctx, M_CTX_MAX_EVENTS)
#define m_c_quit(code)                m_ctx_quit(m_ctx, code)

#define m_c_fd()                      m_ctx_fd(m_ctx)
#define m_c_name()                    m_ctx_name(m_ctx)
#define m_c_dispatch()                m_ctx_dispatch(m_ctx)

#define m_c_trim(thres)               m_ctx_trim(m_ctx, thres)

#define m_c_dump()                    m_ctx_dump(m_ctx)
