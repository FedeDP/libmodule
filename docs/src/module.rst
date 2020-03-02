.. |br| raw:: html

   <br />
   
Module API
==========

Module API denotes libmodule interface functions to manage each module lifecycle. |br|
It is splitted in two APIs.

.. _module_easy:    

Easy API
--------

Module easy API consists of a single-context-multi-modules set of macros. It can be found in <module/module_easy.h>. |br|
These macros make it easy and transparent to developer all of the module's internal mechanisms, providing a very simple way to use libmodule. |br|
It enforces correct modularity too: each module must have its own source file. |br|
Where not specified, these functions return a :ref:`mod_ret <mod_ret>`.

.. c:macro:: MODULE(name)

  Creates "name" module with a default context: declares all needed functions and creates both constructor and destructor that will automatically register/deregister this module at startup. |br|
  Finally, it declares a :c:type:`const self_t *_self` global variable that will be automatically used in every function call. |br|
  Note that default context will be automatically created if this is the first module to bind on it.
  
  :param name: name of the module to be created
  :type name: :c:type:`const char *` 
  :returns: void
  
.. c:macro:: MODULE_CTX(name, ctxName)

  Creates "name" module in ctxName context: declares all needed functions and creates both constructor and destructor that will automatically register/deregister this module at startup. |br|
  Finally, it declares a :c:type:`const self_t *_self` global variable that will be automatically used in every function call. |br|
  Note that ctxName context will be automatically created if this is the first module to bind on it.
  
  :param name: name of the module to be created
  :param ctxName: name of the context in which the module has to be created
  :type name: :c:type:`const char *` 
  :type ctxName: :c:type:`const char *` 
  :returns: void
  
.. c:macro:: self()
  
  Returns _self handler for the module.
  
  :returns: :c:type:`const self_t *`

.. c:macro:: m_is(state)

  Check current module's state
    
  :param state: state we are interested in; note that it can be an OR of states (eg: IDLE | RUNNING)
  :type state: :c:type:`const enum module_states` 
  :returns: false if module' state is not 'state', true if it is and MOD_ERR on error.
  
.. c:macro:: m_start(void)

  Start module's polling

.. c:macro:: m_pause(void)

  Pause module's polling

.. c:macro:: m_resume(void)

  Resume module's polling
  
.. c:macro:: m_stop(void)

  Stop module's polling by closing its fds. Note that module is not destroyed: you can add new fds and call m_start on it.
  Moreover, its enqueued pubsub messages are destroyed.
  
.. c:macro:: m_become(new_recv)

  Change receive callback to receive_$(new_recv)
    
  :param new_recv: new module's receive callback; the function has prefix \receive_ concatenated with new_recv
  :type new_recv: :c:type:`const recv_cb`

.. c:macro:: m_unbecome(void)

  Reset to default receive poll callback

.. c:macro:: m_set_userdata(userdata)

  Set userdata for this module; userdata will be passed as parameter to receive callback
    
  :param userdata: module's new userdata.
  :type userdata: :c:type:`const void *`

.. c:macro:: m_register_fd(fd, autoclose, userptr)

  Registers a new fd to be polled by a module
    
  :param fd: fd to be registered.
  :param autoclose: whether to automatically close the fd on module stop/fd deregistering.
  :param userptr: data to be passed in receive() callback msg->fd_msg_t when an event happens on this fd.
  :type fd: :c:type:`const int`
  :type autoclose: :c:type:`const bool`
  :type userptr: :c:type:`const void *`
  
.. c:macro:: m_deregister_fd(fd)

  Deregisters a fd from a module
    
  :param fd: module's old fd.
  :type fd: :c:type:`const int`

.. c:macro:: m_log(fmt, args)

  Logger function for this module. Call it the same way you'd call printf.
    
  :param fmt: log's format.
  :param args: variadic argument.
  :type fmt: :c:type:`const char *` 
  :type args: :c:type:`variadic`
  
.. c:macro:: m_dump()

  Dump current module's state. Diagnostic API. Output explanation:  
  * Subscribtions: T -> "Topic", Fl -> "Flags", UP -> "UserPointer"
  * Fds: FD -> fd, Fl -> "Flags", UP -> "UserPointer"
  
.. c:macro:: m_ref(name, modref)

  Takes a reference from another module; it can be used in pubsub messaging to tell a message to it. It must not be freed.
    
  :param name: name of a module.
  :param modref: variable that holds reference to module
  :type name: :c:type:`const char *` 
  :type modref: :c:type:`const self_t **`
  
.. c:macro:: m_subscribe(topic)

  Subscribes the module to a topic. If module is already subscribed to topic, MODULE_ERR will be returned. Note that a regex is a valid topic too.
    
  :param topic: topic to which subscribe.
  :type topic: :c:type:`const char *`
  
.. c:macro:: m_unsubscribe(topic)

  Unsubscribes the module from a topic. If module is not subscribed to topic, MODULE_ERR will be returned.
    
  :param topic: topic to which unsubscribe.
  :type topic: :c:type:`const char *`
  
.. c:macro:: m_tell(recipient, msg, size, autofree)

  Tell a message to another module.
    
  :param recipient: module to whom deliver the message.
  :param msg: actual data to be sent.
  :param size: size of data to be sent.
  :param autofree: whether to autofree msg after last recipient's received it.
  :type recipient: :c:type:`const self_t *`
  :type msg: :c:type:`const void *`
  :type size: :c:type:`const ssize_t`
  :type autofree: :c:type:`const bool`
  
.. c:macro:: m_publish(topic, msg, size, autofree)

  Publish a message on a topic.
    
  :param topic: topic on which publish message. Note that only topic creator can publish message on topic.
  :param msg: actual data to be published.
  :param size: size of data to be published.
  :param autofree: whether to autofree msg after last recipient's received it.
  :type topic: :c:type:`const char *`
  :type msg: :c:type:`const void *`
  :type size: :c:type:`const ssize_t`
  :type autofree: :c:type:`const bool`
  
.. c:macro:: m_broadcast(msg, size, autofree, global)

  Broadcast a message.
    
  :param msg: data to be delivered to all modules in a context.
  :param size: size of data to be delivered.
  :param autofree: whether to autofree msg after last recipient's received it.
  :param global: whether to broadcast to every context.
  :type msg: :c:type:`const void *`
  :type size: :c:type:`const ssize_t`
  :type autofree: :c:type:`const bool`
  :type global: :c:type:`const bool`
  
.. c:macro:: m_tell_str(recipient, msg)

  Tell a string message to another module. Size is automatically computed through strlen, and autofree is set to false.
    
  :param recipient: module to whom deliver the message.
  :param msg: message to be sent.
  :type recipient: :c:type:`const self_t *`
  :type msg: :c:type:`const char *`
  
.. c:macro:: m_publish_str(topic, msg)

  Publish a string message on a topic. Size is automatically computed through strlen, and autofree is set to false.
    
  :param topic: topic on which publish message. Note that only topic creator can publish message on topic.
  :param msg: message to be published.
  :type topic: :c:type:`const char *`
  :type msg: :c:type:`const char *`
  
.. c:macro:: m_broadcast_str(msg, global)

  Broadcast a string message. Same as calling m_publish(NULL, msg). Size is automatically computed through strlen, and autofree is set to false.

  :param msg: message to be delivered to all modules in a context.
  :param global: whether to broadcast to every context.
  :type msg: :c:type:`const char *`
  :type global: :c:type:`const bool`
  
.. c:macro:: m_poisonpill(recipient)

  Enqueue a POISONPILL message to recipient. This allows to stop another module after it flushes its pubsub messages.
  
  :param recipient: RUNNING module to be stopped.
  :type recipient: :c:type:`const self_t *`

.. _module_complex:    
  
Complex API
-----------

Complex (probably better to say less-easy) API consists of `Module easy API`_ internally used functions. It can be found in <module/module.h> header. |br|
Sometime you may avoid using easy API; eg: if you wish to use same source file for different modules, or if you wish to manually register a module. |br|
Again, where not specified, these functions return a :ref:`mod_ret <mod_ret>`.

.. c:function:: module_register(name, ctx_name, self, hook)

  Register a new module
    
  :param name: module's name.
  :param ctx_name: module's context name. A new context will be created if it cannot be found.
  :param self: handler for this module that will be created by this call.
  :param hook: struct that holds this module's callbacks.
  :type name: :c:type:`const char *`
  :type ctx_name: :c:type:`const char *`
  :type self: :c:type:`self_t **`
  :type hook: :c:type:`const userhook *`
  
.. c:function:: module_deregister(self)

  Deregister module
    
  :param self: pointer to module's handler. It is set to NULL after this call.
  :type self: :c:type:`self_t **`
  
.. c:function:: module_is(self, state)

  Check current module's state
    
  :param self: pointer to module's handler.
  :param state: state we are interested in; note that it can be an OR of states (eg: IDLE | RUNNING)
  :type self: :c:type:`const self_t *`
  :type state: :c:type:`const enum module_states`
  :returns: false  if module'state is not 'state', true if it is and MOD_ERR on error.
  
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
  Moreover, its enqueued pubsub messages are destroyed.
    
  :param self: pointer to module's handler
  :type self: :c:type:`const self_t *`
  
.. c:function:: module_become(self, new_receive)

  Change receive callback to new_receive
    
  :param self: pointer to module's handler
  :param new_receive: new module's receive.
  :type self: :c:type:`const self_t *`
  :type new_receive: :c:type:`const recv_cb`

.. c:function:: module_set_userdata(self, userdata)

  Set userdata for this module; userdata will be passed as parameter to receive callback.
    
  :param self: pointer to module's handler
  :param userdata: module's new userdata.
  :type self: :c:type:`const self_t *`
  :type userdata: :c:type:`const void *`

.. c:function:: module_register_fd(self, fd, autoclose, userptr)

  Register a new fd to be polled by a module
    
  :param self: pointer to module's handler
  :param fd: fd to be registered.
  :param autoclose: whether to automatically close the fd on module stop/fd deregistering.
  :param userptr: data to be passed in receive() callback msg->fd_msg_t when an event happens on this fd.
  :type self: :c:type:`const self_t *`
  :type fd: :c:type:`const int`
  :type autoclose: :c:type:`const bool`
  :type userptr: :c:type:`const void *`
  
.. c:function:: module_deregister_fd(self, fd)

  Deregister a fd from a module
    
  :param self: pointer to module's handler
  :param fd: module's old fd.
  :type self: :c:type:`const self_t *`
  :type fd: :c:type:`const int`

.. c:function:: module_get_name(self, name)

  Get module's name from his self pointer
    
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

  Logger function for this module. Call it the same way you'd call printf
    
  :param self: pointer to module's handler
  :param fmt: log's format.
  :param args: variadic argument.
  :type self: :c:type:`const self_t *`
  :type fmt: :c:type:`const char *`
  :type args: :c:type:`variadic`
  
.. c:function:: module_dump(self)

  Dump current module's state. Diagnostic API. Output explanation:  
  * Subscribtions: T -> "Topic", Fl -> "Flags", UP -> "UserPointer"
  * Fds: FD -> fd, Fl -> "Flags", UP -> "UserPointer"
  
  :param self: pointer to module's handler
  :type self: :c:type:`const self_t *`
  
.. c:function:: module_ref(self, name, modref)

  Takes a reference from another module; it can be used in pubsub messaging to tell a message to it. It must not be freed.
    
  :param self: pointer to module's handler
  :param name: name of a module.
  :param modref: variable that holds reference to module
  :type self: :c:type:`const self_t *`
  :type name: :c:type:`const char *` 
  :type modref: :c:type:`const self_t **`
  
.. c:function:: module_subscribe(self, topic)

  Subscribes the module to a topic. If module is already subscribed to topic, MODULE_ERR will be returned. Note that a regex is a valid topic too.
    
  :param self: pointer to module's handler
  :param topic: topic to which subscribe.
  :type self: :c:type:`const self_t *`
  :type topic: :c:type:`const char *`
  
.. c:function:: module_unsubscribe(self, topic)

  Unsubscribes the module from a topic. If module is not subscribed to topic, MODULE_ERR will be returned.
    
  :param self: pointer to module's handler
  :param topic: topic to which unsubscribe.
  :type self: :c:type:`const self_t *`
  :type topic: :c:type:`const char *`
  
.. c:function:: module_tell(self, recipient, msg, size, autofree)

  Tell a message to another module.
    
  :param self: pointer to module's handler
  :param recipient: module to whom deliver the message.
  :param msg: actual data to be sent.
  :param size: size of data to be sent.
  :param autofree: whether to autofree msg after last recipient's received it.
  :type self: :c:type:`const self_t *`
  :type recipient: :c:type:`const self_t *`
  :type msg: :c:type:`const void *`
  :type size: :c:type:`const ssize_t`
  :type autofree: :c:type:`const bool`
  
.. c:function:: module_publish(self, topic, msg, size, autofree)

  Publish a message on a topic.

  :param self: pointer to module's handler
  :param topic: topic on which publish message. Note that only topic creator can publish message on topic.
  :param msg: actual data to be published.
  :param size: size of data to be published.
  :param autofree: whether to autofree msg after last recipient's received it.
  :type self: :c:type:`const self_t *`
  :type topic: :c:type:`const char *`
  :type msg: :c:type:`const void *`
  :type size: :c:type:`const ssize_t`
  :type autofree: :c:type:`const bool`
  
.. c:function:: module_broadcast(self, msg, size, autofree, global)

  Broadcast a message.

  :param self: pointer to module's handler
  :param msg: data to be delivered to all modules in a context.
  :param size: size of data to be delivered.
  :param autofree: whether to autofree msg after last recipient's received it.
  :param global: whether to broadcast to every context.
  :type self: :c:type:`const self_t *`
  :type msg: :c:type:`const void *`
  :type size: :c:type:`const ssize_t`
  :type autofree: :c:type:`const bool`
  :type global: :c:type:`const bool`
  
.. c:function:: module_poisonpill(self, recipient)

  Enqueue a POISONPILL message to recipient. This allows to stop another module after it flushes its pubsub messages.
  
  :param self: pointer to module's handler
  :param recipient: RUNNING module to be stopped.
  :type self: :c:type:`const self_t *`
  :type recipient: :c:type:`const self_t *`
