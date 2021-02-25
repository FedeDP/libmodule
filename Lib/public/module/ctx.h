#pragma once

#ifndef LIBMODULE_CORE_H
    #define LIBMODULE_CORE_H
#endif

#include "cmn.h"

/* Logger callback */
typedef void (*m_log_cb)(const m_mod_t *ref, const char *fmt, va_list args);

int m_ctx_set_logger(m_log_cb logger);
int m_ctx_loop_events(int max_events);
int m_ctx_loop(void);
int m_ctx_quit(uint8_t quit_code);

int m_ctx_fd(void);

int m_ctx_dispatch(void);

int m_ctx_dump(void);

const char *m_ctx_get_name(void);
int m_ctx_set_name(const char *name);