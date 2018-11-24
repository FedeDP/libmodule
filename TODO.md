## 3.2.0
- [x] Actually implement a stack for module_become/unbecome
- [x] Expose stack through a stack.h public header
- [x] Add test + doc for stack

## 4.0.0 (?)
- [ ] Prevent other modules from using a module's self_t (as received eg from a PubSub message)
Idea: for each self_t passed as first parameter, check that its address is same as self_t stored in module (?), eg: 
    module_register_fd(self_t **self, ...) { if (self != (*self)->module->trusted_self) { return MOD_NO_PERM; }

- [ ] All API should go through self_t, eg: self_t *recipient = module_ref("foo"); module_tell(self, recipient, ...);
- [ ] Add a module_new_msg(.data=..., .topic=..., ) (https://github.com/mcinglis/c-style#use-structs-to-name-functions-optional-arguments)

## Ideas

- [ ] Let contexts talk together? Eg: broadcast(msg, bool global) to send a message to all modules in every context; module_publish message in another context? etc etc

### Submodules (??)

- [ ] SUBMODULE(B, A) calls module_register(B) and module_binds_to(A);
- [ ] SUBMODULE SHOULD BE STARTED later (after all MODULES) -> ctor3 (this way it'll support only level 1 submodules though (ie: a submodule cannot have submodules))
- [ ] destroy children of modules at module deregister
- [ ] bind children to parent states (ie: parent paused -> children paused; parent resumed -> children resumed...)
- [ ] m_forward -> like m_tell but to all children
