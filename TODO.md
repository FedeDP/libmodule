# TODO

## Make it work

- [ ] fix valgrind issues (they're all from invalid read, caused by realloc; fixable with a linked list or switching to hashmap)

- [ ] use a jmp_buf quit_buf (longjmp) instead of ctx.quit?

- [x] Write README
- [x] codacy
- [x] travis

## Make it context aware (and much more actor-like)

- [ ] implement m_tell and m_broadcast (use hashmap for modules? <ModName, Module *>) -> eg: m_tell(brightness, "capture")? I'm not sure, this should go on a actor lib
- [ ] add concept of module_ctx; one can define a module with _MODULE(test, test_ctx) to create a new ctx. A default ctx is created by default ("default")
- [ ] map <String, module_ctx> where module_ctx is a map<string, modules>
- [ ] don't allow 2 modules with same name in same context
- [ ] drop stateChange? Why having a stateChange callback to start a module, when module X can call m_tell(Y, "start") to module Y? It may be useful though to achieve automatic dep system 
- [ ] Split examples in "simple API" and "hard API" (ie: api with different ctx)

## Document it

- [ ] expand README
- [ ] write DOC (readthedocs?)
- [ ] write some tests
