#pragma once

#include "commons.h"

/* Modules interface functions */
_public_ int m_ctx_register(const char *ctx_name, ctx_t **c, const m_ctx_flags flags);
_public_ int m_ctx_deregister(ctx_t **c);

_public_ int m_ctx_set_logger(ctx_t *c, const log_cb logger);
_public_ int m_ctx_loop(ctx_t *c, const int max_events);
_public_ int m_ctx_quit(ctx_t *c, const uint8_t quit_code);

_public_ int m_ctx_fd(const ctx_t *c);
_public_ _pure_ const char *m_ctx_name(const ctx_t *c);
_public_ int m_ctx_dispatch(ctx_t *c);

_public_ int m_ctx_dump(const ctx_t *c);

_public_ size_t m_ctx_trim(ctx_t *c, const stats_t *thres);

/* External shared object module runtime loading */
_public_ int m_ctx_load(ctx_t *c, const char *module_path);
_public_ int m_ctx_unload(ctx_t *c, const char *module_path);
