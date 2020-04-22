#pragma once

#include "context.h"

/* Defines for easy API (with no need bothering with both self and ctx) */
#define m_c_set_logger(log)           m_ctx_set_logger(M_CTX_DEFAULT, log)
#define m_c_loop()                    m_ctx_loop(M_CTX_DEFAULT, M_CTX_MAX_EVENTS)
#define m_c_quit(code)                m_ctx_quit(M_CTX_DEFAULT, code)

#define m_c_fd()                      m_ctx_fd(M_CTX_DEFAULT)
#define m_c_dispatch()                m_ctx_dispatch(M_CTX_DEFAULT)

#define m_c_trim(thres)               m_ctx_trim(M_CTX_DEFAULT, thres)

#define m_c_dump()                    m_ctx_dump(M_CTX_DEFAULT)

#define m_c_load(path)                m_ctx_load(M_CTX_DEFAULT, path)
#define m_c_unload(path)              m_ctx_unload(M_CTX_DEFAULT, path)
