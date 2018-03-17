# TODO

## 1.0

### Drop on error callback?

- [x] drop modules_ctx_on_error (easy)
- [x] expand error defines (eg MOD_NOT_FOUND -2 etc etc) (easy)

### Generic

- [x] Add a module_pre_start function for each module (easy)
- [X] After module_stop -> STOPPED status (to prevent it being evaluated again after new event)
- [x] Add a m_update_fd(int fd, bool close_old) function to change fd (easy)
- [x] actually check if another mod with same name is already in context
- [x] MODULE and MODULE_CTX macro to take strings parameters
- [x] rename init in get_fd
- [x] rename module ctor in init() and dtor in destroy (they're static now)
- [ ] find a better alternative to $ctx_$name_pre_start for each module...
- [x] Check libmodule with valgrind again (last time it had no memleaks) 
- [ ] Let users implement "non-pollable" modules, ie: modules that are not bound to a FD (but only receives pubsub message) -> ie: set RUNNING state anyway if certain FD is passed
- [x] module_is() to accept bitmask of states
- [x] protect needed functions by checking module's state

### Much more actor like?

- [x] implement tell and publish (per-ctx) (easy)
- [x] implement subscribe (mid)
- [x] add a pubsub type in msg_t (easy)

- [x] Update samples
- [ ] Update doc

**UPDATE DOC with changes until there (where to mention ctx##_##name_pre_start() function?) + RELEASE 1.0**

## 1.1

### Submodules

- [ ] SUBMODULE(B, A) calls module_register(B) and module_binds_to(A);
- [ ] SUBMODULE SHOULD BE STARTED later (after all MODULES) -> ctor3
- [ ] destroy children of modules at module deregister
- [ ] bind children to parent states (ie: parent paused -> children paused; parent resumed -> children resumed...)

**UPDATE DOC with SUBMODULE interface + release 1.1**

## Test it

- [ ] write some tests (cmocka?)
