# Context

A context can be seen as a collector for modules. You can loop on events from each context, and each context behaves independently from others.  
This can be particularly useful when dealing with 2+ threads; ideally, each thread has its own module's context and thus its own events to be polled.  
Modules can only see and reach (through PubSub messaging) other modules from same context.  

## Loop

Libmodule offers an internal loop, started with `m_ctx_loop()`; note that each context has its own loop.  
Moreover, you can even easily integrate it into your own loop: `m_ctx_get_fd()` will retrieve a pollable fd and POLLIN events will be raised whenever a new message is available.  
Remember that before starting your loop, `m_ctx_dispatch()` should be called, to dispatch initial "LoopStarted" messages to each module.  
Then, whenever POLLIN data is available on libmodule's fd, you only need to call m_ctx_dispatch() again.  
Finally, remember to close() libmodule's fd retrieved through `m_ctx_get_fd()`.  

## Default main

Libmodule offers a single-context looping main as a weak, thus overridable, symbol.  
This means that developers must not even create a main for simple applications.  

Default main is mostly useful in conjunction with `<module/mod_easy>` API.  
It offers 2 functions that can be implemented by caller:

* `m_ctx_pre_loop(m_ctx_t *c, int argc, char *argv[])` that is called before the ctx loop is started
* `m_ctx_post_loop(m_ctx_t *c, int argc, char *argv[])` that is called after the loop stopped, right before leaving  

