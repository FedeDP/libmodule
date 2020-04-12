#pragma once

#include "module_cmn.h"

/* Module interface functions */

/* Module registration */
_public_ int m_module_register(const char *name, const char *ctx_name, self_t **self, const userhook_t *hook, const mod_flags flags);
_public_ int m_module_deregister(self_t **self);

/* Module state getters */
_public_ _pure_ bool m_module_is(const self_t *mod_self, const mod_states st);

/* Module state setters */
_public_ int m_module_start(const self_t *self);
_public_ int m_module_pause(const self_t *self);
_public_ int m_module_resume(const self_t *self);
_public_ int m_module_stop(const self_t *self);

/* Module generic functions */
_public_ __attribute__((format (printf, 2, 3))) int m_module_log(const self_t *self, const char *fmt, ...);
_public_ int m_module_dump(const self_t *self);
_public_ _pure_ int m_module_stats(const self_t *self, stats_t *stats);

_public_ int m_module_set_userdata(const self_t *self, const void *userdata);
_public_ const void *m_module_get_userdata(const self_t *self);

_public_ _pure_ const char *m_module_name(const self_t *mod_self);
_public_ _pure_ const char *m_module_ctx(const self_t *mod_self);

_public_ int m_module_ref(const self_t *self, const char *name, const self_t **modref);

/* Module srcs functions */
_public_ int m_module_register_fd(const self_t *self, const int fd, const mod_src_flags flags, const void *userptr);
_public_ int m_module_deregister_fd(const self_t *self, const int fd);

_public_ int m_module_register_tmr(const self_t *self, const mod_tmr_t *its, const mod_src_flags flags, const void *userptr);
_public_ int m_module_deregister_tmr(const self_t *self, const mod_tmr_t *its);

_public_ int m_module_register_sgn(const self_t *self, const mod_sgn_t *its, const mod_src_flags flags, const void *userptr);
_public_ int m_module_deregister_sgn(const self_t *self, const mod_sgn_t *its);

_public_ int m_module_register_pt(const self_t *self, const mod_pt_t *its, const mod_src_flags flags, const void *userptr);
_public_ int m_module_deregister_pt(const self_t *self, const mod_pt_t *its);

_public_ int m_module_register_pid(const self_t *self, const mod_pid_t *pid, const mod_src_flags flags, const void *userptr);
_public_ int m_module_deregister_pid(const self_t *self, const mod_pid_t *pid);

/* Module PubSub interface */
_public_ int m_module_become(const self_t *self, const recv_cb new_recv);
_public_ int m_module_unbecome(const self_t *self);

_public_ int m_module_register_sub(const self_t *self, const char *topic, const mod_src_flags flags, const void *userptr);
_public_ int m_module_deregister_sub(const self_t *self, const char *topic);

_public_ int m_module_tell(const self_t *self, const self_t *recipient, const void *message,
                             const ssize_t size,  const mod_ps_flags flags);
_public_ int m_module_publish(const self_t *self, const char *topic, const void *message,
                                const ssize_t size,  const mod_ps_flags flags);
_public_ int m_module_broadcast(const self_t *self, const void *message,
                                  const ssize_t size,  const mod_ps_flags flags);
_public_ int m_module_poisonpill(const self_t *self, const self_t *recipient);

/* Generic event source registering functions */
#define m_module_register_src(self, X, flags, userptr) _Generic((X) + 0, \
    int: m_module_register_fd, \
    char *: m_module_register_sub, \
    mod_tmr_t *: m_module_register_tmr, \
    mod_sgn_t *: m_module_register_sgn, \
    mod_pt_t *: m_module_register_pt, \
    mod_pid_t *: m_module_register_pid \
    )(self, X, flags, userptr);

#define m_module_deregister_src(self, X) _Generic((X) + 0, \
    int: m_module_deregister_fd, \
    char *: m_module_deregister_sub, \
    mod_tmr_t *: m_module_deregister_tmr, \
    mod_sgn_t *: m_module_deregister_sgn, \
    mod_pt_t *: m_module_deregister_pt, \
    mod_pid_t *: m_module_deregister_pid \
    )(self, X);
