# TODO

## 1.0

### API FREEZE

### Port to kqueue on non-linux builds?

- [x] ifndef __linux__ -> we will only support unix builds -> freebsd and osx
- [x] use kqueue where needed
- [x] travis to build on osx too 
- [x] fix kevent call: returns 0 immediately...
- [x] add a poll_close poll_priv interface function

- [x] create a header "poll_priv.h" and 2 plugins: epoll_priv.c and kqueue_priv.c; decide which one to compile in CMakeLists
- [x] free module_poll_t->ev
- [x] Update doc and readme
- [x] update Examples: properly ifdef linux only features (signalfd/timerfd)

- [x] add MODULE_VERSION_ macros

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
