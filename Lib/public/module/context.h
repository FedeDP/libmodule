#pragma once

#include "commons.h"

/* Modules interface functions */
_public_ int m_ctx_set_logger(const char *ctx_name, const log_cb logger);
_public_ int m_ctx_loop(const char *ctx_name, const int max_events);
_public_ int m_ctx_quit(const char *ctx_name, const uint8_t quit_code);

_public_ int m_ctx_fd(const char *ctx_name);
_public_ int m_ctx_dispatch(const char *ctx_name);

_public_ int m_ctx_dump(const char *ctx_name);

_public_ size_t m_ctx_trim(const char *ctx_name, const stats_t *thres);

/* External shared object module runtime loading */
_public_ int m_ctx_load(const char *ctx_name, const char *module_path);
_public_ int m_ctx_unload(const char *ctx_name, const char *module_path);
