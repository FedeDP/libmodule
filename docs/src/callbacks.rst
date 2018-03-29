.. |br| raw:: html

   <br />

Module callbacks
================

Every module needs 5 functions that must be defined by developer. |br|
If using :ref:`module_easy`, they are automatically declared by MODULE macro. |br|
Moreover, a module_pre_start function is declared too, but it is not needed by libmodule interface, ie: it can be left undefined. Your compiler may warn you about that though.

.. code::

    static void module_pre_start(void);
    static int init(void);
    static int check(void);
    static int evaluate(void);
    static void receive(const msg_t *msg, const void *userdata);
    static void destroy(void);

.. c:function:: module_pre_start(void)

  This function will be called before any module is registered. |br|
  It is the per-module version of :ref:`modules_pre_start <modules_pre_start>` function.

.. c:function:: init(void)

  Initializes module state and returns an fd to be polled. |br|
  A special fd value MODULE_DONT_POLL is defined, to create a non-pollable module. |br|
  Non-pollable module acts much more similar to an actor, ie: they can only receive and send PubSub messages.
  
  :returns: FD to be polled for this module.

.. c:function:: check(void)

  Startup filter to check whether this module should be created and managed by libmodule, |br|
  as sometimes you may wish that not your modules are automatically started.
  
  :returns: true (not-0) if the module should be created, 0 otherwise.

.. c:function:: evaluate(void)

  Similar to check() function but at runtime: this function is called for each IDLE module after evey state machine update
  and it should check whether a module is now ready to be start (ie: init should be called on this module).
  
  :returns: true (not-0) if module is now ready to be started, else 0.
  
.. c:function:: receive(msg, userdata)

  Poll callback, called when any event is ready on module's fd or when a PubSub message is received by a module.
  
  :param: :c:type:`const msg_t *` msg: pointer to msg_t struct.
  :param: :c:type:`const void *` userdata: pointer to userdata as set by m_set_userdata.

.. c:function:: destroy(void)

  Destroys module, called automatically at module deregistration. Please note that module's fd is automatically closed.
