# TODO

## Make it work

- [x] module_set_userdata() (to be passed to callbacks)
- [x] constify some pointers (eg self_t *self)?
- [x] add a modules_on_error() callback
- [x] actually start modules
- [x] add meaningful example
- [x] Rename some functions (eg: stateChange and check are not very meaningful names)
- [x] Fix module_log()...
- [x] drop m_get_hook and add m_become/unbecome
- [x] rename pollCb in recv; recv to take a message_t
- [x] implement module_stop function. BEFORE a module_destroy any module can be re-started though.
- [x] add a module_deregister function and rename set_module_self to module_register
- [x] automatically call deregister (same way it's done for register)
- [x] correctly force priority: init library, register all modules, deregister modules, destroy library.

- [x] use  __attribute__ ((visibility("default"))
- [x] Should module_deregister be exposed?

- [ ] fix valgrind issues (they're all from invalid read, caused by realloc.)

- [ ] use a jmp_buf quit_buf (longjmp) instead of ctx.quit?

- [ ] Write README
- [ ] codacy
- [ ] travis

## Make it context aware (and much more actor-like)

- [ ] implement m_tell and m_broadcast (use hashmap for modules? <ModName, Module *>) -> eg: m_tell(brightness, "capture") ?
- [ ] add concept of module_ctx; one can define a module with _MODULE(test, test_ctx) to create a new ctx. A default ctx is created by default ("default")
- [ ] map <String, module_ctx> where module_ctx is a map<string, modules>
- [ ] don't allow 2 modules with same name in same context
- [ ] drop stateChange? Why having a stateChange callback to start a module, when module X can call m_tell(Y, "start") to module Y? It may be useful though to achieve automatic dep system 
- [ ] Split examples in "simple API" and "hard API"(ie: api with different ctx)

## Document it

- [ ] write DOC (readthedocs?)
- [ ] write some tests
