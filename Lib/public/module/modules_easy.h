#pragma once

#include "modules.h"

/* Defines for easy API (with no need bothering with both self and ctx) */
#define modules_set_logger(log)         modules_ctx_set_logger(MODULES_DEFAULT_CTX, log)
#define modules_loop()                  modules_ctx_loop_events(MODULES_DEFAULT_CTX, MODULES_MAX_EVENTS)
#define modules_quit(code)              modules_ctx_quit(MODULES_DEFAULT_CTX, code)

/* Define for easy looping without having to set a events limit */
#define modules_ctx_loop(ctx)           modules_ctx_loop_events(ctx, MODULES_MAX_EVENTS)

#define modules_get_fd(fd)              modules_ctx_get_fd(MODULES_DEFAULT_CTX, fd)
#define modules_dispatch(ret)           modules_ctx_dispatch(MODULES_DEFAULT_CTX, ret)

#define modules_dump()                  modules_ctx_dump(MODULES_DEFAULT_CTX)
