# Libmodule core/ctx API

Ctx API denotes libmodule interface functions to manage contexts.  
It can be found under `<module/ctx.h>`.  

> **All the ctx API expects a non-NULL ctx handler**, except for `m_ctx_register` function.  

## Types

```C
typedef enum {
    M_CTX_NAME_DUP          = 1 << 0,         // Should ctx's name be strdupped
    M_CTX_NAME_AUTOFREE     = 1 << 1,         // Should ctx's name be autofreed
    M_CTX_PERSIST           = 1 << 2,         // Prevent ctx automatic destroying when there are no modules in it anymore. With this option, context is kept alive until m_ctx_deregister() is called.
    M_CTX_USERDATA_AUTOFREE = 1 << 3          // Automatically free ctx userdata upon deregister
} m_ctx_flags;
```
> Bitmask of flags that can be passed to `m_ctx_register` function

```C
typedef struct {
    double activity_freq;
    uint64_t total_idle_time;
    uint64_t total_looping_time;
    uint64_t recv_msgs;
    size_t num_modules;
    size_t running_modules;
} m_ctx_stats_t;
```
> Returned stats about a context by `m_ctx_stats` function

```C
typedef void (*m_log_cb)(const m_mod_t *ref, const char *fmt, va_list args);
```
> Log callback set through `m_ctx_set_logger` function

## Functions

```C
int m_ctx_register(const char *ctx_name, m_ctx_t **c, m_ctx_flags flags, const void *userdata);
```
> Register a new ctx.  
> **Params:**
> * `ctx_name`: name of the new ctx
> * `c`: ctx handler storage
> * `flags`: flags of the newly created ctx
> * `userdata`: user pointer stored in the ctx

```C
int m_ctx_deregister(m_ctx_t **c);
```
> Deregister a ctx.  
> **Params:**
> * `c`: ctx handler storage, reset to NULL after the call

```C
m_ctx_t *m_ctx_ref(const char *ctx_name);
```
> Get a reference on a ctx, if existent.  
> NOTE: this API increments number of reference on ctx object;  
> remember to `m_mem_unref` the reference when you do not need it anymore.  
> **Params:**
> * `ctx_name`: context name
>
> **Returns:** a ctx handler or NULL.

```C
int m_ctx_set_logger(m_ctx_t *c, m_log_cb logger);
```
> Set a logger callback; otherwise, a default one is used.  
> **Params:**
> * `c`: ctx handler
> * `logger`: new logger callback to be set

```C
int m_ctx_loop(m_ctx_t *c);
```
> Loop a ctx in a blocking manner, until `m_ctx_quit` is called by any module.  
> NOTE: stopping a ctx is a blocking action:  
> all present events will be flushed to their modules,  
> and, in case any `M_SRC_TYPE_TASK` src is enabled,  
> its thread will be joined for a clean exit.  
> **Params:**
> * `c`: ctx handler
>
> **Returns:** `m_ctx_quit` quit_code.

```C
int m_ctx_quit(m_ctx_t *c, uint8_t quit_code);
```
> Quit a ctx loop, returning given exit code.  
> **Params:**
> * `c`: ctx handler
> * `quit_code`: quit value


```C
int m_ctx_fd(const m_ctx_t *c);
```
> Retrieve ctx pollable file descriptor.  
> **Params:**
> * `c`: ctx handler
>
> **Returns:** ctx fd.

```C
int m_ctx_dispatch(m_ctx_t *c);
```
> Dispatch events from the context.  
> Useful when ctx fd is embedded in another loop.  
> First time it gets called, it will start the loop;  
> then, consecutive calls will dispatch ctx events.  
> Finally, after `m_ctx_quit` has been called,  
> it will notify the ctx to stop.  
> NOTE: stopping a ctx is a blocking action:
> all present events will be flushed to their modules,
> and, in case any `M_SRC_TYPE_TASK` src is enabled,
> it will join its thread.
> **Params:**
> * `c`: ctx handler
>
> **Returns:** first time it is called: errno-style negative error code.  
> Subsequent calls: number of messages dispatched.  
> Last time (after a `m_ctx_quit` call): errno-style negative error code.  

```C
int m_ctx_dump(const m_ctx_t *c);
```
> Dump a json of the current ctx state.  
> **Params:**
> * `c`: ctx handler

```C
int m_ctx_stats(const m_ctx_t *c, m_ctx_stats_t *stats);
```
> Retrieve stats about current ctx state.  
> **Params:**
> * `c`: ctx handler

```C
const char *m_ctx_name(const m_ctx_t *c);
```
> Retrieve ctx name.  
> **Params:**
> * `c`: ctx handler

```C
const void *m_ctx_userdata(const m_ctx_t *c);
```
> Retrieve ctx userdata.  
> **Params:**
> * `c`: ctx handler

```C
ssize_t m_ctx_len(const m_ctx_t *c);
```
> Retrieve number of registered modules.  
> **Params:**
> * `c`: ctx handler

```C
int m_ctx_finalize(m_ctx_t *c);
```
> Set the context as finalized, denying any subsequent module registeration.  
> **Params:**
> * `c`: ctx handler

When built with `WITH_FS` enabled, ctx API will expose 2 additional functions:  

```C
const char *m_ctx_fs_get_root(const m_ctx_t *c);
```
> Retrieve the context FS root.  
> **Params:**
> * `c`: ctx handler

```C
int m_ctx_fs_set_root(m_ctx_t *c, const char *path);
```
> Set the context FS root. Must be set before the ctx loop is started.
> **Params:**
> * `c`: ctx handler
> * `path`: FS root path
