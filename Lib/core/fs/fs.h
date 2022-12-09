#pragma once

#include "public/module/mod.h"
#include "public/module/ctx.h"

int fs_init(m_ctx_t *c);
int fs_notify(m_mod_t *mod, const m_queue_t *const evts);
int fs_ctx_stopped(m_mod_t *mod);
int fs_cleanup(m_mod_t *mod);
int fs_end(m_ctx_t *c);
