.. |br| raw:: html

   <br />
   
Modules
===========

Modules API denotes all of libmodule interface functions whose name starts with \modules_. |br|
Like Module API, it has an easy, single-context API. Moreover, it has an equivalent multi-context API. |br|
All these functions but modules_pre_start() return a :ref:`module_ret_code <module_ret_code>`.

Modules easy API
----------------
Modules easy API should be used in conjunction with :ref:`module_easy`. |br|
It abstracts all of libmodule internals mechanisms to provide an easy-to-use and simple API.

.. _modules_pre_start:

.. c:function:: modules_pre_start(void)

  This function will be called by libmodule before creating any module.
  It can be useful to set some global state/read config that are needed to decide whether to start a module.
  You only need to define this function and it will be automatically called by libmodule.
  
.. c:macro:: modules_loop(void)

  Start looping on events from modules
  
.. c:macro:: modules_quit(void)

  Leave libmodule's events loop
  
Modules multi-context API
-------------------------
Modules multi-context API let you manage your contexts in a very simple way. |br|
It exposes very similar functions to single-context API (again, single-context is only a particular case of multi-context), that now take a "context_name" parameter.
  
.. c:function:: modules_ctx_loop(ctx_name)

  Start looping on events from modules
  
  :param ctx_name: context name.
  :type ctx_name: :c:type:`const char *`
  
.. c:function:: modules_ctx_quit(ctx_name)

  Leave libmodule's events loop
  
  :param ctx_name: context name.
  :type ctx_name: :c:type:`const char *`
