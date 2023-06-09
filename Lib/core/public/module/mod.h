#pragma once

#ifndef LIBMODULE_CORE_H
    #define LIBMODULE_CORE_H
#endif

#include <module/cmn.h>
#include <module/structs/itr.h>
#include <time.h>

/* Module event sources' types */
typedef enum {
    M_SRC_TYPE_PS,    // PubSub Source
    M_SRC_TYPE_FD,    // FD Source -> M_SRC_PRIO_HIGH flag is implicit
    M_SRC_TYPE_TMR,   // Timer Source
    M_SRC_TYPE_SGN,   // Signal Source
    M_SRC_TYPE_PATH,  // Path Source
    M_SRC_TYPE_PID,   // PID Source
    M_SRC_TYPE_TASK,  // Task source -> M_SRC_ONESHOT flag is implicit
    M_SRC_TYPE_THRESH,// Threshold source -> M_SRC_ONESHOT flag is implicit
    M_SRC_TYPE_END    // End of supported sources
} m_src_types;

/*
 * Module event sources' flags:
 * First 8 bits are reserved for common source flags.
 * Then, each source type has 8 bits reserved for its flags.
 */
#define M_SRC_SHIFT(type, val)   val << (8 * (type + 1))
typedef enum {
    M_SRC_PRIO_LOW        =       1 << 0, // Src low priority
    M_SRC_PRIO_NORM       =       1 << 1, // Src mid priority (default)
    M_SRC_PRIO_HIGH       =       1 << 2, // Src high priority
    M_SRC_AUTOFREE        =       1 << 3, // Automatically free userdata upon source deregistation.
    M_SRC_ONESHOT         =       1 << 4, // Run just once then automatically deregister source.
    M_SRC_DUP             =       1 << 5, // Duplicate PubSub topic, source fd or source path.
    M_SRC_FORCE           =       1 << 6, // Force the registration of a src, even if it is already existent (it deregisters previous src)
    M_SRC_FD_AUTOCLOSE    =       M_SRC_SHIFT(M_SRC_TYPE_FD, 1 << 0), // Automatically close fd upon deregistation.
    M_SRC_TMR_ABSOLUTE    =       M_SRC_SHIFT(M_SRC_TYPE_TMR, 1 << 0), // Absolute timer
} m_src_flags;

/* PubSub system topics. Subscribe to any of these to receive system messages. */
#define M_PS_CTX_STARTED    "LIBMODULE_CTX_STARTED"
#define M_PS_CTX_STOPPED    "LIBMODULE_CTX_STOPPED"
#define M_PS_CTX_TICK       "LIBMODULE_CTX_TICK"
#define M_PS_MOD_STARTED    "LIBMODULE_MOD_STARTED"
#define M_PS_MOD_STOPPED    "LIBMODULE_MOD_STOPPED"

/*
 * Module's pubsub API flags (m_mod_ps_tell(), m_mod_ps_publish(), m_mod_ps_broadcast())
 */
typedef enum {
    M_PS_AUTOFREE   = 1 << 0,     // Autofree PubSub data after every recipient receives message (ie: when ps_evt ref counter goes to 0)
} m_ps_flags;

/** Libmodule input src types for m_mod_src_register() API **/

typedef struct {
    clockid_t clock_id;     // Clock_id (eg: CLOCK_MONOTONIC). Unsupported on libkqueue/kqueue
    uint64_t ns;            // Timer in ns
} m_src_tmr_t;

typedef struct {
    unsigned int signo;     // Requested signal number source, as defined in signal.h
} m_src_sgn_t;

typedef struct {
    const char *path;       // Path of file/folder to which subscribe for events
    unsigned int events;    // Desired events
} m_src_path_t;

typedef struct {
    pid_t pid;              // Process id to be watched
    unsigned int events;    // Desired events. Unused on linux: only process exit is supported
} m_src_pid_t;

typedef struct {
    int tid;                // Unique task id; it allows to run multiple times the same fn
    int (*fn)(void *);      // Function to be run on thread
} m_src_task_t;

typedef struct {
    uint64_t inactive_ms;   // if != 0 -> if module is inactive for longer than this, an alarm will be received
    double activity_freq;   // if != 0 -> if module's activity is higher than this, an alarm will be received
} m_src_thresh_t;

/** Libmodule output messages received in m_mod_on_evt() callback queue **/

/* PubSub messages */
typedef struct {
    const m_mod_t *sender;
    const char *topic;
    const void *data;       // NULL for system messages
} m_evt_ps_t;

/* Generic fd event messages */
typedef struct {
    int fd;
} m_evt_fd_t;

/* Timer event messages */
typedef struct {
    uint64_t ns;
} m_evt_tmr_t;

/* Signal event messages */
typedef struct {
    unsigned int signo;
} m_evt_sgn_t;

/* Path event messages */
typedef struct {
    const char *path;
    unsigned int events;
} m_evt_path_t;

/* Pid event messages */
typedef struct {
    pid_t pid;
    unsigned int events;
} m_evt_pid_t;

/* Task event messages */
typedef struct {
    unsigned int tid;
    int retval;
} m_evt_task_t;

/* Thresh event messages */
typedef struct {
    unsigned int id;
    uint64_t inactive_ms;
    double activity_freq;
} m_evt_thresh_t;

/* Libmodule receive() main message type */
typedef struct {
    m_src_types type;                               // Event type
    union {                                         // Event data
        m_evt_fd_t      *fd_evt;
        m_evt_ps_t      *ps_evt;
        m_evt_tmr_t     *tmr_evt;
        m_evt_sgn_t     *sgn_evt;
        m_evt_path_t    *path_evt;
        m_evt_pid_t     *pid_evt;
        m_evt_task_t    *task_evt;
        m_evt_thresh_t  *thresh_evt;
    };
    const void *userdata;                           // Event userdata, passed through m_mod_src_register()
    uint64_t ts;                                    // Event timestamp
} m_evt_t;

/* Modules states */
typedef enum {
    M_MOD_IDLE = 1 << 0,
    M_MOD_RUNNING = 1 << 1,
    M_MOD_PAUSED = 1 << 2,
    M_MOD_STOPPED = 1 << 3,
    M_MOD_ZOMBIE = 1 << 4
} m_mod_states;

/*
 * Modules flags, leave upper 16b for module permissions management;
 * First 8 bits are constant flags.
 * There is still no API to update set flags though.
 */
#define M_MOD_FL_PERM(val)         val << 16
#define M_MOD_FL_MODIFIABLE(val)   val << 8
typedef enum {
    M_MOD_NAME_DUP          = 1 << 0,         // Should module's name be strdupped? (force M_MOD_NAME_AUTOFREE flag)
    M_MOD_NAME_AUTOFREE     = 1 << 1,         // Should module's name be autofreed?
    M_MOD_ALLOW_REPLACE     = M_MOD_FL_MODIFIABLE(1 << 0),         // Can module be replaced by another module with same name?
    M_MOD_PERSIST           = M_MOD_FL_MODIFIABLE(1 << 1),         // Module cannot be deregistered by direct call to m_mod_deregister (or by FS delete) while its context is looping
    M_MOD_USERDATA_AUTOFREE = M_MOD_FL_MODIFIABLE(1 << 2),         // Automatically free module userdata upon deregister
    M_MOD_DENY_CTX          = M_MOD_FL_PERM(1 << 0), // Deny access to module's ctx through m_mod_ctx() (it means the module won't be able to call ctx API)
    M_MOD_DENY_PUB          = M_MOD_FL_PERM(1 << 1), // Deny access to module's publishing functions: m_mod_ps_{tell,publish,broadcast,poisonpill}
    M_MOD_DENY_SUB          = M_MOD_FL_PERM(1 << 2), // Deny access to m_mod_ps_(un)subscribe()
} m_mod_flags;

/* Callbacks typedefs */
typedef bool (*m_start_cb)(m_mod_t *self);
typedef bool (*m_eval_cb)(m_mod_t *self);
typedef void (*m_evt_cb)(m_mod_t *self, const m_queue_t *const evts);
typedef void (*m_stop_cb)(m_mod_t *self);

/* Struct that holds user defined callbacks */
typedef struct {
    m_start_cb on_start;
    m_eval_cb on_eval;
    m_evt_cb on_evt;
    m_stop_cb on_stop;
} m_mod_hook_t;

typedef struct {
    uint64_t inactive_ms;
    double activity_freq;
    uint64_t sent_msgs;
    uint64_t recv_msgs;
} m_mod_stats_t;

/* Module interface functions */

/* Module registration */
int m_mod_register(const char *name, OUT m_mod_t **mod_ref, const m_mod_hook_t *hook,
                   m_mod_flags flags, const void *userdata);
int m_mod_deregister(OUT m_mod_t **mod);

/* Retrieve module name */
const char *m_mod_name(const m_mod_t *mod);

/* Module state getter */
bool m_mod_is(const m_mod_t *mod, m_mod_states st);
m_mod_states m_mod_state(const m_mod_t *mod);

/* Module state setters */
int m_mod_start(m_mod_t *mod);
int m_mod_pause(m_mod_t *mod);
int m_mod_resume(m_mod_t *mod);
int m_mod_stop(m_mod_t *mod);

int m_mod_bind(m_mod_t *mod, m_mod_t *ref);

/* Module generic functions */
int m_mod_log(const m_mod_t *mod, const char *fmt, ...);
int m_mod_dump(const m_mod_t *mod);
int m_mod_stats(const m_mod_t *mod, OUT m_mod_stats_t *stats);

const void *m_mod_userdata(const m_mod_t *mod);

m_mod_t *m_mod_lookup(const m_mod_t *mod, const char *name);

int m_mod_become(m_mod_t *mod, m_evt_cb new_on_evt);
int m_mod_unbecome(m_mod_t *mod);

/* Module PubSub interface (Subscribe/unsubscribe API is below under the event sources management) */
int m_mod_ps_tell(m_mod_t *mod, const m_mod_t *recipient, const void *message, m_ps_flags flags);
int m_mod_ps_publish(m_mod_t *mod, const char *topic, const void *message, m_ps_flags flags);
int m_mod_ps_poisonpill(m_mod_t *mod, const m_mod_t *recipient);

/* Events' stashing API */
int m_mod_stash(m_mod_t *mod, const m_evt_t *evt);
ssize_t m_mod_unstash(m_mod_t *mod, size_t len);

/* Event Sources management */
ssize_t m_mod_src_len(const m_mod_t *mod, m_src_types type);

int m_mod_ps_subscribe(m_mod_t *mod, const char *topic, m_src_flags flags, const void *userptr);
int m_mod_ps_unsubscribe(m_mod_t *mod, const char *topic);

int m_mod_src_register_fd(m_mod_t *mod, int fd, m_src_flags flags, const void *userptr);
int m_mod_src_deregister_fd(m_mod_t *mod, int fd);

int m_mod_src_register_tmr(m_mod_t *mod, const m_src_tmr_t *its, m_src_flags flags, const void *userptr);
int m_mod_src_deregister_tmr(m_mod_t *mod, const m_src_tmr_t *its);

int m_mod_src_register_sgn(m_mod_t *mod, const m_src_sgn_t *sgs, m_src_flags flags, const void *userptr);
int m_mod_src_deregister_sgn(m_mod_t *mod, const m_src_sgn_t *sgs);

int m_mod_src_register_path(m_mod_t *mod, const m_src_path_t *pts, m_src_flags flags, const void *userptr);
int m_mod_src_deregister_path(m_mod_t *mod, const m_src_path_t *pts);

int m_mod_src_register_pid(m_mod_t *mod, const m_src_pid_t *pid, m_src_flags flags, const void *userptr);
int m_mod_src_deregister_pid(m_mod_t *mod, const m_src_pid_t *pid);

int m_mod_src_register_task(m_mod_t *mod, const m_src_task_t *tid, m_src_flags flags, const void *userptr);
int m_mod_src_deregister_task(m_mod_t *mod, const m_src_task_t *tid);

int m_mod_src_register_thresh(m_mod_t *mod, const m_src_thresh_t *thr, m_src_flags flags, const void *userptr);
int m_mod_src_deregister_thresh(m_mod_t *mod, const m_src_thresh_t *thr);

/* Event batch management */
int m_mod_batch_set(m_mod_t *mod, size_t len, uint64_t timeout_ns);

/* Mod tokenbucket */
int m_mod_tb_set(m_mod_t *mod, uint32_t rate, uint64_t burst);

/* Generic event source registering functions */
#define m_mod_src_register(mod, X, flags, userptr) _Generic((X) + 0, \
    int: m_mod_src_register_fd, \
    char *: m_mod_ps_subscribe, \
    m_src_tmr_t *: m_mod_src_register_tmr, \
    m_src_sgn_t *: m_mod_src_register_sgn, \
    m_src_path_t *: m_mod_src_register_path, \
    m_src_pid_t *: m_mod_src_register_pid, \
    m_src_task_t *: m_mod_src_register_task, \
    m_src_thresh_t *: m_mod_src_register_thresh \
    )(mod, X, flags, userptr)

#define m_mod_src_deregister(mod, X) _Generic((X) + 0, \
    int: m_mod_src_deregister_fd, \
    char *: m_mod_ps_unsubscribe, \
    m_src_tmr_t *: m_mod_src_deregister_tmr, \
    m_src_sgn_t *: m_mod_src_deregister_sgn, \
    m_src_path_t *: m_mod_src_deregister_path, \
    m_src_pid_t *: m_mod_src_deregister_pid, \
    m_src_task_t *: m_mod_src_deregister_task, \
    m_src_thresh_t *: m_mod_src_deregister_thresh \
    )(mod, X)
