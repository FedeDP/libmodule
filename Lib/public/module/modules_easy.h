#pragma once

#include "modules.h"

/* Defines for easy API (with no need bothering with both self and ctx) */
#define modules_set_logger(log)         modules_ctx_set_logger(MODULE_DEFAULT_CTX, log)
#define modules_loop()                  modules_ctx_loop_events(MODULE_DEFAULT_CTX, MODULE_MAX_EVENTS)
#define modules_quit(code)              modules_ctx_quit(MODULE_DEFAULT_CTX, code)

/* Define for easy looping without having to set a events limit */
#define modules_ctx_loop(ctx)           modules_ctx_loop_events(ctx, MODULE_MAX_EVENTS)
