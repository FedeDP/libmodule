#pragma once

#include <module_cmn.h> 

/* Defines for easy API (with no need bothering with both self and ctx) */
#define modules_set_logger(log)         modules_ctx_set_logger(DEFAULT_CTX, log)
#define modules_loop()                  modules_ctx_loop(DEFAULT_CTX)
#define modules_quit()                  modules_ctx_quit(DEFAULT_CTX)

/* Modules interface functions */
#ifdef __cplusplus
extern "C"{
#endif

_public_ void _ctor0_ _weak_ modules_pre_start(void);
_public_ module_ret_code modules_ctx_set_logger(const char *ctx_name, log_cb logger);
_public_ module_ret_code modules_ctx_loop(const char *ctx_name);
_public_ module_ret_code modules_ctx_quit(const char *ctx_name);

#ifdef __cplusplus
}
#endif
