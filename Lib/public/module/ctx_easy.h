#pragma once

#include "ctx.h"

#define ctx() *get_ctx()

#define M_CTX() \
    static inline m_ctx_t **get_ctx() { static m_ctx_t *_ctx = NULL; return &_ctx; } \
    static void _ctor2_ m_ctx_ctor(void) { \
        m_ctx_register(M_CTX_DEFAULT, get_ctx(), 0, NULL); \
    } \
    static void _dtor1_ m_ctx_dtor(void) { m_ctx_deregister(get_ctx()); }

/* Defines for easy API (with no need bothering with both self and ctx) */
#define m_c_set_logger(log)           m_ctx_set_logger(ctx(), log)
#define m_c_loop()                    m_ctx_loop(ctx(), M_CTX_MAX_EVENTS)
#define m_c_quit(code)                m_ctx_quit(ctx(), code)

#define m_c_fd()                      m_ctx_fd(ctx())
#define m_c_name()                    m_ctx_name(ctx())
#define m_c_dispatch()                m_ctx_dispatch(ctx())

#define m_c_trim(thres)               m_ctx_trim(ctx(), thres)

#define m_c_dump()                    m_ctx_dump(ctx())

#define m_c_load(path, flags)         m_ctx_load(ctx(), path, flags)
#define m_c_unload(path)              m_ctx_unload(ctx(), path)
