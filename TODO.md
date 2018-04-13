# TODO

## 1.0

**API FREEZE**

### Test it

- [ ] write remaining modules.h tests (modules_ctx_loop and modules_ctx_quit successful run)
- [ ] write remaining module.h tests (module_add_fd, module_rm_fd, module_update_fd)

### Finally

- [ ] 1.0

## Later

### Dep system

- [ ] REQUIRE and AFTER macros

- [ ] 1.1

### Submodules

- [ ] SUBMODULE(B, A) calls module_register(B) and module_binds_to(A);
- [ ] SUBMODULE SHOULD BE STARTED later (after all MODULES) -> ctor3 (this way it'll support only level 1 submodules though (ie: a submodule cannot have submodules))
- [ ] destroy children of modules at module deregister
- [ ] bind children to parent states (ie: parent paused -> children paused; parent resumed -> children resumed...)
- [ ] m_forward -> like m_tell but to all children

- [ ] 1.2
