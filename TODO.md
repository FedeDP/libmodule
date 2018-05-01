# TODO

## 1.1

### Dep system

- [ ] REQUIRE and AFTER macros

- [ ] Release 1.1

## 1.2

### Submodules

- [ ] SUBMODULE(B, A) calls module_register(B) and module_binds_to(A);
- [ ] SUBMODULE SHOULD BE STARTED later (after all MODULES) -> ctor3 (this way it'll support only level 1 submodules though (ie: a submodule cannot have submodules))
- [ ] destroy children of modules at module deregister
- [ ] bind children to parent states (ie: parent paused -> children paused; parent resumed -> children resumed...)
- [ ] m_forward -> like m_tell but to all children

- [ ] Release 1.2

## 1.3

### Extra stuff for easier development

- [ ] m_add_timer
- [ ] m_add_socket
- [ ] m_add_signal
- [ ] m_read_signal, m_read_socket, m_read_timer internal functions with an exposed m_read_fd() function.
