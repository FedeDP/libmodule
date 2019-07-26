## 5.0.0

### PubSub improvements
- [x] avoid strdupping topic when creating pubsub_msg_t
- [x] Only use one pubsub msg for every module, instead of  creating one for each recipient
- [x] Keep a reference count for each "sent" pubsub msg: when it reaches 0 it means all recipients received the message, and then in can be freed
- [x] Add an autofree field to module_tell/module_pubsub that automatically frees sent data when last recipient receives it
- [x] Avoid strdupping module's name and ctx's name
- [x] Avoid any allocation inside library and ALWAYS TRUST USER POINTER as core rule
- [x] Add a self() alias to _self
- [x] module_subscribe/unsubscribe should return OK if already subscribed/not subscribed
- [x] Add new System messages: MODULE_STARTED/STOPPED
- [x] Avoid telling system messages like MODULE_STARTED/TOPIC_REGISTERED to ourselves
- [x] Test pubsub messagging for paused modules
- [x] Fix memleaks!
- [x] Unsubscribe shopuld not check if topic is registered in ctx as otherwise umsubscribing from a deregistered topic would not work.
- [x] pubsub interface should take "const void *" instead of "const unsigned char *" as data
- [x] Add a new parameter "bool global" to module_broadcast?
- [x] FIX: if first context does not receive a global broadcast, message will be destroyed as it has 0 refs...
- [x] Rename pubsub_msg to ps_msg inside msg_t
- [x] Rename pubsub_msg_t to ps_msg_t
- [x] Rename pubsub_priv_t to ps_priv_t
- [x] Add a module_poisonpill() function, to send a system message to a module to stop it -> this will be enqueued in module's message queue, 
conversely to module_stop that should stop module right away freeing all its enqueued messages.
- [x] Always destroy messages in flush_pubsub_msg()? RIght now it delivers all of them if we're stopping looping on ctx
- [x] module_poisonpill should only be sent to RUNNING modules
- [x] when stop looping on a context, flush all pubsub messages to RUNNING modules only. Destroy messages for non-running modules.

### Map
- [x] FIx: avoid incrementing map size on value update
- [x] Add test for update
- [x] map->dtor should default to memhook._free, and fallback to default if map_set_dtor is called with NULL callback parameter 
- [x] Add map_itr_t interface
- [x] Add tests for new interface
- [x] Improve hashmap implementation
- [x] Add a stress test for hashmap implementation
- [x] Fix map_clear implementation
- [x] Add map_iterate test
- [x] Set keydup and autofree in map_new()
- [x] Drop map_set_dtor
- [x] Destroy old value upon val update

### Stack
- [x] stack->dtor should default to memhook._free, and fallback to default if stack_set_dtor is called with NULL callback parameter 
- [X] Add stack_itr_t interface
- [x] Add tests for new interface
- [x] Stack_iterate to follow same logic as map_iterate
- [x] Set autofree in stack_new()
- [x] Drop stack_set_dtor

### Generic
- [x] Add some diagnostic API, eg: module_dump() (to dump each module's state)
- [x] Add a module_load/unload function, to load a module from a compiled object at runtime
- [x] simplify interface for module_load/unload? ie: module_load(path, ctx_name)
- [x] Add test.so build to Easy example
- [x] Re-evaluate module_register/deregister parameters (ie: self_t should not be a const!)
- [x] Provide a default (weak symbol) main() that just runs modules_ctx_loop() on any ctx it found
- [x] Rename MODULE_DEFAULT_CTX and MODULE_MODULE_MAX_EVENTS to MODULES_*
- [x] Actually call init() callback first time module is started, even without passing from evaluate_module (thus without looping ctx)
- [x] module_load/unload should use RTLD_NOLOAD flag instead of yet another hashmap
- [x] module_get_name/ctx to return a strdup string
- [x] Rename modules_set_memalloc_hook to modules_set_memhook() + rename memalloc_hook to memhook_t
- [x] stop() and start() should avoid err_str parameter.
- [x] Stop looping on context when there are no RUNNING modules
- [x] Cleanup some out of date comments in examples
- [x] Avoid exposing in modules.h main() and modules_pre_start() functions: they're not part of libmodule's API and cannot be called as functions.

### Doc
- [x] module_dump
- [x] module_subscribe/unsubscribe
- [x] self()
- [x] module_register/deregister
- [x] module_load/unload
- [x] module_tell/publish/broadcast
- [x] MODULE_STARTED/MODULE_STOPPED new sysmessages
- [x] Avoid telling system messages like MODULE_STARTED/TOPIC_REGISTERED to ourselves
- [x] Document main() weak symbol!
- [x] Add a new page about trusting pointers
- [x] modules_set_memhook/memhook_t
- [x] new stop behaviour (stop will always destroy pubsub messages instead of delivering them, except when a module gets stopped through module_poisonpill)
- [x] module_poisonpill
- [x] New modules_loop behaviour (stop looping when no RUNNING modules)
- [x] New Map API
- [x] New Stack API

### Samples
- [x] Fix samples

### Test
- [x] Fix tests
- [x] add module_dump tests

## 5.1.0

### Submodules
- [ ] SUBMODULE(B, A) calls module_register(B) and module_binds_to(A);
- [ ] SUBMODULE SHOULD BE STARTED later (after all MODULES) -> ctor3 (this way it'll support only level 1 submodules though (ie: a submodule cannot have submodules))
- [ ] destroy children of modules at module deregister
- [ ] bind children to parent states (ie: parent paused -> children paused; parent resumed -> children resumed...)
- [ ] m_forward -> like m_tell but to all children

## Ideas
- [ ] Akka-persistence like message store? (ie: store all messages and replay them)
