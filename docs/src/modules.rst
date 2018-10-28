.. |br| raw:: html

   <br />
   
Modules
=======

Modules API denotes libmodule interface functions that can be found in <module/modules.h> header. |br|
Like Module API, it has an easy, single-context API. Moreover, it has an equivalent multi-context API. |br|
All these functions but modules_pre_start() return a :ref:`module_ret_code <module_ret_code>`. |br|

Moreover, there is a single function that is common to every context (thus does not need ctx_name param). |br|

.. c:function:: modules_set_memalloc_hook(hook)

  Set memory management functions. By default: malloc, realloc, calloc and free are used.
  
  :param hook: new memory management hook.
  :type hook: :c:type:`memalloc_hook *`

Modules easy API
----------------
Modules easy API should be used in conjunction with :ref:`module_easy`. |br|
It abstracts all of libmodule internals mechanisms to provide an easy-to-use and simple API.

.. _modules_pre_start:

.. c:function:: modules_pre_start(void)

  This function will be called by libmodule before creating any module.
  It can be useful to set some global state/read config that are needed to decide whether to start a module.
  You only need to define this function and it will be automatically called by libmodule.
  
.. c:macro:: modules_set_logger(logger)

  Set a logger. By default, module's log prints to stdout. 
  
  :param logger: logger function.
  :type logger: :c:type:`log_cb`
  
.. c:macro:: modules_loop(void)

  Start looping on events from modules. Note that as soon as modules_loop is called, a message with type == SYSTEM will be broadcasted to all modules.
  
.. c:macro:: modules_quit(quit_code)

  Leave libmodule's events loop.
  
  :param quit_code: exit code that should be returned by modules_loop.
  :type quit_code: :c:type:`const uint8_t`
  
Modules multi-context API
-------------------------
Modules multi-context API let you manage your contexts in a very simple way. |br|
It exposes very similar functions to single-context API (again, single-context is only a particular case of multi-context), that now take a "context_name" parameter.
  
.. c:function:: modules_ctx_set_logger(ctx_name, logger)

  Set a logger for a context. By default, module's log prints to stdout.
  
  :param ctx_name: context name.
  :param logger: logger function.
  :type ctx_name: :c:type:`const char *`
  :type logger: :c:type:`log_cb`
  
.. c:macro:: modules_ctx_loop(ctx_name)

  Start looping on events from modules. Note that this is just a macro that calls modules_ctx_loop_events with MODULE_MAX_EVENTS (64) events.
  
  :param ctx_name: context name.
  :type ctx_name: :c:type:`const char *`
  
.. c:function:: modules_ctx_loop_events(ctx_name, maxevents)

  Start looping on events from modules, on at most maxevents events at the same time. Note that as soon as modules_ctx_loop_events is called, a message with type == SYSTEM will be broadcasted to all modules.
  
  :param ctx_name: context name.
  :param maxevents: max number of fds wakeup that will be managed at the same time.
  :type ctx_name: :c:type:`const char *`
  :type maxevents: :c:type:`int`
  
.. c:function:: modules_ctx_quit(ctx_name, quit_code)

  Leave libmodule's events loop.
  
  :param ctx_name: context name.
  :param quit_code: exit code that should be returned by modules_loop.
  :type ctx_name: :c:type:`const char *`
  :type quit_code: :c:type:`const uint8_t`

