.. |br| raw:: html

   <br />

Module Lifecycle
================

Easy API
--------

We previously saw that every module has to expose at least 5 functions. |br|
We will now go deeper, better understanding a module's lifecycle. |br|
First of all, module lifecycle is automatically managed by libmodule; moreover, when using :ref:`module_easy`,
module registration/deregistration is completely automated and transparent to developer. |br|
This means that when using easy API (or multicontext API), you will only have to declare a source file as a module and define needed functions. |br|

Before any module is registered into libmodule, each module's module_pre_start() function is called. |br|
This function can be useful to set each module pre-starting state: this may be needed if any check() function depends on another module resource. |br|

After every module_pre_start() function is called, libmodule will start checking which module needs to be registered, eventually registering them. |br|
For every module, check() function will be called; if and only if that function returns a TRUE value (ie: non-0 return code), the module will be registered. |br|
An unregistered module is just dead code; none of its functions will ever be called. |br|
Once a module is registered, it will be initially set to an IDLE state. Idle means that the module is not started yet, ie: it must still set a proper fd to be polled. |br|

Right after module's registration, its evaluate() function will be called, trying to start the module right after it was registered. |br|
Evaluate will be then called at each state machine change, for each idle module. |br|

As soon as module's evaluate() returns TRUE, the module is started. It means its init() function is finally called and, if a proper FD or the special MODULE_DONT_POLL value is returned,
its state is set to STARTED. |br|
When a module reaches started state, modules_loop()/modules_ctx_loop() functions will finally receive events from its fd. |br|

Whenever an event triggers on a module's fd, or the module receives a PubSub message from another one, its recv() callback is called. |br|

Finally, after a modules_quit()/modules_ctx_quit(), each module's destroy() function is automatically called, during the process of module's deregistration. |br|

Complex API
-----------

When dealing with libmodule's :ref:`module_complex`, no modules is automatically started for you, ie: you must manually call module_register()/module_deregister() on each module. |br|
While this may seem useless, i am sure there can be some cases where you may wish to register/deregister modules yourself. |br|
When using complex API, you are responsible to register/deregister modules, and thus initing/destroying them. |br|
Note that with Complex API, module_pre_start() function is not available (it would be useless), and you won't need to define check() function. |br|
You will still have to define evaluate(), init(), recv() and destroy() functions (but you can freely name them!). |br|

Everything else but module's (de)registration is same as Easy API.

