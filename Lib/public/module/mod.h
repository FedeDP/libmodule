#pragma once

#ifndef LIBMODULE_CORE_H
    #define LIBMODULE_CORE_H
#endif

#include "cmn.h"

/* Modules states */
typedef enum { M_MOD_IDLE = 0x01, M_MOD_RUNNING = 0x02, M_MOD_PAUSED = 0x04, M_MOD_STOPPED = 0x08, M_MOD_ZOMBIE = 0x10 } m_mod_states;

/* Modules flags, leave upper 16b for module permissions management */
#define M_MOD_PERM(val)     val << 16
typedef enum {
    M_MOD_NAME_DUP          = 0x01,         // Should module's name be strdupped?
    M_MOD_NAME_AUTOFREE     = 0x02,         // Should module's name be autofreed?
    M_MOD_ALLOW_REPLACE     = 0x04,         // Can module be replaced by another module with same name?
    M_MOD_PERSIST           = 0x08,         // Module cannot be deregistered by directly call to m_mod_deregister (or by FS delete) while its context is looping
    M_MOD_USERDATA_AUTOFREE = 0x10,         // Automatically free module userdata upon deregister
    M_MOD_DENY_CTX          = M_MOD_PERM(0x01), // Deny access to module's ctx through m_mod_ctx() (it means the module won't be able to call ctx API)
    M_MOD_DENY_LOAD         = M_MOD_PERM(0x02), // Deny access to m_mod_(un)load()
    M_MOD_DENY_PUB          = M_MOD_PERM(0x04), // Deny access to module's publishing functions: m_mod_ps_{tell,publish,broadcast,poisonpill}
    M_MOD_DENY_SUB          = M_MOD_PERM(0x08), // Deny access to m_mod_src_register_sub()
} m_mod_flags;

/* Callbacks typedefs */
typedef bool (*m_start_cb)(void);
typedef bool (*m_eval_cb)(void);
typedef void (*m_evt_cb)(const m_evt_t *const msg);
typedef void (*m_stop_cb)(void);

/* Struct that holds user defined callbacks */
typedef struct {
    m_start_cb on_start;
    m_eval_cb on_eval;
    m_evt_cb on_evt;
    m_stop_cb on_stop;
} m_userhook_t;

/* Module interface functions */

/* Module registration */
_public_ int m_mod_register(const char *name, m_ctx_t *c, m_mod_t **mod, const m_userhook_t *hook, 
                            const m_mod_flags flags, const void *userdata);
_public_ int m_mod_deregister(m_mod_t **mod);

/* External shared object module runtime loading */
_public_ int m_mod_load(const m_mod_t *mod, const char *module_path, const m_mod_flags flags, m_mod_t **ref);
_public_ int m_mod_unload(const m_mod_t *mod, const char *module_path);

/* Retrieve module context */
_public_ _pure_ m_ctx_t *m_mod_ctx(const m_mod_t *mod);

/* Retrieve module name */
_public_ _pure_ const char *m_mod_name(const m_mod_t *mod);

/* Module state getter */
_public_ _pure_ bool m_mod_is(const m_mod_t *mod, const m_mod_states st);

/* Module state setters */
_public_ int m_mod_start(m_mod_t *mod);
_public_ int m_mod_pause(m_mod_t *mod);
_public_ int m_mod_resume(m_mod_t *mod);
_public_ int m_mod_stop(m_mod_t *mod);

/* Module generic functions */
_public_ __attribute__((format (printf, 2, 3))) int m_mod_log(const m_mod_t *mod, const char *fmt, ...);
_public_ int m_mod_dump(const m_mod_t *mod);
_public_ _pure_ int m_mod_stats(const m_mod_t *mod, m_stats_t *stats);

_public_ int m_mod_set_userdata(m_mod_t *mod, const void *userdata);
_public_ const void *m_mod_get_userdata(const m_mod_t *mod);

_public_ m_mod_t *m_mod_ref(const m_mod_t *mod, const char *name);

_public_ int m_mod_become(m_mod_t *mod, const m_evt_cb new_recv);
_public_ int m_mod_unbecome(m_mod_t *mod);

/* Module PubSub interface */
_public_ int m_mod_ps_tell(m_mod_t *mod, const m_mod_t *recipient, const void *message, const m_ps_flags flags);
_public_ int m_mod_ps_publish(m_mod_t *mod, const char *topic, const void *message, const m_ps_flags flags);
_public_ int m_mod_ps_broadcast(m_mod_t *mod, const void *message, const m_ps_flags flags);
_public_ int m_mod_ps_poisonpill(m_mod_t *mod, const m_mod_t *recipient);
_public_ int m_mod_ps_subscribe(m_mod_t *mod, const char *topic, const m_src_flags flags, const void *userptr);
_public_ int m_mod_ps_unsubscribe(m_mod_t *mod, const char *topic);

/* Event Sources management */
_public_ int m_mod_src_register_fd(m_mod_t *mod, const int fd, const m_src_flags flags, const void *userptr);
_public_ int m_mod_src_deregister_fd(m_mod_t *mod, const int fd);

_public_ int m_mod_src_register_tmr(m_mod_t *mod, const m_src_tmr_t *its, const m_src_flags flags, const void *userptr);
_public_ int m_mod_src_deregister_tmr(m_mod_t *mod, const m_src_tmr_t *its);

_public_ int m_mod_src_register_sgn(m_mod_t *mod, const m_src_sgn_t *its, const m_src_flags flags, const void *userptr);
_public_ int m_mod_src_deregister_sgn(m_mod_t *mod, const m_src_sgn_t *its);

_public_ int m_mod_src_register_path(m_mod_t *mod, const m_src_path_t *its, const m_src_flags flags, const void *userptr);
_public_ int m_mod_src_deregister_path(m_mod_t *mod, const m_src_path_t *its);

_public_ int m_mod_register_pid(m_mod_t *mod, const m_src_pid_t *pid, const m_src_flags flags, const void *userptr);
_public_ int m_mod_deregister_pid(m_mod_t *mod, const m_src_pid_t *pid);

_public_ int m_mod_src_register_task(m_mod_t *mod, const m_src_task_t *tid, const m_src_flags flags, const void *userptr);
_public_ int m_mod_src_deregister_task(m_mod_t *mod, const m_src_task_t *tid);

/* Generic event source registering functions */
#define m_mod_src_register(mod, X, flags, userptr) _Generic((X) + 0, \
    int: m_mod_src_register_fd, \
    char *: m_mod_ps_subscribe, \
    m_src_tmr_t *: m_mod_src_register_tmr, \
    m_src_sgn_t *: m_mod_src_register_sgn, \
    m_src_path_t *: m_mod_src_register_path, \
    m_src_pid_t *: m_mod_register_pid, \
    m_src_task_t *: m_mod_src_register_task \
    )(mod, X, flags, userptr)

#define m_mod_src_deregister(mod, X) _Generic((X) + 0, \
    int: m_mod_src_deregister_fd, \
    char *: m_mod_ps_unsubscribe, \
    m_src_tmr_t *: m_mod_src_deregister_tmr, \
    m_src_sgn_t *: m_mod_src_deregister_sgn, \
    m_src_path_t *: m_mod_src_deregister_path, \
    m_src_pid_t *: m_mod_deregister_pid, \
    m_src_task_t *: m_mod_src_deregister_task \
    )(mod, X)
