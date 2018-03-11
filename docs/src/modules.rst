.. |br| raw:: html

   <br />
   
Modules
===========

Modules API denotes all of libmodule interface functions whose name starts with \modules_. |br|
Like Module API, it has an easy, single-context API. Moreover, it has an equivalent multi-context API.

Modules easy API
----------------
Modules easy API should be used in conjunction with :ref:`module_easy`. |br|
It abstracts all of libmodule internals mechanisms to provide an easy-to-use and simple API.

.. c:macro:: modules_pre_start(void)

  This function will be called by libmodule before creating any module.
  It can be useful to set some global state/read config that are needed to decide whether to start a module.
  You only need to define this function and it will be automatically called by libmodule.

.. c:macro:: modules_on_error(on_error)

  Set libmodule error's callback
    
  :param on_error: user error callback.
  :type on_error: :c:type:`error_cb` 
  
.. c:macro:: modules_loop(void)

  Start looping on events from modules
  :returns: MOD_OK if no error happened, MOD_ERR if any error happened.
  
.. c:macro:: modules_quit(void)

  Leave libmodule's events loop

Modules multi-context API
-------------------------
Modules multi-context API let you manage your contexts in a very simple way. |br|
It exposes very similar functions to single-context API (again, single-context is only a particular case of multi-context), that now take a "context_name" parameter.

.. c:function:: modules_ctx_on_error(ctx_name, on_error)

  Set libmodule error's callback

  :param ctx_name: context name.
  :param on_error: user error callback.
  :type ctx_name: :c:type:`const char *`
  :type on_error: :c:type:`error_cb`
  
.. c:function:: modules_ctx_loop(ctx_name)

  Start looping on events from modules
  
  :param ctx_name: context name.
  :type ctx_name: :c:type:`const char *`
  :returns: MOD_OK if no error happened, MOD_ERR if any error happened.
  
.. c:function:: modules_ctx_quit(ctx_name)

  Leave libmodule's events loop
  
  :param ctx_name: context name.
  :type ctx_name: :c:type:`const char *`
