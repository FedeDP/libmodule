#include "poll_priv.h"

#define dummy(x) x { return MOD_UNSUPPORTED; }

#ifndef WITH_FUSE
#define FuseExposed(x) dummy(x)
#else
#define FuseExposed(x) x
#endif

FuseExposed(int fuse_init(ctx_t *c));
FuseExposed(int fuse_process(ctx_t *c));
FuseExposed(int fuse_end(ctx_t *c));
