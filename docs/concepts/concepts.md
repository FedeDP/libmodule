# Concepts

## Naming Conventions

Libmodule uses the following naming conventions:  

* `m_` is the prefix for all the library API  
* `m_foo_` APIs group together the same namespace API  
* struct types have the `_t` suffix  
* enum types **do not have** the `_t` suffix  

## Logging

Libmodule offers an internal logging facility, to enable verbose logging for the library.  
There are 4 log levels that can be enabled through `LIBMODULE_LOG` env variable:  

* 0 (error, **default**)  
* 1 (warn)  
* 2 (info)  
* 3 (debug)  

Moreover, you can specify an output file for the log, by passing `LIBMODULE_LOG_OUTPUT` env variable.  
By default, stdout is used.

## Memory

### Ref counted

Ideally, all of the exposed pointers have their lifetime reference based.  
This means that you can call `m_mem_ref()` API to manage eg: `m_mod_t`, `m_ctx_t`, `m_evt_t` pointers, and so on.  
Normally, there is no such need because the library manages everything.  
But if you called `m_mod_ref()`, then you own a reference on that module and it's up to you to free the reference by calling `m_mem_unref()` on it.  

### Memhook

Moreover, libmodule allows users to override default memhook used, by calling `m_set_memhook()`.  
This function must be called from `m_pre_start()` function, because it has to be called before any internal ctor is run, ie: before any allocation takes place.  
A memhook is just a wrapper around 3 main memory related functions:  

* `malloc`  
* `calloc`  
* `free`  
