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
- [x] update doc (mention MAX_EVENTS limit for each context)
- [x] make MAX_EVENTS customizable at compile time?

### API FREEZE

### Port to kqueue on non-linux builds?

- [x] ifndef __linux__ -> we will only support unix builds -> freebsd and osx
- [x] use kqueue where needed
- [x] travis to build on osx too 
- [ ] fix kevent call: returns 0 immediately...
- [x] create a header "poll_priv.h" and 2 plugins: epoll_priv.c and kqueue_priv.c; decide which one to compile in CMakeLists
- [x] free module_poll_t->ev
- [ ] Update doc and readme
- [x] update Examples: properly ifdef linux only features (signalfd/timerfd)

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
