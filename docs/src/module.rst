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
    
  :param: :c:type:`enum module_states` state: state we are interested in.
  :returns: false (0) if module'state is not 'state', true (1) if it is.
  
.. c:macro:: m_start(fd)

  Start a module by polling on fd
    
  :param: :c:type:`int` fd: fd to be polled.
  :returns: 0 if no error happened, < 0 if any error happened.
  
.. c:macro:: m_pause(void)

  Pause module's polling
    
  :returns: 0 if no error happened, < 0 if any error happened.
  
.. c:macro:: m_resume(void)

  Resume module's polling
    
  :returns: 0 if no error happened, < 0 if any error happened.
  
.. c:macro:: m_stop(void)

  Stop module's polling by closing its fd
    
  :returns: 0 if no error happened, < 0 if any error happened.
  
.. c:macro:: m_become(new_recv)

  Change recv callback to recv_new_recv
    
  :param: new_recv: new module's recv; the function has suffix \recv_ concatenated with new_recv.

.. c:macro:: m_unbecome(void)

  Reset to default recv poll callback
  
.. c:macro:: m_set_userdata(userdata)

  Set userdata for this module; userdata will be passed as parameter to recv callback.
    
  :param: :c:type:`const void *` userdata: module's new userdata.

.. c:macro:: m_log(fmt, ...)

  Logger for this module. Call it the same way you'd call printf
    
  :param: :c:type:`const char *` fmt: log's format.
  :param ...: variadic argument.

Module less-easy API
--------------------

Less-easy API consists of `Module easy API`_ internal functions. |br|
Sometime you may avoid using easy API; eg: if you wish to use same source file for different modules.

.. c:function:: module_register(name, ctx_name, self, hook)

  Register a new module
    
  :param: :c:type:`const char *` name: module's name.
  :param: :c:type:`const char *` ctx_name: module's context name. A new context will be created if it cannot be found.
  :param: :c:type:`const void **` self: handler for this module that will be created by this call.
  :param: :c:type:`const userhook *` hook: struct that holds this module's callbacks.
  
.. c:function:: module_deregister(self)

  Deregister module
    
  :param: :c:type:`const void **` self: pointer to module's handler. It is set to NULL after this call.
  
.. c:function:: module_is(self, state)

  Check current module's state
    
  :param: :c:type:`const void *` self: pointer to module's handler.
  :param: :c:type:`enum module_states` state: state we are interested in.
  :returns: false (0) if module'state is not 'state', true (1) if it is.
  
.. c:function:: module_start(self, fd)

  Start a module by polling on fd
    
  :param: :c:type:`const void *` self: pointer to module's handler.
  :param: :c:type:`int` fd: fd to be polled.
  :returns: 0 if no error happened, < 0 if any error happened.
  
.. c:function:: module_pause(self)

  Pause module's polling
    
  :param: :c:type:`const void *` self: pointer to module's handler.
  
.. c:function:: module_resume(self)

  Resume module's polling
    
  :param: :c:type:`const void *` self: pointer to module's handler.
  
.. c:function:: module_stop(self)

  Stop module's polling by closing its fd
    
  :param: :c:type:`const void *` self: pointer to module's handler.
  
.. c:macro:: module_become(self, new_recv)

  Change recv callback to new_recv
    
  :param: :c:type:`const void *` self: pointer to module's handler.
  :param: :c:type:`recv_cb` new_recv: new module's recv.

.. c:function:: module_set_userdata(self, userdata)

  Set userdata for this module; userdata will be passed as parameter to recv callback.
    
  :param: :c:type:`const void *` self: pointer to module's handler.
  :param: :c:type:`const void *` userdata: module's new userdata.
  
.. c:function:: module_log(self, fmt, ...)

  Module's logger
    
  :param: :c:type:`const void *` self: pointer to module's handler.
  :param: :c:type:`const char *` fmt: log's format.
  :param ...: variadic argument.
  
