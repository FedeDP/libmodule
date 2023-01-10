# Libmodule core/mod API

Mod API denotes libmodule interface functions to manage modules.  
It can be found in `<module/mod.h>` header.  

> **NOTE:** all the mod API expects a non-NULL mod handler, except for `m_mod_register` function.

## Types

```C
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
```
> List of available event source types

```C
typedef enum {
    M_SRC_PRIO_LOW        =       1 << 0,  // Src low priority
    M_SRC_PRIO_NORM       =       1 << 1,  // Src mid priority (default)
    M_SRC_PRIO_HIGH       =       1 << 2,  // Src high priority
    M_SRC_AUTOFREE        =       1 << 3,  // Automatically free userdata upon source deregistation.
    M_SRC_ONESHOT         =       1 << 4,  // Run just once then automatically deregister source.
    M_SRC_DUP             =       1 << 5,  // Duplicate PubSub topic, source fd or source path.
    M_SRC_FD_AUTOCLOSE    =       1 << 16, // Automatically close fd upon deregistation.
    M_SRC_TMR_ABSOLUTE    =       1 << 24, // Absolute timer
} m_src_flags;
```
> Bitmask of flags related to event source registration

```C
typedef enum {
    M_PS_AUTOFREE   = 1 << 0,     // Autofree PubSub data after every recipient receives message (ie: when ps_evt ref counter goes to 0)
} m_ps_flags;
```
> Flags available when sending PubSub messages through m_mod_ps API

```C
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
```
> Libmodule input src types for m_mod_src_register() API

```C
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
```
> Event types received inside `m_mod_on_evt()` callback queue

```C
typedef struct {
    bool system;            // Is this a system message?
    const m_mod_t *sender;
    const char *topic;
    const void *data;       // NULL for system messages
} m_evt_ps_t;

typedef struct {
    int fd;
} m_evt_fd_t;

typedef struct {
    uint64_t ns;
} m_evt_tmr_t;

typedef struct {
    unsigned int signo;
} m_evt_sgn_t;

typedef struct {
    const char *path;
    unsigned int events;
} m_evt_path_t;

typedef struct {
    pid_t pid;
    unsigned int events;
} m_evt_pid_t;

typedef struct {
    unsigned int tid;
    int retval;
} m_evt_task_t;

typedef struct {
    unsigned int id;
    uint64_t inactive_ms;
    double activity_freq;
} m_evt_thresh_t;
```
> Event types stored inside m_evt_t

```C
typedef enum {
    M_MOD_IDLE = 1 << 0,
    M_MOD_RUNNING = 1 << 1,
    M_MOD_PAUSED = 1 << 2,
    M_MOD_STOPPED = 1 << 3,
    M_MOD_ZOMBIE = 1 << 4
} m_mod_states;
```
> Bitmask of possible module states

```C
typedef enum {
    M_MOD_NAME_DUP          = 1 << 0,         // Should module's name be strdupped? (force M_MOD_NAME_AUTOFREE flag)
    M_MOD_NAME_AUTOFREE     = 1 << 1,         // Should module's name be autofreed?
    M_MOD_ALLOW_REPLACE     = 1 << 8,         // Can module be replaced by another module with same name?
    M_MOD_PERSIST           = 1 << 9,         // Module cannot be deregistered by direct call to m_mod_deregister (or by FS delete) while its context is looping
    M_MOD_USERDATA_AUTOFREE = 1 << 10,        // Automatically free module userdata upon deregister
    M_MOD_DENY_CTX          = 1 << 16,        // Deny access to module's ctx through m_mod_ctx() (it means the module won't be able to call ctx API)
    M_MOD_DENY_PUB          = 1 << 17,        // Deny access to module's publishing functions: m_mod_ps_{tell,publish,broadcast,poisonpill}
    M_MOD_DENY_SUB          = 1 << 18,        // Deny access to m_mod_ps_(un)subscribe()
} m_mod_flags;
```
> Bitmaks of module flags, passed at registration time

```C
typedef bool (*m_start_cb)(m_mod_t *self);
typedef bool (*m_eval_cb)(m_mod_t *self);
typedef void (*m_evt_cb)(m_mod_t *self, const m_queue_t *const evts);
typedef void (*m_stop_cb)(m_mod_t *self);
```
> Module callbacks types

```C
typedef struct {
    m_start_cb on_start;
    m_eval_cb on_eval;
    m_evt_cb on_evt;
    m_stop_cb on_stop;
} m_mod_hook_t;
```
> Userhook to hold callbacks from user

```C
typedef struct {
    uint64_t inactive_ms;
    double activity_freq;
    uint64_t sent_msgs;
    uint64_t recv_msgs;
} m_mod_stats_t;
```
> Hold stats about a module

## Functions

```C
int m_mod_register(const char *name, OUT m_mod_t **mod_ref, const m_mod_hook_t *hook,
                   m_mod_flags flags, const void *userdata);
```
> Register a new module in current thread context.  

**Params:**  

* `name`: name of the new module  
* `mod_ref`: when not NULL, store a ref to the newly creted module. The ref must be then `m_mem_unref` to free up the module
* `hook`: module userhook; when NULL, treat `name` as a path to `dlopen` a runtime-loaded module  
* `flags`: flags of the newly created module  
* `userdata`: user pointer stored inside the mod  

```C
int m_mod_deregister(OUT m_mod_t **mod);
```
> Deregister a module from current thread context.  

**Params:**  

* `mod`: pointer to module's handler. Set to NULL after return.

```C
const char *m_mod_name(const m_mod_t *mod);
```
> Retrieve a module's name

**Params:**  

* `mod`: module's handler  
