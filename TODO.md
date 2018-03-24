# TODO

## 1.0

### Generic

- [ ] review hashmap code
- [ ] split in modules.h and module.h? Split even module{s}_easy?

### Samples

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
