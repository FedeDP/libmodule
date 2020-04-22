#pragma once

#include "module_cmn.h"

/* Module interface functions */

/* Module registration */
_public_ int m_mod_register(const char *name, const char *ctx_name, self_t **self, const userhook_t *hook, const mod_flags flags);
_public_ int m_mod_deregister(self_t **self);

/* Module state getters */
_public_ _pure_ bool m_mod_is(const self_t *mod_self, const mod_states st);

/* Module state setters */
_public_ int m_mod_start(const self_t *self);
_public_ int m_mod_pause(const self_t *self);
_public_ int m_mod_resume(const self_t *self);
_public_ int m_mod_stop(const self_t *self);

/* Module generic functions */
_public_ __attribute__((format (printf, 2, 3))) int m_mod_log(const self_t *self, const char *fmt, ...);
_public_ int m_mod_dump(const self_t *self);
_public_ _pure_ int m_mod_stats(const self_t *self, stats_t *stats);

_public_ int m_mod_set_userdata(const self_t *self, const void *userdata);
_public_ const void *m_mod_get_userdata(const self_t *self);

_public_ _pure_ const char *m_mod_name(const self_t *mod_self);
_public_ _pure_ const char *m_mod_ctx(const self_t *mod_self);

_public_ int m_mod_ref(const self_t *self, const char *name, const self_t **modref);

/* Module PubSub interface */
_public_ int m_mod_become(const self_t *self, const recv_cb new_recv);
_public_ int m_mod_unbecome(const self_t *self);

_public_ int m_mod_tell(const self_t *self, const self_t *recipient, const void *message,
                        const ssize_t size,  const mod_ps_flags flags);
_public_ int m_mod_publish(const self_t *self, const char *topic, const void *message,
                           const ssize_t size,  const mod_ps_flags flags);
_public_ int m_mod_broadcast(const self_t *self, const void *message,
                             const ssize_t size,  const mod_ps_flags flags);
_public_ int m_mod_poisonpill(const self_t *self, const self_t *recipient);

/* Event Sources management */
_public_ int m_mod_register_fd(const self_t *self, const int fd, const mod_src_flags flags, const void *userptr);
_public_ int m_mod_deregister_fd(const self_t *self, const int fd);

_public_ int m_mod_register_tmr(const self_t *self, const mod_tmr_t *its, const mod_src_flags flags, const void *userptr);
_public_ int m_mod_deregister_tmr(const self_t *self, const mod_tmr_t *its);

_public_ int m_mod_register_sgn(const self_t *self, const mod_sgn_t *its, const mod_src_flags flags, const void *userptr);
_public_ int m_mod_deregister_sgn(const self_t *self, const mod_sgn_t *its);

_public_ int m_mod_register_path(const self_t *self, const mod_path_t *its, const mod_src_flags flags, const void *userptr);
_public_ int m_mod_deregister_path(const self_t *self, const mod_path_t *its);

_public_ int m_mod_register_pid(const self_t *self, const mod_pid_t *pid, const mod_src_flags flags, const void *userptr);
_public_ int m_mod_deregister_pid(const self_t *self, const mod_pid_t *pid);

_public_ int m_mod_register_task(const self_t *self, const mod_task_t *tid, const mod_src_flags flags, const void *userptr);
_public_ int m_mod_deregister_task(const self_t *self, const mod_task_t *tid);

_public_ int m_mod_register_sub(const self_t *self, const char *topic, const mod_src_flags flags, const void *userptr);
_public_ int m_mod_deregister_sub(const self_t *self, const char *topic);

/* Generic event source registering functions */
#define m_mod_register_src(self, X, flags, userptr) _Generic((X) + 0, \
    int: m_mod_register_fd, \
    char *: m_mod_register_sub, \
    mod_tmr_t *: m_mod_register_tmr, \
    mod_sgn_t *: m_mod_register_sgn, \
    mod_path_t *: m_mod_register_path, \
    mod_pid_t *: m_mod_register_pid, \
    mod_task_t *: m_mod_register_task \
    )(self, X, flags, userptr)

#define m_mod_deregister_src(self, X) _Generic((X) + 0, \
    int: m_mod_deregister_fd, \
    char *: m_mod_deregister_sub, \
    mod_tmr_t *: m_mod_deregister_tmr, \
    mod_sgn_t *: m_mod_deregister_sgn, \
    mod_path_t *: m_mod_deregister_path, \
    mod_pid_t *: m_mod_deregister_pid, \
    mod_task_t *: m_mod_deregister_task \
    )(self, X)
