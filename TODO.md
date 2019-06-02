## 4.2.0

### Generic 
- [x] Avoid calling evaluate_module when registering a new module. Call evaluate_module on each module when loop_start() is called.
- [x] Fix default values for pubsub_fd[] for each module
- [x] Update tests (module_start should now be called upon registering)
- [x] Fix MultiCtx sample (signalfd multithread crazyness)
- [x] Update doc (changed behaviours!)
- [x] Drop mcontext -> num_fds

## 4.3.0

### Submodules
- [ ] SUBMODULE(B, A) calls module_register(B) and module_binds_to(A);
- [ ] SUBMODULE SHOULD BE STARTED later (after all MODULES) -> ctor3 (this way it'll support only level 1 submodules though (ie: a submodule cannot have submodules))
- [ ] destroy children of modules at module deregister
- [ ] bind children to parent states (ie: parent paused -> children paused; parent resumed -> children resumed...)
- [ ] m_forward -> like m_tell but to all children

## Ideas
- [ ] Let contexts talk together? Eg: broadcast(msg, bool global) to send a message to all modules in every context; module_publish message in another context? etc etc
- [ ] Akka-persistence like message store? (ie: store all messages and replay them)
