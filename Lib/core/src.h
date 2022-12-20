#pragma once

#include "mod.h"

#define M_SRC_INTERNAL          1 << 7
#define M_SRC_PRIO_MASK         (M_SRC_PRIO_HIGH << 1) - 1
#define M_SRC_ASSERT_PRIO_FLAGS() \
    int prio_flags = flags & M_SRC_PRIO_MASK; \
    M_PARAM_ASSERT(prio_flags == 0 || __builtin_popcount(prio_flags) == 1); \
    if (prio_flags == 0) \
        flags |= M_SRC_PRIO_NORM;

/* Forward declare evt_priv_t to avoid dep cycle */
typedef struct _ev_priv evt_priv_t;

/* Struct that holds fds to self_t mapping for poll plugin */
typedef struct {
    int fd;
} fd_src_t;

/* Struct that holds timers to self_t mapping for poll plugin */
typedef struct {
#ifdef __linux__
    fd_src_t f;
#endif
    m_src_tmr_t its;
} tmr_src_t;

/* Struct that holds signals to self_t mapping for poll plugin */
typedef struct {
#ifdef __linux__
    fd_src_t f;
#endif
    m_src_sgn_t sgs;
} sgn_src_t;

/* Struct that holds paths to self_t mapping for poll plugin */
typedef struct {
    fd_src_t f; // in kqueue EVFILT_VNODE: open(path) is needed. Thus a fd is needed too.
    m_src_path_t pt;
} path_src_t;

/* Struct that holds pids to self_t mapping for poll plugin */
typedef struct {
#ifdef __linux__
    fd_src_t f;
#endif
    m_src_pid_t pid;
} pid_src_t;

/* Struct that holds task to self_t mapping for poll plugin */
typedef struct {
#ifdef __linux__
    fd_src_t f;
#endif
    m_src_task_t tid;
    pthread_t th;
    int retval;
} task_src_t;

/* Struct that holds thresh to self_t mapping for poll plugin */
typedef struct {
#ifdef __linux__
    fd_src_t f;
#endif
    m_src_thresh_t thr;
    m_src_thresh_t alarm;
} thresh_src_t;

/* Struct that holds pubsub subscriptions source data */
typedef struct {
    regex_t reg;
    const char *topic;
} ps_src_t;

typedef struct _ev_src *(*process_cb)(struct _ev_src *this, m_ctx_t *c, int idx, evt_priv_t *evt);

/* Struct that holds generic "event source" data */
typedef struct _ev_src {
    union {
        ps_src_t     ps_src;
        fd_src_t     fd_src;
        tmr_src_t    tmr_src;
        sgn_src_t    sgn_src;
        path_src_t   path_src;
        pid_src_t    pid_src;
        task_src_t   task_src;
        thresh_src_t thresh_src;
    };
    m_src_types type;
    m_src_flags flags;
    void *ev;                               // poll plugin defined data structure
    m_mod_t *mod;                           // ptr needed to map an event source to a module in poll_plugin
    const void *userptr;
    process_cb process; // Processors can update the src, that's why they return an ev_src_t (PS only)
} ev_src_t;

/* Struct that holds pubsub messaging, private */
typedef struct {
    m_evt_ps_t msg;
    m_ps_flags flags;
    ev_src_t *sub;
} ps_priv_t;

extern const char *src_names[];
int init_src(m_mod_t *mod, m_src_types t);
int register_mod_src(m_mod_t *mod, m_src_types type, const void *src_data,
                 m_src_flags flags, const void *userptr);
int deregister_mod_src(m_mod_t *mod, m_src_types type, void *src_data);
ev_src_t *register_ctx_src(m_ctx_t *c, m_src_types type, process_cb proc, const void *src_data);
int deregister_ctx_src(m_ctx_t *c, ev_src_t **src);
int start_task(m_ctx_t *c, ev_src_t *src);
