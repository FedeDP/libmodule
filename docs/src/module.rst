.. |br| raw:: html

   <br />
   
Module
======

.. _module_easy:    

Module easy API
---------------

Module easy API consists of a single-context multi-modules set of macros. |br|
These macros make it easy and transparent to developer all of the module's internal mechanisms, providing a very simple way to use libmodule. |br|
It enforces correct modularity too: each module must have its own source file.

.. c:macro:: m_is(state)

  Check current module's state
    
  :param state: state we are interested in.
  :type state: :c:type:`enum module_states` 
  :returns: false (0) if module'state is not 'state', true (1) if it is and MOD_ERR on error.
  
.. c:macro:: m_start(fd)

  Start a module by polling on fd
    
  :param fd: fd to be polled.
  :type fd: :c:type:`int` 
  :returns: MOD_OK if no error happened, MOD_ERR if any error happened.
  
.. c:macro:: m_pause(void)

  Pause module's polling
    
  :returns: MOD_OK if no error happened, MOD_ERR if any error happened.
  
.. c:macro:: m_resume(void)

  Resume module's polling
    
  :returns: MOD_OK if no error happened, MOD_ERR if any error happened.
  
.. c:macro:: m_stop(void)

  Stop module's polling by closing its fd
    
  :returns: MOD_OK if no error happened, MOD_ERR if any error happened.
  
.. c:macro:: m_become(new_recv)

  Change recv callback to recv_new_recv
    
  :param new_recv: new module's recv; the function has suffix \recv_ concatenated with new_recv.
  :type new_recv: untyped
  :returns: MOD_OK if no error happened, MOD_ERR if any error happened.

.. c:macro:: m_unbecome(void)

  Reset to default recv poll callback
  
  :returns: MOD_OK if no error happened, MOD_ERR if any error happened.
  
.. c:macro:: m_set_userdata(userdata)

  Set userdata for this module; userdata will be passed as parameter to recv callback.
    
  :param userdata: module's new userdata.
  :type userdata: :c:type:`const void *`
  :returns: MOD_OK if no error happened, MOD_ERR if any error happened.

.. c:macro:: m_log(fmt, args)

  Logger for this module. Call it the same way you'd call printf
    
  :param fmt: log's format.
  :param args: variadic argument.
  :type fmt: :c:type:`const char *` 
  :type args: :c:type:`variadic`
  :returns: MOD_OK if no error happened, MOD_ERR if any error happened.

Module less-easy API
--------------------

Less-easy API consists of `Module easy API`_ internal functions. |br|
Sometime you may avoid using easy API; eg: if you wish to use same source file for different modules.

.. c:function:: module_register(name, ctx_name, self, hook)

  Register a new module
    
  :param name: module's name.
  :param ctx_name: module's context name. A new context will be created if it cannot be found.
  :param self: handler for this module that will be created by this call.
  :param hook: struct that holds this module's callbacks.
  :type name: :c:type:`const char *`
  :type ctx_name: :c:type:`const char *`
  :type self: :c:type:`const void **`
  :type hook: :c:type:`const userhook *`
  :returns: MOD_OK if no error happened, MOD_ERR if any error happened.
  
.. c:function:: module_deregister(self)

  Deregister module
    
  :param self: pointer to module's handler. It is set to NULL after this call.
  :type self: :c:type:`const void **`
  :returns: MOD_OK if no error happened, MOD_ERR if any error happened.
  
.. c:function:: module_is(self, state)

  Check current module's state
    
  :param self: pointer to module's handler.
  :param state: state we are interested in.
  :type self: :c:type:`const void *`
  :type state: :c:type:`enum module_states`
  :returns: false (0) if module'state is not 'state', true (1) if it is and MOD_ERR on error.
  
.. c:function:: module_start(self, fd)

  Start a module by polling on fd
    
  :param self: pointer to module's handler
  :param fd: fd to be polled.
  :type self: :c:type:`const void *`
  :type fd: :c:type:`int` 
  :returns: MOD_OK if no error happened, MOD_ERR if any error happened.
  
.. c:function:: module_pause(self)

  Pause module's polling
    
  :param self: pointer to module's handler
  :type self: :c:type:`const void *`
  :returns: MOD_OK if no error happened, MOD_ERR if any error happened.
  
.. c:function:: module_resume(self)

  Resume module's polling
    
  :param self: pointer to module's handler
  :type self: :c:type:`const void *`
  :returns: MOD_OK if no error happened, MOD_ERR if any error happened.
  
.. c:function:: module_stop(self)

  Stop module's polling by closing its fd
    
  :param self: pointer to module's handler
  :type self: :c:type:`const void *`
  :returns: MOD_OK if no error happened, MOD_ERR if any error happened.
  
.. c:macro:: module_become(self, new_recv)

  Change recv callback to new_recv
    
  :param self: pointer to module's handler
  :param new_recv: new module's recv.
  :type self: :c:type:`const void *`
  :type new_recv: :c:type:`recv_cb`
  :returns: MOD_OK if no error happened, MOD_ERR if any error happened.

.. c:function:: module_set_userdata(self, userdata)

  Set userdata for this module; userdata will be passed as parameter to recv callback.
    
  :param self: pointer to module's handler
  :param userdata: module's new userdata.
  :type self: :c:type:`const void *`
  :type userdata: :c:type:`const void *`
  :returns: MOD_OK if no error happened, MOD_ERR if any error happened.
  
.. c:function:: module_log(self, fmt, args)

  Module's logger
    
  :param self: pointer to module's handler
  :param fmt: log's format.
  :param args: variadic argument.
  :type self: :c:type:`const void *`
  :type fmt: :c:type:`const char *`
  :type args: :c:type:`variadic`
  :returns: MOD_OK if no error happened, MOD_ERR if any error happened.
  
