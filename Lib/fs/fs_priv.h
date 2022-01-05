#pragma once

#include "public/module/mod.h"
#include "public/module/ctx.h"

#define dummy(x) x { return -ENOSYS; }

#ifndef WITH_FS
#define FsExposed(x) dummy(x)
#else
#define FsExposed(x) x
#endif

FsExposed(int fs_init(m_ctx_t *c));
FsExposed(int fs_process(m_ctx_t *c));
FsExposed(int fs_notify(m_mod_t *mod, const m_queue_t *const evts));
FsExposed(int fs_ctx_stopped(m_mod_t *mod));
FsExposed(int fs_cleanup(m_mod_t *mod));
FsExposed(int fs_end(m_ctx_t *c));
