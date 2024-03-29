# Context

A context can be seen as a collector for modules. You can loop on events from each context, and each context behaves independently from others.  
It is stored as a [thread specific object](https://linux.die.net/man/3/pthread_setspecific), created through `m_ctx_register` and deleted through `m_ctx_deregister`.  
This can be particularly useful when dealing with 2+ threads; each thread has its own module's context and thus its own events to be polled.  
Modules can only see and reach (through PubSub messaging) other modules from same context.  
A context is given a name at registration time. This is only useful for logging purposes.  

> NOTE: having multiple contexts with same name is allowed; given that each context is thread-specific, there will be no clash.  
> Of course, it's better to set different names.  

## Loop

Libmodule offers an internal loop, started with `m_ctx_loop()`; note that each context has its own loop.  
Moreover, you can even easily integrate it into your own loop: `m_ctx_fd()` will retrieve a pollable fd and POLLIN events will be raised whenever a new message is available.  
Remember that before starting your loop, `m_ctx_dispatch()` should be called, to dispatch initial "LoopStarted" messages to each module.  
Then, whenever POLLIN data is available on libmodule's fd, you only need to call `m_ctx_dispatch()` again.  
Finally, remember to `close()` libmodule's fd retrieved through `m_ctx_fd()`.  

## Default main

Libmodule offers a single-context looping main as a weak, thus overridable, symbol.  
This means that developers must not even create a main for simple applications.  

Default main is mostly useful in conjunction with `<module/mod_easy>` API.  
It offers 2 functions that can be implemented by caller:

* `m_ctx_pre_loop(m_ctx_t *c, int argc, char *argv[])` that is called before the ctx loop is started
* `m_ctx_post_loop(m_ctx_t *c, int argc, char *argv[])` that is called after the loop stopped, right before leaving

## FS

When built with `WITH_FS` enabled, ctx API will expose 2 additional functions:  
```C
int m_ctx_fs_set_root(m_ctx_t *c, const char *path);
int m_ctx_fs_set_ops(m_ctx_t *c, const struct fuse_operations *ops);
```
By setting a context root path, a ctx will expose its structure as a fuse FS, where each module inside the context is a read-only file.  
one can then perform multiple read operations on the generated directory tree:  

* `open` and then `poll` module files to get notified whenever a message is received by the module
* Read (`cat`) module files to get a nice overview of the module state (same as `m_mod_dump`)
* Perform a couple of read IOCTLs on a module:
* * `M_MOD_FS_STATE` to get a module state
* * `M_MOD_FS_STATS` to get module stats

The FS feature is specially useful to debug issues with module,  
because it allows a simple runtime inspection on a context and its modules state.
