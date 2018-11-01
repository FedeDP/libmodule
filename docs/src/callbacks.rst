.. |br| raw:: html

   <br />

Module callbacks
================

Every module needs 5 functions that must be defined by developer. |br|
If using :ref:`module_easy`, they are automatically declared by MODULE macro. |br|
Moreover, a module_pre_start function is declared too, but it is not needed by libmodule interface, ie: it can be left undefined. Your compiler may warn you about that though.

.. code::

    static void module_pre_start(void);
    static void init(void);
    static bool check(void);
    static bool evaluate(void);
    static void receive(const msg_t *const msg, const void *userdata);
    static void destroy(void);

.. c:function:: module_pre_start(void)

  This function will be called before any module is registered. |br|
  It is the per-module version of :ref:`modules_pre_start <modules_pre_start>` function.

.. c:function:: init(void)

  Initializes module state; useful to call module_register_fd() for each fd this module should listen to. |br|
  To create a non-pollable module, just avoid registering any fd. |br|
  Non-pollable module acts much more similar to an actor, ie: they can only receive and send PubSub messages. |br|
  Moreover this is the right place to eventually register any topic.

.. c:function:: check(void)

  Startup filter to check whether this module should be registered and managed by libmodule, |br|
  as sometimes you may wish that not your modules are automatically started.
  
  :returns: true if the module should be registered, false otherwise.

.. c:function:: evaluate(void)

  Similar to check() function but at runtime: this function is called for each IDLE module after evey state machine update
  and it should check whether a module is now ready to be start (ie: init should be called on this module).
  
  :returns: true if module is now ready to be started, else false.
  
.. c:function:: receive(msg, userdata)

  Poll callback, called when any event is ready on module's fd or when a PubSub message is received by a module. |br|
  Use msg->is_pubsub to decide which internal message should be read (ie: pubsub_msg_t or fd_msg_t).
  
  :param: :c:type:`const msg_t * const` msg: pointer to msg_t struct.
  :param: :c:type:`const void *` userdata: pointer to userdata as set by m_set_userdata.

.. c:function:: destroy(void)

  Destroys module, called automatically at module deregistration. Please note that module's fds set as autoclose will be closed.
