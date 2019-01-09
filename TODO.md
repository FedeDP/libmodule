## 4.0.0

### Module_ref

- [x] module_ref returns a new self_t* with a "bool is_ref=true"; then we only need to check that is_ref is false.
module_ref should then return a new self_t* object, add it to a stack in module. 
- [x] A module should clean all its reference when leaving (destroying stack)
- [x] All API should go through self_t, eg: self_t *recipient = module_ref("foo"); module_tell(self, recipient, ...);

- [x] All API should avoid using an another-mod-ref as self

- [x] self_t to have a self_t *owner; if it is NULL it is not a reference; otherwise it is. Only module that referenced can unreference.
- [x] Should we pass an unique reference in pubsub messages for each recipient? Or only one for all?

-> this leads to better performance too (no more hashmap lookups for module_tell)

- [x] Better arrange code in module.c (ie: above internal API, below exposed API)
- [x] Split module.c into module_pubsub.c and module_generic.c (?)
- [x] Add a module_priv.c with various common (priv) functions?
- [ ] Better macros naming (eg: GET_MOD_PURE etc etc...)

- [x] Add new tests for module_ref!
- [x] Update libmodule API doc
- [x] State in doc when you can use a module ref for normal functions (eg: module_is)

- [x] Each module's self should be a static (non-pointer) variable (to avoid user freeing it)?

### Generic
- [x] Use attribute pure where needed
- [x] Use attribute format where needed
- [x] Use calloc where needed
- [x] Module register should avoid memleaks when it fails with NOMEM.
- [x] Added modules_ctx_loop test
- [x] Added module pubsub recv test

### Stack API
- [x] Add _clear function
- [x] Add tests!
- [x] Update doc!

### Map API
- [x] Add _clear function
- [x] Add tests!
- [x] Fix issue in map_new if m->data calloc fails: map object was not memsetted to 0; it would lead to a crash when map_free was called
- [x] Update doc!

### Fix
- [x] Actually honor current module's callback when flushing pubsub messages

### Examples
- [x] Update examples!

### Release 4.0 (api break)!

## 4.1.0
- [ ] PoisonPill message to automatically stop another module?
- [ ] Expose a queue and use it for internally storing each module's fds

## Ideas
- [ ] Let contexts talk together? Eg: broadcast(msg, bool global) to send a message to all modules in every context; module_publish message in another context? etc etc

### Submodules (??)
- [ ] SUBMODULE(B, A) calls module_register(B) and module_binds_to(A);
- [ ] SUBMODULE SHOULD BE STARTED later (after all MODULES) -> ctor3 (this way it'll support only level 1 submodules though (ie: a submodule cannot have submodules))
- [ ] destroy children of modules at module deregister
- [ ] bind children to parent states (ie: parent paused -> children paused; parent resumed -> children resumed...)
- [ ] m_forward -> like m_tell but to all children
