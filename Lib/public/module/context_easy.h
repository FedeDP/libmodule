#pragma once

#include "context.h"

/* Defines for easy API (with no need bothering with both self and ctx) */
#define m_ctx_set_logger(log)         m_context_set_logger(M_CTX_DEFAULT, log)
#define m_ctx_loop()                  m_context_loop(M_CTX_DEFAULT)
#define m_ctx_quit(code)              m_context_quit(M_CTX_DEFAULT, code)

/* Define for easy looping without having to set a events limit */
#define m_context_loop(ctx)       m_context_loop_events(ctx, M_CTX_MAX_EVENTS)

#define m_ctx_fd()                    m_context_fd(M_CTX_DEFAULT)
#define m_ctx_dispatch()              m_context_dispatch(M_CTX_DEFAULT)

#define m_ctx_trim(thres)             m_context_trim(M_CTX_DEFAULT, thres)

#define m_ctx_dump()                  m_context_dump(M_CTX_DEFAULT)

#define m_ctx_load(path)              m_context_load(M_CTX_DEFAULT, path)
#define m_ctx_unload(path)            m_context_unload(M_CTX_DEFAULT, path)
