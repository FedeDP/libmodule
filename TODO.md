# TODO

## 1.1

### Dep system

- [x] REQUIRE and AFTER macros
- [ ] Each module that has dependent modules will be stored in a map with a list of <list of dependent modules> as value.
- [ ] When a module A is registered, the map will be checked and in case it has dependent modules, we will remove A module from their deps and if a module has no more deps, it will be started
- [ ] After all modules have been registered, modules_post_register() will be called to -> drop all modules not yet started if they miss at least a REQUIRE module, or start them if they only miss AFTER modules.


- [ ] Update doc
- [ ] Update sample
- [ ] Update tests
- [ ] Release 1.1

## 1.2

### Submodules

- [ ] SUBMODULE(B, A) calls module_register(B) and module_binds_to(A);
- [ ] SUBMODULE SHOULD BE STARTED later (after all MODULES) -> ctor3 (this way it'll support only level 1 submodules though (ie: a submodule cannot have submodules))
- [ ] destroy children of modules at module deregister
- [ ] bind children to parent states (ie: parent paused -> children paused; parent resumed -> children resumed...)
- [ ] m_forward -> like m_tell but to all children

- [ ] Release 1.2

## 2.0 (API break)

### Extra stuff for easier development

- [ ] m_add_timer
- [ ] m_add_socket
- [ ] m_add_signal
- [ ] m_read_signal, m_read_socket, m_read_timer internal functions with an exposed m_read_fd function.
- [ ] API break: msg_t type will have an fd_t field with "fd, enum fd_type type"
- [ ] "type" defaults to generic when eg: module_add_fd is called; will take correct type (eg {TIMER, SOCKET, SIGNAL}) when helper functions are called
- [ ] Calling m_read_fd(fd_t x) on a generic type fd will just fail obviously.
- [ ] m_read_fd(fd_t x) { switch (x.type) { case TIMER: ...} }
