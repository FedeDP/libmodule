.. |br| raw:: html

   <br />

Concepts
========

Module
------

A module is core entity of libmodule: it is a single and indipendent logical unit that reacts to certain events, both pubsub and socket ones. |br|
It can be seen as an Actor with the power of managing socket events. |br|
It offers some callbacks that are used by libmodule to manage its life. |br|
It is initialized through MODULE macro:
   
.. code::
    
    MODULE("test")
    
This macro creates a "test" module. |br|
MODULE macro also creates a constructor and destructor that are automatically called by libmodule at start and at end of program. |br|
Finally, this macro declares all of needed callbacks and returns an opaque handler for the module, that will be transparently passed with each call to libmodule API while using :ref:`module_easy`. |br|

Context
-------

A context is a way to create subnets of modules. You can loop on events from each context, and each context behaves independently from others. |br| 
This can be particularly useful when dealing with 2+ threads; ideally, each thread has its own module's context and thus its own events to be polled. |br|
A context is automatically created for you first time a module that binds on that context is registered; so, multi-context API is very similar to single context one. |br|
To initialize a module binding it to its context, use MODULE_CTX macro:
   
.. code::
    
    MODULE_CTX("test", "myCtx")
    
This macro firstly creates a "myCtx" context, then a "test" module using same MODULE macro as before. |br|
Indeed, MODULE macro is only a particular case of MODULE_CTX macro, where myCtx is automatically setted to "default". |br|
This makes sense, as you can expect: single context API is a multi context API with only 1 context. |br|
Modules can only see and reach (through PubSub messaging) other modules from same context. |br|

Loop
----

Libmodule offers an internal loop, started with modules_ctx_loop(); note that each context has its own loop. |br|
Moreover, you can even easily integrate it into your own loop: modules_ctx_get_fd() will retrieve a pollable fd and POLLIN events will be raised whenever a new message is available. |br|
Remember that before starting your loop, modules_ctx_dispatch() should be called, to dispatch initial "LoopStarted" messages to each module. |br|
Then, whenever POLLIN data is available on libmodule's fd, you only need to call modules_ctx_dispatch() again. |br|
Finally, remember to close() libmodule's fd retrieved through modules_ctx_get_fd(). |br|
