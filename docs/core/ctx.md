# Libmodule core/ctx API

Ctx API denotes libmodule interface functions to manage contexts.  
It can be found under `<module/ctx.h>`.  

> NOTE: there is no context handler visible to user, because the handler is basically the thread itself.
> Trying to use the context API on a thread that has no context associated, will promptly return -EPIPE errno code.

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
int m_ctx_register(const char *ctx_name, m_ctx_flags flags, const void *userdata);
```
> Register a new ctx and associates it to current thread.  

**Params:**  

* `ctx_name`: name of the new ctx  
* `flags`: flags of the newly created ctx  
* `userdata`: user pointer stored in the ctx  

```C
int m_ctx_deregister();
```
> Deregister the ctx associated with the current thread.  
> NOTE: this API cannot be called if the ctx is still looping.  
> Make sure to `m_ctx_quit` the loop before deregistering a context.  
> NOTE: unless a ctx is registered with `M_CTX_PERSIST` flag, it will get  
> automatically destroyed when there are no more modules registered in it.  
> NOTE: all of the modules in the context will be deregistered  
> when their context gets deregistered.  

```C
int m_ctx_set_logger(m_log_cb logger);
```
> Set a logger callback; otherwise, a default one is used.  

**Params:**  

* `logger`: new logger callback to be set  

```C
int m_ctx_loop(void);
```
> Loop a ctx in a blocking manner, until `m_ctx_quit` is called by any module.  
> NOTE: stopping a ctx is a blocking action:  
> all present events will be flushed to their modules,  
> and, in case any `M_SRC_TYPE_TASK` src is enabled,  
> its thread will be joined for a clean exit.  

**Returns:** `m_ctx_quit` quit_code.  

```C
int m_ctx_quit(uint8_t quit_code);
```
> Quit a ctx loop, returning given exit code.  

**Params:**  

* `quit_code`: quit value  

```C
int m_ctx_fd(void);
```
> Retrieve ctx pollable file descriptor.  

**Returns:** ctx fd.

```C
int m_ctx_dispatch(void);
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

**Returns:** first time it is called: errno-style negative error code.  
Subsequent calls: number of messages dispatched.  
Last time (after a `m_ctx_quit` call): errno-style negative error code.  

```C
int m_ctx_dump(void);
```
> Dump a json of the current ctx state.  

```C
int m_ctx_stats(m_ctx_stats_t *stats);
```
> Retrieve stats about current ctx state.  

```C
const char *m_ctx_name(void);
```
> Retrieve ctx name.  

```C
const void *m_ctx_userdata(void);
```
> Retrieve ctx userdata as set at registration time.  

```C
ssize_t m_ctx_len(void);
```
> Retrieve number of registered modules.  

```C
int m_ctx_finalize(void);
```
> Set the context as finalized, denying any subsequent module registeration.  

```C
int m_ctx_set_tick(uint64_t ns);
```
> Set a context tick. You can subscribe modules to `M_PS_CTX_TICK` system topic to receive tick events.

**Params:**  

* `ns`: nanoseconds for the tick  

### Only when built with `WITH_FS` build option

```C
int m_ctx_fs_set_root(const char *path);
```
> Set the context FS root. Must be set before the ctx loop is started.  

**Params:**  

* `path`: FS root path. NULL to disable FUSE fs.  

```C
int m_ctx_fs_set_ops(const struct fuse_operations *ops);
```
> Set specified FUSE operations to context. Must be set before the ctx loop is started.  
> NOTE: module files will always be created readonly.  

**Params:**  

* `ops`: fuse operations. NULL to reset default ops.  
