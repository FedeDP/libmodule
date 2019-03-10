## 4.1.0

### New features
- [x] Add an API to integrate module loop inside another loop
- [x] modules_get_fd(const char *ctx)
- [x] modules_dispatch(ctx)
- [ ] Update Doc
- [ ] Add tests?

### Fixes
- [x] In stack_clear and map_clear, avoid unsetting dtor

## Ideas
- [ ] Let contexts talk together? Eg: broadcast(msg, bool global) to send a message to all modules in every context; module_publish message in another context? etc etc

### Submodules (??)
- [ ] SUBMODULE(B, A) calls module_register(B) and module_binds_to(A);
- [ ] SUBMODULE SHOULD BE STARTED later (after all MODULES) -> ctor3 (this way it'll support only level 1 submodules though (ie: a submodule cannot have submodules))
- [ ] destroy children of modules at module deregister
- [ ] bind children to parent states (ie: parent paused -> children paused; parent resumed -> children resumed...)
- [ ] m_forward -> like m_tell but to all children
