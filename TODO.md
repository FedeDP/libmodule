# TODO

## 1.0

### Generic

- [x] Use self_t as opaque handler to avoid void pointers and update various internal module.c macros
- [x] find a better alternative to $ctx_$name_pre_start for each module pre_start function
- [x] rename get_fd function (as same function should be used to eg: set module's initial subscriptions and initial state)
- [x] Let users implement "non-pollable" modules, ie: modules that are not bound to a FD (but only receives pubsub message) -> ie: set RUNNING state anyway if certain FD is passed
- [x] Update doc
- [x] msg_t all const
- [x] m_tell and m_publish -> check for NULL message
- [x] m_tell check for NULL recipient
- [ ] review hashmap code
- [ ] split in modules.h and module.h? Split even module{s}_easy?

### Logger

- [x] modules_set_logger() function to set a logger?
- [x] Logger will be called in module_log/m_log
- [x] pass module's userdata to logger

### Samples

- [x] Improve easy example
- [x] Improve sharedSrd example
- [ ] Improve multictx example

### Finally

- [ ] Release 1.0

## Later

### Submodules

- [ ] SUBMODULE(B, A) calls module_register(B) and module_binds_to(A);
- [ ] SUBMODULE SHOULD BE STARTED later (after all MODULES) -> ctor3
- [ ] destroy children of modules at module deregister
- [ ] bind children to parent states (ie: parent paused -> children paused; parent resumed -> children resumed...)
- [ ] m_propagate -> like m_tell but to all children

- [ ] new release


### Dep system

- [ ] REQUIRE and AFTER macros

- [ ] new release

## Test it

- [ ] write some tests (cmocka?)
