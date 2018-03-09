# TODO

## Make it context aware

- [x] add concept of module_ctx; one can define a module with _MODULE(test, test_ctx) to create a new ctx. A default ctx is created by default ("default")
- [x] map <String, module_ctx> where module_ctx is a map<string, modules>
- [x] don't allow 2 modules with same name in same context -> easily achievable renaming "name()" function to "ctx_name()" func. THis way, different ctx can have same module, but same ctx cannot.
- [x] add a multi-context example
- [x] add a multi module in same source file example
- [ ] SUBMODULE concept? (eg: SUBMODULE(B, A) where B is a submodule of A). When A starts, B is automatically started. When A is stopped, B is automatically stopped. IE: their status is binded
- [ ] Destroy context as soon as modules_ctx_quit() is called -> PRO: free some memory; CONS: you cannot restart looping on this context. (it may happen that one receives an error, leave the loop and then later retries to loop.). One can forcefully destroy a context by deregistering all modules inside it though.

## Generic

- [x] AUR pkgbuild
- [x] modules_pre_start() function that will be run right before modules' constructors?
- [ ] let developers not implement some functions (eg: check/destroy can be useless in some cases)? It should be enough to declare these functions as "weak" and check if pointer is NULL in module.c

## Document it

- [ ] expand README
- [ ] write DOC (readthedocs?)
- [ ] write some tests (cmocka?)
