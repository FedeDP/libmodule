# TODO

## 1.0

### Generic

- [x] Use self_t as opaque handler to avoid void pointers and update various internal module.c macros
- [x] find a better alternative to $ctx_$name_pre_start for each module pre_start function
- [x] rename get_fd function (as same function should be used to eg: set module's initial subscriptions and initial state)
- [x] Let users implement "non-pollable" modules, ie: modules that are not bound to a FD (but only receives pubsub message) -> ie: set RUNNING state anyway if certain FD is passed
- [x] Update doc
- [x] msg_t all const
- [ ] add an hashmap_exists() function
- [ ] properly use an hashset for module subscriptions
- [ ] split in module.h and modules.h (module/module{s}.h)

### Logger

- [ ] modules_set_logger() function to set a logger?
- [ ] Logger will be called in module_log/m_log

### Finally

- [ ] Release 1.0

## Later

### Submodules

- [ ] SUBMODULE(B, A) calls module_register(B) and module_binds_to(A);
- [ ] SUBMODULE SHOULD BE STARTED later (after all MODULES) -> ctor3
- [ ] destroy children of modules at module deregister
- [ ] bind children to parent states (ie: parent paused -> children paused; parent resumed -> children resumed...)

- [ ] new release


### Dep system

- [ ] REQUIRE and AFTER macros

- [ ] new release

## Test it

- [ ] write some tests (cmocka?)
