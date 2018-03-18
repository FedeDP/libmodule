.. |br| raw:: html

   <br />

Module callbacks
================

Every module needs 5 functions that be must defined by developer. |br|
They are automatically declared by MODULE macro. |br|
Moreover, a module_pre_start function is declared too, but it is not needed by libmodule interface, ie: it can be left undefined.

.. code::
    static void module_pre_start(void);
    static int init(void);
    static int check(void);
    static int evaluate(void);
    static void recv(const msg_t *msg, const void *userdata);
    static void destroy(void);

.. c:function:: module_pre_start(void)

  It can be used to create a function that will be called before any module is registered. |br|
  It is the per-module version of :ref:`modules_pre_start <modules_pre_start>` function. |br|

.. c:function:: init(void)

  Initializes module state
  
  :returns: FD to be polled for this module.

.. c:function:: check(void)

  Startup filter to check whether this module should be created and managed by libmodule
  
  :returns: true (not-0) if the module should be created, 0 otherwise.

.. c:function:: evaluate(void)

  Similar to check() function but at runtime: this function is called for each IDLE module after evey state machine update
  and it should check whether a module is now ready to be start (ie: init should be called on this module).
  
  :returns: true (not-0) if module is now ready to be started, else 0.
  
.. c:function:: recv(msg, userdata)

  Poll callback
  
  :param: :c:type:`message_t *` msg: pointer to message_t struct. As of now, only fd field is used.
  :param: :c:type:`const void *` userdata: pointer to userdata as set by m_set_userdata.

.. c:function:: destroy(void)

  Destroys module, called automatically at module deregistration. Please note that module's fd is automatically closed.
