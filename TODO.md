## 3.1.0

### Generic
- [x] Add readme mention of Clightd, project that is using libmodule
- [x] Expose module_map_{create/put/get/destroy} through a module_map.h header?

### Improvements
- [ ] self_t will hold a pointer to module and to its context
- [ ] Store module name in module struct (strdup'd)
- [ ] Store context name in m_context (strdup'd)
- [ ] Use them as hashmap key without strudpping again
- [ ] hashmap_put to take additional "bool key_strdup" param
- [ ] insert self_t into context->modules, instead of module
- [ ] this way GET_MOD etc etc are way faster (no context and modules hashmap lookup)
- [x] add a module_broadcast function

### Fixes
- [x] Move call to "destroy()" callback after module and self_t destruction; this way no more libmodule things can be called on that being-destroyed module
- [x] Fix poll loop to manage EINTR/EAGAIN interrupt signals
- [x] Properly check poll_set_data call in module.c

### Tests
- [x] Add module_broadcast test
- [x] Add test_module_map.h

### Doc
- [x] Add module_map API documentation

- [ ] Release 3.1.0

## Ideas

- [ ] Let contexts talk together? Eg: broadcast(msg, bool global) to send a message to all modules in every context; module_publish message in another context? etc etc

### Submodules (??)

- [ ] SUBMODULE(B, A) calls module_register(B) and module_binds_to(A);
- [ ] SUBMODULE SHOULD BE STARTED later (after all MODULES) -> ctor3 (this way it'll support only level 1 submodules though (ie: a submodule cannot have submodules))
- [ ] destroy children of modules at module deregister
- [ ] bind children to parent states (ie: parent paused -> children paused; parent resumed -> children resumed...)
- [ ] m_forward -> like m_tell but to all children
