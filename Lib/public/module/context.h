#pragma once

#include "module_cmn.h"

/* Modules interface functions */
_public_ int m_set_memhook(const memhook_t *hook); // common to any context; set this in modules_pre_start()
_public_ int m_context_set_logger(const char *ctx_name, const log_cb logger);
_public_ int m_context_loop_events(const char *ctx_name, const int max_events);
_public_ int m_context_quit(const char *ctx_name, const uint8_t quit_code);

_public_ int m_context_fd(const char *ctx_name);
_public_ int m_context_dispatch(const char *ctx_name);

_public_ int m_context_dump(const char *ctx_name);

_public_ size_t m_context_trim(const char *ctx_name, const stats_t *thres);

/* External shared object module runtime loading */
_public_ int m_context_load(const char *ctx_name, const char *module_path);
_public_ int m_context_unload(const char *ctx_name, const char *module_path);
