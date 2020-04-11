#pragma once

#include "modules.h"

/* Defines for easy API (with no need bothering with both self and ctx) */
#define modules_set_logger(log)         modules_ctx_set_logger(MODULES_DEFAULT_CTX, log)
#define modules_loop()                  modules_ctx_loop_events(MODULES_DEFAULT_CTX, MODULES_MAX_EVENTS)
#define modules_quit(code)              modules_ctx_quit(MODULES_DEFAULT_CTX, code)

/* Define for easy looping without having to set a events limit */
#define modules_ctx_loop(ctx)           modules_ctx_loop_events(ctx, MODULES_MAX_EVENTS)

#define modules_fd()                    modules_ctx_fd(MODULES_DEFAULT_CTX)
#define modules_dispatch()              modules_ctx_dispatch(MODULES_DEFAULT_CTX)

#define modules_trim(thres)             modules_ctx_trim(MODULES_DEFAULT_CTX, thres)

#define modules_dump()                  modules_ctx_dump(MODULES_DEFAULT_CTX)

#define modules_load(path)              modules_ctx_load(MODULES_DEFAULT_CTX, path)
#define modules_unload(path)            modules_ctx_unload(MODULES_DEFAULT_CTX, path)
