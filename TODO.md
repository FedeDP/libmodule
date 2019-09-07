## 5.1.0

### Submodules
- [ ] SUBMODULE(B, A) calls module_register(B) and module_binds_to(A);
- [ ] SUBMODULE SHOULD BE STARTED later (after all MODULES) -> ctor3 (this way it'll support only level 1 submodules though (ie: a submodule cannot have submodules))
- [ ] destroy children of modules at module deregister
- [ ] bind children to parent states (ie: parent paused -> children paused; parent resumed -> children resumed...)
- [ ] m_forward -> like m_tell but to all children

## Ideas
- [ ] Akka-persistence like message store? (ie: store all messages and replay them)
- [ ] module_msg_ref/unref to forcefully keep an autofree message alive?
