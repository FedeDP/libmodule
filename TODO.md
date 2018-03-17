# TODO

## 1.0

### Generic

- [ ] find a better alternative to $ctx_$name_pre_start for each module pre_start function
- [ ] rename get_fd function (as same function should be used to eg: set module's initial subscriptions and initial state)
- [ ] Let users implement "non-pollable" modules, ie: modules that are not bound to a FD (but only receives pubsub message) -> ie: set RUNNING state anyway if certain FD is passed
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
