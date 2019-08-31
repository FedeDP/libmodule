#pragma once

#include "module_cmn.h"

/* Module interface functions */

#ifdef __cplusplus
extern "C"{
#endif

/* Module registration */
_public_ module_ret_code module_register(const char *name, const char *ctx_name, self_t **self, const userhook_t *hook);
_public_ module_ret_code module_deregister(self_t **self);

/* External shared object module runtime loading */
_public_ module_ret_code module_load(const char *module_path, const char *ctx_name);
_public_ module_ret_code module_unload(const char *module_path);

/* Module state getters */
_public_ _pure_ bool module_is(const self_t *self, const enum module_states st);

/* Module state setters */
_public_ module_ret_code module_start(const self_t *self);
_public_ module_ret_code module_pause(const self_t *self);
_public_ module_ret_code module_resume(const self_t *self);
_public_ module_ret_code module_stop(const self_t *self);

/* Module generic functions */
_public_ __attribute__((format (printf, 2, 3))) module_ret_code module_log(const self_t *self, const char *fmt, ...);
_public_ module_ret_code module_dump(const self_t *self);
_public_ module_ret_code module_set_userdata(const self_t *self, const void *userdata);

/* Module fds functions */
_public_ module_ret_code module_register_fd(const self_t *self, const int fd, const bool autoclose, const void *userptr);
_public_ module_ret_code module_deregister_fd(const self_t *self, const int fd);

/* Note that name and ctx must be freed by user */
_public_ module_ret_code module_get_name(const self_t *mod_self, char **name);
_public_ module_ret_code module_get_context(const self_t *mod_self, char **ctx);

_public_ module_ret_code module_ref(const self_t *self, const char *name, const self_t **modref);

/* Module PubSub interface */
_public_ module_ret_code module_become(const self_t *self, const recv_cb new_recv);
_public_ module_ret_code module_unbecome(const self_t *self);

_public_ module_ret_code module_subscribe(const self_t *self, const char *topic);
_public_ module_ret_code module_unsubscribe(const self_t *self, const char *topic);

_public_ module_ret_code module_tell(const self_t *self, const self_t *recipient, const void *message,
                                     const ssize_t size, const bool autofree);
_public_ module_ret_code module_publish(const self_t *self, const char *topic, const void *message,
                                        const ssize_t size, const bool autofree);
_public_ module_ret_code module_broadcast(const self_t *self, const void *message,
                                          const ssize_t size, const bool autofree, bool global);
_public_ module_ret_code module_poisonpill(const self_t *self, const self_t *recipient);

#ifdef __cplusplus
}
#endif
