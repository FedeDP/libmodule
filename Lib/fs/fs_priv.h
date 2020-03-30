#include "poll_priv.h"

#define dummy(x) x { return MOD_UNSUPPORTED; }

#ifndef WITH_FS
#define FsExposed(x) dummy(x)
#else
#define FsExposed(x) x
#endif

FsExposed(mod_ret fs_init(ctx_t *c));
FsExposed(mod_ret fs_process(ctx_t *c));
FsExposed(mod_ret fs_notify(const msg_t *msg));
FsExposed(mod_ret fs_end(ctx_t *c));
