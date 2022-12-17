# Libmodule core API

Libmodule core API denotes the set of symbols exposed by `libmodule_core.so` library, whose headers are installed in `$includedir/module/`.  

It is made of multiple headers:  

* `mod.h` that contains module related API  
* `mod_easy.h` that contains an helper macro to build simplest applications, ie: single context, single module for each source file  
* `ctx.h` that contains context related API  
* `fs.h` (installed only if `WITH_FS` cmake option is enabled) that contains libmodule fuseFS related API  
* `cmn.h` that contains some common symbols, and cannot be manually included  

For the sake of readiness, function params where an output value will be stored, are marked with `OUT` (empty) macro.  

> **All the libmodule core API returns an errno-style negative error code, where left unspecified.**

## Constructors

Libmodule makes heavy usage of gcc `__attribute__((constructor))` (and destructor) to inizialize itself.  
The order is the following:

**Ctors**  

* `m_on_boot()`
* internal `libmodule_init()`
* each `m_mod_on_boot()` (only mod_easy API)
* each `m_mod_ctor()` (only mod_easy API)

**Dtors**  

* each `m_mod_dtor()` (only mod_easy API)
* internal `libmodule_deinit()`

## Memory

### Ref counted

Ideally, all of the exposed pointers have their lifetime reference based.  
This means that you can call `m_mem_ref()` API to manage eg: `m_mod_t`, `m_ctx_t`, `m_evt_t` pointers, and so on.  
Normally, there is no such need because the library manages everything.  
But if you called `m_mod_ref()`, then you own a reference on that module and it's up to you to free the reference by calling `m_mem_unref()` on it.  
