#include "poll_priv.h"

#define dummy(x) x { return -ENOSYS; }

#ifndef WITH_FS
#define FsExposed(x) dummy(x)
#else
#define FsExposed(x) x
#endif

FsExposed(int fs_init(ctx_t *c));
FsExposed(int fs_process(ctx_t *c));
FsExposed(int fs_notify(const msg_t *msg));
FsExposed(int fs_cleanup(mod_t *mod));
FsExposed(int fs_end(ctx_t *c));
