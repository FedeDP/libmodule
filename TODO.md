# TODO

## Make it work

- [x] fix valgrind issues (they're all from invalid read, caused by realloc; fixable with a linked list or switching to hashmap)
- [x] switch to hashmap for modules
- [x] self_t completely opaque to clients (void *)

- [ ] use a jmp_buf quit_buf (longjmp) instead of ctx.quit?

- [x] Write README
- [x] codacy
- [x] travis
- [x] use cmake (generate pkg-config script?)
- [x] add pkg-config script
- [x] add a Sample folder readme and makefile

## Make it context aware

- [x] add concept of module_ctx; one can define a module with _MODULE(test, test_ctx) to create a new ctx. A default ctx is created by default ("default")
- [x] map <String, module_ctx> where module_ctx is a map<string, modules>
- [x] don't allow 2 modules with same name in same context -> easily achievable renaming "name()" function to "ctx_name()" func. THis way, different ctx can have same module, but same ctx cannot.
- [ ] add a multi-context example
- [ ] add a multi module in same source file example
- [ ] SUBMODULE concept? (eg: SUBMODULE(B, A) where B is a submodule of A). When A starts, B is automatically started. When A is stopped, B is automatically stopped. IE: their status is binded

## Document it

- [ ] expand README
- [ ] write DOC (readthedocs?)
- [ ] write some tests (cmocka?)
