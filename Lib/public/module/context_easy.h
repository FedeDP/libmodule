#pragma once

#include "context.h"

/* Defines for easy API (with no need bothering with both self and ctx) */
#define m_ctx_set_logger(log)         m_context_set_logger(MODULES_DEFAULT_CTX, log)
#define m_ctx_loop()                  m_context_loop(MODULES_DEFAULT_CTX)
#define m_ctx_quit(code)              m_context_quit(MODULES_DEFAULT_CTX, code)

/* Define for easy looping without having to set a events limit */
#define m_context_loop(ctx)       m_context_loop_events(ctx, MODULES_MAX_EVENTS)

#define m_ctx_fd()                    m_context_fd(MODULES_DEFAULT_CTX)
#define m_ctx_dispatch()              m_context_dispatch(MODULES_DEFAULT_CTX)

#define m_ctx_trim(thres)             m_context_trim(MODULES_DEFAULT_CTX, thres)

#define m_ctx_dump()                  m_context_dump(MODULES_DEFAULT_CTX)

#define m_ctx_load(path)              m_context_load(MODULES_DEFAULT_CTX, path)
#define m_ctx_unload(path)            m_context_unload(MODULES_DEFAULT_CTX, path)
