#pragma once

#include "cmn.h"

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

/* External shared object module runtime loading */
_public_ int m_ctx_load(m_ctx_t *c, const char *module_path, const m_mod_flags flags);
_public_ int m_ctx_unload(m_ctx_t *c, const char *module_path);
