#pragma once

#include "module_cmn.h"

/* Module interface functions */

#ifdef __cplusplus
extern "C"{
#endif

/* Module registration */
_public_ module_ret_code module_register(const char *name, const char *ctx_name, const self_t **self, const userhook *hook);
_public_ module_ret_code module_deregister(const self_t **self);

/* Module state getters */
_public_ _pure_ bool module_is(const self_t *self, const enum module_states st);

/* Module state setters */
_public_ module_ret_code module_start(const self_t *self);
_public_ module_ret_code module_pause(const self_t *self);
_public_ module_ret_code module_resume(const self_t *self);
_public_ module_ret_code module_stop(const self_t *self);

/* Module generic functions */
_public_ __attribute__((format (printf, 2, 3))) module_ret_code module_log(const self_t *self, const char *fmt, ...);
_public_ module_ret_code module_set_userdata(const self_t *self, const void *userdata);
_public_ module_ret_code module_register_fd(const self_t *self, const int fd, const bool autoclose, const void *userptr);
_public_ module_ret_code module_deregister_fd(const self_t *self, const int fd);

/* Note that both name and ctx must be freed by user */
_public_ _pure_ module_ret_code module_get_name(const self_t *mod_self, char **name);
_public_ _pure_ module_ret_code module_get_context(const self_t *mod_self, char **ctx);

/* Module PubSub interface */
_public_ module_ret_code module_ref(const self_t *self, const char *name, const self_t **modref);

_public_ module_ret_code module_become(const self_t *self, const recv_cb new_recv);
_public_ module_ret_code module_unbecome(const self_t *self);

_public_ module_ret_code module_register_topic(const self_t *self, const char *topic);
_public_ module_ret_code module_deregister_topic(const self_t *self, const char *topic);
_public_ module_ret_code module_subscribe(const self_t *self, const char *topic);
_public_ module_ret_code module_unsubscribe(const self_t *self, const char *topic);

_public_ module_ret_code module_tell(const self_t *self, const self_t *recipient, const unsigned char *message,
                                        const ssize_t size);
_public_ module_ret_code module_publish(const self_t *self, const char *topic, const unsigned char *message,
                                        const ssize_t size);
_public_ module_ret_code module_broadcast(const self_t *self, const unsigned char *message,
                                        const ssize_t size);

#ifdef __cplusplus
}
#endif
