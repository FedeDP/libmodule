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
- [x] Unsubscribe every module when a topic gets deregistered

### Generic
- [x] Add some diagnostic API, eg: module_dump() (to dump each module's state)
- [x] Add a module_load/unload function, to load a module from a compiled object at runtime
- [ ] simplify interface for module_load/unload? ie: module_load(path, ctx_name)
- [x] Add test.so build to Easy example
- [x] Re-evaluate module_register/deregister parameters (ie: self_t should not be a const!)

### Doc
- [x] module_dump
- [x] module_subscribe/unsubscribe
- [x] self()
- [x] module_register/deregister
- [ ] module_load/unload
- [x] module_tell/publish/broadcast
- [x] MODULE_STARTED/MODULE_STOPPED new sysmessages
- [x] Avoid telling system messages like MODULE_STARTED/TOPIC_REGISTERED to ourselves
- [ ] Add doc for new deregister_topic behaviour (ie: it automatically unsubscribes any module subscribed)
- [ ] Add a new page about trusting pointers

### Samples
- [x] Fix samples

### Test
- [x] Fix tests
- [x] add module_dump tests
- [ ] Add test to new deregister_topic behaviour (ie: it automatically unsubscribes any module subscribed)

## 5.1.0

### Submodules
- [ ] SUBMODULE(B, A) calls module_register(B) and module_binds_to(A);
- [ ] SUBMODULE SHOULD BE STARTED later (after all MODULES) -> ctor3 (this way it'll support only level 1 submodules though (ie: a submodule cannot have submodules))
- [ ] destroy children of modules at module deregister
- [ ] bind children to parent states (ie: parent paused -> children paused; parent resumed -> children resumed...)
- [ ] m_forward -> like m_tell but to all children

## Ideas
- [ ] Let contexts talk together? Eg: broadcast(msg, bool global) to send a message to all modules in every context; module_publish message in another context? etc etc
- [ ] Akka-persistence like message store? (ie: store all messages and replay them)
