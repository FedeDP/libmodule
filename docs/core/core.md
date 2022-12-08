# Libmodule core API

Libmodule core API denotes the set of symbols exposed by `libmodule_core.so` objects, whose headers are installed in $includedir/module/.  

It is made of various headers:  

* `mod.h` that contains module related API  
* `mod_easy.h` that contains an helper macro to build simplest applications, ie: single context, single module for source file  
* `ctx.h` that contains context related API  
* `fs.h` (installed only if `WITH_FS` cmake option is enabled) that contains libmodule fuseFS related API  
* `cmn.h` that contains some common symbols, and cannot be manually included  
* `plugin.h` that contains libmodule plugin API  
* `plugin_C.h` that contains a small C sdk (ie: a macro) to build a C plugin  
