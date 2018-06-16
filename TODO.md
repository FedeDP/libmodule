# TODO

## 2.0

### Small API improvements

- [x] Pass a self_t instead of const char *name/context in various exposed function
- [x] Add a module_get_name/context(self_t *self) that returns a strdupped string
- [x] add a modules_set_memhook function to let user define memory allocator/deallocator functions. Defaults to malloc/realloc/free

- [ ] UPDATE DOC + document MODULE_VERSION_MAJ/MIN/PATCH

### Remote modules concept

- [ ] Add "type" enum in self_t -> LOCAL, REMOTE
- [ ] Add a MODULE_REMOTE() macro
- [ ] Remote modules work as bus between 2+ remote libmodule instances. When you send a message to a remote module, it will be forwarded to real remote module
- [ ] Study feasibility...
- [ ] REST like api? Wrap remote messages in json through jansson? (https://babelouest.github.io/ulfius/ ?)

- [ ] UPDATE DOC

## Dep system (??)

- [ ] REQUIRE and AFTER macros?

- [ ] Release

## Submodules (??)

- [ ] SUBMODULE(B, A) calls module_register(B) and module_binds_to(A);
- [ ] SUBMODULE SHOULD BE STARTED later (after all MODULES) -> ctor3 (this way it'll support only level 1 submodules though (ie: a submodule cannot have submodules))
- [ ] destroy children of modules at module deregister
- [ ] bind children to parent states (ie: parent paused -> children paused; parent resumed -> children resumed...)
- [ ] m_forward -> like m_tell but to all children

- [ ] Release
