#pragma once

#include "module_cmn.h"

/* Modules interface functions */
#ifdef __cplusplus
extern "C"{
#endif

_public_ void _ctor0_ _weak_ modules_pre_start(void);
_public_ int _weak_ main(int argc, char *argv[]);
_public_ module_ret_code modules_set_memalloc_hook(const memalloc_hook *hook);
_public_ module_ret_code modules_ctx_set_logger(const char *ctx_name, const log_cb logger);
_public_ module_ret_code modules_ctx_loop_events(const char *ctx_name, const int max_events);
_public_ module_ret_code modules_ctx_quit(const char *ctx_name, const uint8_t quit_code);

_public_ module_ret_code modules_ctx_get_fd(const char *ctx_name, int *fd);
_public_ module_ret_code modules_ctx_dispatch(const char *ctx_name, int *ret);

#ifdef __cplusplus
}
#endif
