.. |br| raw:: html

   <br />

Libmodule Concepts
==================

Module concept
--------------

A module is core entity of libmodule: it is a single and indipendent logical unit that reacts to certain events by polling on a fd. |br|
It offers some callbacks that are used by libmodule to manage its life. |br|
It is initialized through MODULE macro:
   
.. code::
    
    MODULE("test")
    
This macro creates a "test" module. |br|
MODULE macro also creates a constructor and destructor that are automatically called by libmodule at start and at end of program. |br|
Finally, this macro declares all of needed callbacks and returns an opaque handler for the module, that will be transparently passed with each call to libmodule API while using :ref:`module_easy`. |br|

Context concept
---------------

A context is a way to create subnets of modules. You can loop on events from each context, and each context behaves independently from others. |br| 
This can be particularly useful when dealing with 2+ threads; ideally, each thread has its own module's context and thus its own events to be polled. |br|
A context is automatically created for you first time a module that binds on that context is registered; so, multi-context API is very similar to single context one. |br|
To initialize a module binding it to its context, use MODULE_CTX macro:
   
.. code::
    
    MODULE_CTX("test", "myCtx")
    
This macro firstly creates a "myCtx" context, then a "test" module using same MODULE macro as before. |br|
Indeed, MODULE macro is only a particular case of MODULE_CTX macro, where myCtx is automatically setted to "default". |br|
This makes sense, as you can expect: single context API is a multi context API with only 1 context. |br|
