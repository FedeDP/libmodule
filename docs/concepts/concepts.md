# Concepts

## Naming Conventions

Libmodule uses the following naming conventions for its public API:  

* `m_` is the prefix for all the library API  
* `m_foo_` APIs group together the same namespace API  
* struct types have the `_t` suffix  
* enum types **do not have** the `_t` suffix  
* `get`ters with no `set`ter, do not have the `get` prefix; for example: `m_ctx_userdata`.

## Logging

Libmodule offers an internal logging facility, to enable verbose logging for the whole library.  
There are 4 log levels that can be enabled through `LIBMODULE_LOG` env variable:  

* "err" (**default**)  
* "warn"  
* "info"  
* "debug"  

Fine-grained control over logging contexts is enabled through the use of specific env variables; following are the supported contexts:  

* `LIBMODULE_LOG_MEM`
* `LIBMODULE_LOG_STRUCTS`
* `LIBMODULE_LOG_THPOOL`
* `LIBMODULE_LOG_CORE`
* `LIBMODULE_LOG_OTHER`

Basically, you can set different logging level for each context; a default log level is set using the  
aforementioned `LIBMODULE_LOG` env variable (or **err** only by default).

Moreover, you can specify an output file for the log, by passing `LIBMODULE_LOG_OUTPUT` env variable.  
By default, stdout/stderr are used.

## Memory

### Constructors

Libmodule makes heavy usage of gcc `__attribute__((constructor))` (and destructor) to inizialize itself.  
Ctor order is specified in each namespace doc.  

### Memhook

Moreover, libmodule allows users to override default memhook used, by calling `m_set_memhook()`.  
This function must be called from `m_pre_start()` function, because it has to be called before any internal ctor is run, ie: before any allocation takes place.  
A memhook is just a wrapper around 3 main memory related functions:  

* `malloc`  
* `calloc`  
* `free`  

