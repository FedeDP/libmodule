.. |br| raw:: html

   <br />
   
Module
======

Module API denotes libmodule interface functions that can be found in <module/module.h> header. |br|
It is splitted in two APIs.

.. _module_easy:    

Module easy API
---------------

Module easy API consists of a single-context-multi-modules set of macros. |br|
These macros make it easy and transparent to developer all of the module's internal mechanisms, providing a very simple way to use libmodule. |br|
It enforces correct modularity too: each module must have its own source file. |br|
Where not specified, these functions return a :ref:`module_ret_code <module_ret_code>`.

.. c:macro:: MODULE(name)

  Creates "name" module: declares all needed functions and creates both constructor and destructor that will automatically register/deregister this module at startup. |br|
  Finally, it declares a :c:type:`const self_t *_self` global variable that will be automatically used in every function call.
  
  :param name: name of the module to be created
  :type name: :c:type:`const char *` 
  :returns: void

.. c:macro:: m_is(state)

  Check current module's state
    
  :param state: state we are interested in; note that it can be an OR of states (eg: IDLE | RUNNING)
  :type state: :c:type:`enum module_states` 
  :returns: false (0) if module' state is not 'state', true (1) if it is and MOD_ERR on error.
  
.. c:macro:: m_start(void)

  Start module's polling

.. c:macro:: m_pause(void)

  Pause module's polling

.. c:macro:: m_resume(void)

  Resume module's polling
  
.. c:macro:: m_stop(void)

  Stop module's polling by closing its fds. Note that module is not destroyed: you can add new fds and call m_start on it.
  
.. c:macro:: m_become(new_recv)

  Change receive callback to receive_new_recv
    
  :param new_recv: new module's receive callback; the function has prefix \receive_ concatenated with new_recv
  :type new_recv: untyped

.. c:macro:: m_unbecome(void)

  Reset to default receive poll callback

.. c:macro:: m_set_userdata(userdata)

  Set userdata for this module; userdata will be passed as parameter to receive callback
    
  :param userdata: module's new userdata.
  :type userdata: :c:type:`const void *`

.. c:macro:: m_add_fd(fd)

  Add a new fd to be polled by a module
    
  :param fd: module's old fd.
  :type fd: :c:type:`int`
  
.. c:macro:: m_rm_fd(fd, close_fd)

  Remove a fd from a module
    
  :param fd: module's old fd.
  :param close_fd: true to close the removed fd.
  :type fd: :c:type:`int`
  :type close_fd: :c:type:`int`

.. c:macro:: m_update_fd(old_fd, new_fd, close_old)

  Update old_fd to new_fd for this module
    
  :param old_fd: module's old fd to be replaced.
  :param new_fd: module's new fd.
  :param close_old: whether to close old_fd.
  :type old_fd: :c:type:`int`
  :type new_fd: :c:type:`int`
  :type close_old: :c:type:`int`

.. c:macro:: m_log(fmt, args)

  Logger function for this module. Call it the same way you'd call printf
    
  :param fmt: log's format.
  :param args: variadic argument.
  :type fmt: :c:type:`const char *` 
  :type args: :c:type:`variadic`
  
.. c:macro:: m_subscribe(topic)

  Subscribes the module to a topic.
    
  :param topic: topic to which subscribe.
  :type topic: :c:type:`const char *`
  
.. c:macro:: m_tell(recipient, msg)

  Tell a message to another module.
    
  :param recipient: module to whom deliver the message.
  :param msg: actual message to be sent.
  :type recipient: :c:type:`const char *`
  :type msg: :c:type:`const char *`
  
    
.. c:macro:: m_reply(sender, msg)

  Reply to a received message.
    
  :param sender: module which sent us a message.
  :param msg: actual message to be sent.
  :type recipient: :c:type:`const self_t *`
  :type msg: :c:type:`const char *`
  
.. c:macro:: m_publish(topic, msg)

  Publish a message on a topic.
    
  :param topic: topic on which publish message. NULL to broadcast message to all modules in same context.
  :param msg: actual message to be sent.
  :type topic: :c:type:`const char *`
  :type msg: :c:type:`const char *`
  
.. c:macro:: m_broadcast(msg)

  Broadcast a message in module's context. Same as calling m_publish(NULL, msg).
    
  :param msg: message to be delivered to all modules in a context.
  :type msg: :c:type:`const char *`

.. _module_complex:    
  
Module Complex API
------------------

Complex (probably better to say less-easy) API consists of `Module easy API`_ internally used functions. |br|
Sometime you may avoid using easy API; eg: if you wish to use same source file for different modules. |br|
Again, where not specified, these functions return a :ref:`module_ret_code <module_ret_code>`.

.. c:function:: module_register(name, ctx_name, self, hook)

  Register a new module
    
  :param name: module's name.
  :param ctx_name: module's context name. A new context will be created if it cannot be found.
  :param self: handler for this module that will be created by this call.
  :param hook: struct that holds this module's callbacks.
  :type name: :c:type:`const char *`
  :type ctx_name: :c:type:`const char *`
  :type self: :c:type:`const self_t **`
  :type hook: :c:type:`const userhook *`
  
.. c:function:: module_deregister(self)

  Deregister module
    
  :param self: pointer to module's handler. It is set to NULL after this call.
  :type self: :c:type:`const self_t **`
  
.. c:function:: module_is(self, state)

  Check current module's state
    
  :param self: pointer to module's handler.
  :param state: state we are interested in; note that it can be an OR of states (eg: IDLE | RUNNING)
  :type self: :c:type:`const self_t *`
  :type state: :c:type:`enum module_states`
  :returns: false (0) if module'state is not 'state', true (1) if it is and MOD_ERR on error.
  
.. c:function:: module_start(self)

  Start module's polling
    
  :param self: pointer to module's handler
  :type self: :c:type:`const self_t *`
  
.. c:function:: module_pause(self)

  Pause module's polling
    
  :param self: pointer to module's handler
  :type self: :c:type:`const self_t *`
  
.. c:function:: module_resume(self)

  Resume module's polling
    
  :param self: pointer to module's handler
  :type self: :c:type:`const self_t *`
  
.. c:function:: module_stop(self)

  Stop module's polling by closing its fds. Note that module is not destroyed: you can add new fds and call module_start on it.
    
  :param self: pointer to module's handler
  :type self: :c:type:`const self_t *`
  
.. c:function:: module_become(self, new_receive)

  Change receive callback to new_receive
    
  :param self: pointer to module's handler
  :param new_receive: new module's receive.
  :type self: :c:type:`const self_t *`
  :type new_receive: :c:type:`recv_cb`

.. c:function:: module_set_userdata(self, userdata)

  Set userdata for this module; userdata will be passed as parameter to receive callback.
    
  :param self: pointer to module's handler
  :param userdata: module's new userdata.
  :type self: :c:type:`const self_t *`
  :type userdata: :c:type:`const void *`

.. c:function:: module_add_fd(self, fd)

  Add a new fd to be polled by a module
    
  :param self: pointer to module's handler
  :param fd: module's old fd.
  :type self: :c:type:`const self_t *`
  :type fd: :c:type:`int`
  
.. c:function:: module_rm_fd(self, fd, close_fd)

  Remove a fd from a module
    
  :param self: pointer to module's handler
  :param fd: module's old fd.
  :param close_fd: true to close the removed fd.
  :type self: :c:type:`const self_t *`
  :type fd: :c:type:`int`
  :type close_fd: :c:type:`int`
  
.. c:function:: module_update_fd(self, old_fd, new_fd, close_old)

  Update old_fd to new_fd for this module
    
  :param self: pointer to module's handler
  :param old_fd: module's old fd to be replaced.
  :param new_fd: module's new fd.
  :param close_old: whether to close old_fd.
  :type self: :c:type:`const self_t *`
  :type old_fd: :c:type:`int`
  :type new_fd: :c:type:`int`
  :type close_old: :c:type:`int`

.. c:function:: module_get_name(self, name)

  Get module's name from his self pointer.
    
  :param self: pointer to module's handler
  :param name: pointer to storage for module's name. Note that this must be freed by user.
  :type self: :c:type:`const self_t *`
  :type name: :c:type:`char **`
  
  .. c:function:: module_get_context(self, ctx)

  Get module's name from his self pointer.
    
  :param self: pointer to module's handler
  :param ctx: pointer to storage for module's ctx. Note that this must be freed by user.
  :type self: :c:type:`const self_t *`
  :type ctx: :c:type:`char **`
  
.. c:function:: module_log(self, fmt, args)

  Module's logger
    
  :param self: pointer to module's handler
  :param fmt: log's format.
  :param args: variadic argument.
  :type self: :c:type:`const self_t *`
  :type fmt: :c:type:`const char *`
  :type args: :c:type:`variadic`

.. c:function:: module_subscribe(self, topic)

  Subscribes the module to a topic.

  :param self: pointer to module's handler
  :param topic: topic to which subscribe.
  :type self: :c:type:`const self_t *`
  :type topic: :c:type:`const char *`
  
.. c:function:: module_tell(self, recipient, msg)

  Tell a message to another module.
    
  :param self: pointer to module's handler
  :param recipient: module to whom deliver the message.
  :param msg: actual message to be sent.
  :type self: :c:type:`const self_t *`
  :type recipient: :c:type:`const char *`
  :type msg: :c:type:`const char *`
  
.. c:function:: module_reply(self, sender, msg)

  Reply to a received message.
    
  :param self: pointer to module's handler
  :param sender: module which sent us a message.
  :param msg: actual message to be sent.
  :type self: :c:type:`const self_t *`
  :type sender: :c:type:`const self_t *`
  :type msg: :c:type:`const char *`
  
.. c:function:: module_publish(self, topic, msg)

  Publish a message on a topic.

  :param self: pointer to module's handler
  :param topic: topic on which publish message. NULL to broadcast message to all modules in same context.
  :param msg: actual message to be sent.
  :type self: :c:type:`const self_t *`
  :type topic: :c:type:`const char *`
  :type msg: :c:type:`const char *`
