## 3.0.1

- [x] Shouldn't module_deregister_fd() remove it from epoll too if module is running?
- [x] When deregistering, actually call stop() on STOPPED modules too!
- [x] PAUSED module should still be delivered PubSub messages, but they will only receive them as soon as they get resumed (thus RUNNING state).
- [x] module_stop can be called on PAUSED modules too!
- [x] Document module states! (STOP is same as IDLE; IDLE means it is registered but still never started. STOPPED it means it is registered, was started and was stopped.)

- [ ] Release 3.0.1

## 3.1.0

- [ ] self_t will hold a pointer to module *
- [ ] insert self_t into context->modules, instead of module
- [ ] this way we avoid modules hashmap and GET_MOD etc etc is way faster (no hashmap lookup)

- [ ] Release 3.1.0

## Ideas

- [ ] Let contexts talk together? Eg: broadcast(msg, bool global) to send a message to all modules in every context; module_publish message in another context? etc etc

### Submodules (??)

- [ ] SUBMODULE(B, A) calls module_register(B) and module_binds_to(A);
- [ ] SUBMODULE SHOULD BE STARTED later (after all MODULES) -> ctor3 (this way it'll support only level 1 submodules though (ie: a submodule cannot have submodules))
- [ ] destroy children of modules at module deregister
- [ ] bind children to parent states (ie: parent paused -> children paused; parent resumed -> children resumed...)
- [ ] m_forward -> like m_tell but to all children
