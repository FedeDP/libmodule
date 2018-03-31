# TODO

## 1.0

### Multiple-fds module

- [x] Properly add a way to let one module listen on multiple fds (new private type: module_poll_t -> void init() { module_add_fd(x) } )
- [x] Drop MODULE_DONT_POLL and limits.h include in module.h: if module has no fds it will act like MODULE_DONT_POLL
- [x] What about epoll? How can i find out fd to be passed to receive()?
- [x] update module_update_fd to just rm old fd and add a new one
- [x] fix valgrind issues
- [x] update easy example
- [x] add module_remove_fd function
- [ ] update doc (mention MAX_EVENTS limit for each context)

### API FREEZE

## Test it

- [ ] write some tests (cmocka?)

### Finally

- [ ] Release 1.0

## Later

### Submodules

- [ ] SUBMODULE(B, A) calls module_register(B) and module_binds_to(A);
- [ ] SUBMODULE SHOULD BE STARTED later (after all MODULES) -> ctor3 (this way it'll support only level 1 submodules though (ie: a submodule cannot have submodules))
- [ ] destroy children of modules at module deregister
- [ ] bind children to parent states (ie: parent paused -> children paused; parent resumed -> children resumed...)
- [ ] m_forward -> like m_tell but to all children

- [ ] new release


### Dep system

- [ ] REQUIRE and AFTER macros

- [ ] new release
