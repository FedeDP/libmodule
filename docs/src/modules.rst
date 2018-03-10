.. |br| raw:: html

   <br />
   
Modules
===========

Modules API denotes all of libmodule interface functions whose name starts with \modules_. |br|
Like Module API, it has an easy, single-context API, and its equivalent multi-context API.

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
    
  :param: :c:type:`error_cb` on_error: user error callback.
  
.. c:macro:: modules_loop(void)

  Start looping on events from modules
  
.. c:macro:: modules_quit(void)

  Leave libmodule's events loop

