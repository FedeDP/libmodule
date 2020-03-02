.. |br| raw:: html

   <br />

Lifecycle
=========

Easy API
--------

We previously saw that every module has to expose 5 functions. |br|
We will now go deeper, better understanding a module's lifecycle. |br|
First of all, module lifecycle is automatically managed by libmodule; moreover, when using :ref:`module_easy`,
module registration/deregistration is completely automated and transparent to developer. |br|
This means that when using easy API (or multicontext API), you will only have to declare a source file as a module and define needed functions. |br|

Before any module is registered into libmodule, each module's module_pre_start() function is called. |br|
This function can be useful to set each module pre-starting state: this may be needed if any check() function depends on some global state (eg: as read by config file). |br|

After every module_pre_start() function is called, libmodule will start checking which module needs to be registered, eventually registering them. |br|
For every module, check() function will be called; if and only if that function returns true, the module will be registered. |br|
An unregistered module is just dead code; none of its functions will ever be called. |br|
Once a module is registered, it will be initially set to an IDLE state. Idle means that the module is not started yet, thus it cannot receive any PubSub msg nor any event from fds. |br|

As soon as its context starts looping, a module's evaluate() function will be called on every IDLE module, trying to start it right away. |br|
Evaluate() will be then called at each state machine change, for each idle module. |br|

As soon as module's evaluate() returns true, the module is started. It means its init() function is finally called and its state is set to RUNNING. |br|
A single module can poll on multiple fds: just call module_register_fd() multiple times. |br|
When a module reaches RUNNING state, modules_loop()/modules_ctx_loop() functions will finally receive events from its fds. |br|

Whenever an event triggers on a module's fd, or the module receives a PubSub message from another one, its receive() callback is called. |br|
Receive callback will receive userdata too as parameter, as set by module_set_userdata().

Finally, when leaving program, each module's destroy() function is automatically called during the process of automatic module's deregistration. |br|

Complex API
-----------

When dealing with libmodule's :ref:`module_complex`, no modules is automatically started for you, ie: you must manually call module_register()/module_deregister() on each module. |br|
When using complex API, you are responsible to register/deregister modules, and thus initing/destroying them. |br|
Note that with Complex API, module_pre_start() function is not available (it would be useless), and check(), evaluate() and destroy() functions are no more mandatory. |br|
You will still have to define init() and receive() functions (but you can freely name them!). |br|

Everything else but module's (de)registration is same as Easy API.

Module States
-------------

As previously mentioned, a registered module, before being started, is in IDLE state. |br|
IDLE state means that it has still no source of events; it won't receive any PubSub message and even if it registers any fd, they won't be polled. |br|
When module is started, thus reaching RUNNING state, all its registered fds will start being polled; moreover, it can finally receive PubSub messages. Fds registered while in RUNNING state, are automatically polled. |br|
If a module is PAUSED, it will stop polling on its fds and PubSub messages, but PubSub messages will still be queued on its write end of pipe. Thus, as soon as module is resumed, all PubSub messages received during PAUSED state will trigger receive() callback. |br|
If a module gets stopped, it will stop polling on its fds and PubSub messages, and every autoclose fd will be closed. Moreover, all its registered fds are freed and its enqueued pubsub messages are destroyed. Its state is then set back to IDLE. |br|
If you instead wish to stop a module letting it manage any already-enqueued pubsub message, you need to send a POISONPILL message to it, through module_poisonpill() function. |br|

module_start() needs to be called on an IDLE module. |br|
module_pause() needs to be called on a RUNNING module. |br|
module_resume() needs to  be called on a PAUSED module. |br|
module_stop() needs to be called on a RUNNING or PAUSED module.
