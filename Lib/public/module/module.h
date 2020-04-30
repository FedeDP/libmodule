#pragma once

#include "commons.h"

/* Module interface functions */

/* Module registration */
_public_ int m_mod_register(const char *name, ctx_t *c, mod_t **mod, const userhook_t *hook, const m_mod_flags flags);
_public_ int m_mod_deregister(mod_t **mod);

/* Retrieve module context */
_public_ _pure_ ctx_t *m_mod_ctx(const mod_t *mod);

/* Retrieve module name */
_public_ _pure_ const char *m_mod_name(const mod_t *mod);

/* Module state getters */
_public_ _pure_ bool m_mod_is(const mod_t *mod, const mod_states st);

/* Module state setters */
_public_ int m_mod_start(mod_t *mod);
_public_ int m_mod_pause(mod_t *mod);
_public_ int m_mod_resume(mod_t *mod);
_public_ int m_mod_stop(mod_t *mod);

/* Module generic functions */
_public_ __attribute__((format (printf, 2, 3))) int m_mod_log(const mod_t *mod, const char *fmt, ...);
_public_ int m_mod_dump(const mod_t *mod);
_public_ _pure_ int m_mod_stats(const mod_t *mod, stats_t *stats);

_public_ int m_mod_set_userdata(mod_t *mod, const void *userdata);
_public_ const void *m_mod_get_userdata(const mod_t *mod);

_public_ int m_mod_ref(const mod_t *mod, const char *name, mod_t **modref);
_public_ int m_mod_unref(const mod_t *mod, mod_t **modref);

/* Module PubSub interface */
_public_ int m_mod_become(mod_t *mod, const recv_cb new_recv);
_public_ int m_mod_unbecome(mod_t *mod);

_public_ int m_mod_tell(mod_t *mod, const mod_t *recipient, const void *message, const mod_ps_flags flags);
_public_ int m_mod_publish(mod_t *mod, const char *topic, const void *message, const mod_ps_flags flags);
_public_ int m_mod_broadcast(mod_t *mod, const void *message, const mod_ps_flags flags);
_public_ int m_mod_poisonpill(mod_t *mod, const mod_t *recipient);

/* Event Sources management */
_public_ int m_mod_register_fd(mod_t *mod, const int fd, const mod_src_flags flags, const void *userptr);
_public_ int m_mod_deregister_fd(mod_t *mod, const int fd);

_public_ int m_mod_register_tmr(mod_t *mod, const mod_tmr_t *its, const mod_src_flags flags, const void *userptr);
_public_ int m_mod_deregister_tmr(mod_t *mod, const mod_tmr_t *its);

_public_ int m_mod_register_sgn(mod_t *mod, const mod_sgn_t *its, const mod_src_flags flags, const void *userptr);
_public_ int m_mod_deregister_sgn(mod_t *mod, const mod_sgn_t *its);

_public_ int m_mod_register_path(mod_t *mod, const mod_path_t *its, const mod_src_flags flags, const void *userptr);
_public_ int m_mod_deregister_path(mod_t *mod, const mod_path_t *its);

_public_ int m_mod_register_pid(mod_t *mod, const mod_pid_t *pid, const mod_src_flags flags, const void *userptr);
_public_ int m_mod_deregister_pid(mod_t *mod, const mod_pid_t *pid);

_public_ int m_mod_register_task(mod_t *mod, const mod_task_t *tid, const mod_src_flags flags, const void *userptr);
_public_ int m_mod_deregister_task(mod_t *mod, const mod_task_t *tid);

_public_ int m_mod_register_sub(mod_t *mod, const char *topic, const mod_src_flags flags, const void *userptr);
_public_ int m_mod_deregister_sub(mod_t *mod, const char *topic);

/* Generic event source registering functions */
#define m_mod_register_src(mod, X, flags, userptr) _Generic((X) + 0, \
    int: m_mod_register_fd, \
    char *: m_mod_register_sub, \
    mod_tmr_t *: m_mod_register_tmr, \
    mod_sgn_t *: m_mod_register_sgn, \
    mod_path_t *: m_mod_register_path, \
    mod_pid_t *: m_mod_register_pid, \
    mod_task_t *: m_mod_register_task \
    )(mod, X, flags, userptr)

#define m_mod_deregister_src(mod, X) _Generic((X) + 0, \
    int: m_mod_deregister_fd, \
    char *: m_mod_deregister_sub, \
    mod_tmr_t *: m_mod_deregister_tmr, \
    mod_sgn_t *: m_mod_deregister_sgn, \
    mod_path_t *: m_mod_deregister_path, \
    mod_pid_t *: m_mod_deregister_pid, \
    mod_task_t *: m_mod_deregister_task \
    )(mod, X)
