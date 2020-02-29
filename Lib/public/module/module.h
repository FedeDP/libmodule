#pragma once

#include "module_cmn.h"

/* Module interface functions */

/* Module registration */
_public_ mod_ret module_register(const char *name, const char *ctx_name, self_t **self, const userhook_t *hook);
_public_ mod_ret module_deregister(self_t **self);

/* External shared object module runtime loading */
_public_ mod_ret module_load(const char *module_path, const char *ctx_name);
_public_ mod_ret module_unload(const char *module_path);

/* Module state getters */
_public_ _pure_ bool module_is(const self_t *mod_self, const mod_states st);

/* Module state setters */
_public_ mod_ret module_start(const self_t *self);
_public_ mod_ret module_pause(const self_t *self);
_public_ mod_ret module_resume(const self_t *self);
_public_ mod_ret module_stop(const self_t *self);

/* Module generic functions */
_public_ __attribute__((format (printf, 2, 3))) mod_ret module_log(const self_t *self, const char *fmt, ...);
_public_ mod_ret module_dump(const self_t *self);

_public_ mod_ret module_set_userdata(const self_t *self, const void *userdata);
_public_ const void *module_get_userdata(const self_t *self);

_public_ const char *module_get_name(const self_t *mod_self);
_public_ const char *module_get_ctx(const self_t *mod_self);

_public_ mod_ret module_ref(const self_t *self, const char *name, const self_t **modref);

/* Module fds functions */
_public_ mod_ret module_register_fd(const self_t *self, const int fd, const mod_src_flags flags, const void *userptr);
_public_ mod_ret module_deregister_fd(const self_t *self, const int fd);

/* Module PubSub interface */
_public_ mod_ret module_become(const self_t *self, const recv_cb new_recv);
_public_ mod_ret module_unbecome(const self_t *self);

_public_ mod_ret module_subscribe(const self_t *self, const char *topic, const mod_src_flags flags, const void *userptr);
_public_ mod_ret module_unsubscribe(const self_t *self, const char *topic);

_public_ mod_ret module_tell(const self_t *self, const self_t *recipient, const void *message,
                                     const ssize_t size, const bool autofree);
_public_ mod_ret module_publish(const self_t *self, const char *topic, const void *message,
                                        const ssize_t size, const bool autofree);
_public_ mod_ret module_broadcast(const self_t *self, const void *message,
                                          const ssize_t size, const bool autofree, bool global);
_public_ mod_ret module_poisonpill(const self_t *self, const self_t *recipient);
_public_ mod_ret module_msg_ref(const self_t *self, const ps_msg_t *msg);
_public_ mod_ret module_msg_unref(const self_t *self, const ps_msg_t *msg);


/* Generic event source registering functions */
#define module_register_src(self, X, flags, userptr) _Generic((X) + 0, \
    int: module_register_fd, \
    char *: module_subscribe \
    )(self, X, flags, userptr);

#define module_deregister_src(self, X) _Generic((X) + 0, \
    int: module_deregister_fd, \
    char *: module_unsubscribe \
    )(self, X);
