## 6.0.0

### TODO

#### Ctx

- [x] store ctxs in thread local storage (pthread_setspecific API)? https://pubs.opengroup.org/onlinepubs/009695399/functions/pthread_key_create.html `__find_thread_by_id` and loop over /proc/self/task? -> https://stackoverflow.com/questions/3707358/get-all-the-thread-id-created-with-pthread-created-within-an-process
- [ ] we could then drop: (since every context is thread specific data)
- - [x] global ctx map and mutex
- - [ ] m_mod_ctx() api
- - [x] m_ctx_ref() api
- - [x] m_mod_ref() api should become m_mod_lookup() and let users manage its lifecyle (ie: m_mem_ref it if needed)
- - [ ] m_mod_t->ctx field -> this would help us enforce that module API is called by same thread that registered a context
- - [ ] drop m_ctx_t param from funtion calls
- - [x] drop libmodule_init() and deinit() constructors
- - [x] move ctor defines into mod_easy.h
- - [ ] Specify that m_set_memhook should be called before allocating any libmodule related resource (ie: m_on_boot() for mod_easy, or before allocating first ctx, when manually)
- [x] and just add an m_ctx() API that returns ctx associated with current thread
- [x] store in ctx a `void **curr_mod` that points to the module whose callback is currently being processed, if any, or NULL; when NULL; we are outside a module callback, therefore we can return it; else, we must guarantee that module has permissions to access its ctx
- [ ] Downside: how could we guarantee that 2 ctx with same name do not exist? We cannot; is this a limitation, actually?

#### DOC

- [x] Fully rewrite documentation per-namespace
- [ ] Document that m_ctx_deregister() cannot be called on a looping context (`M_PARAM_ASSERT(c && *c && (*c)->state == M_CTX_IDLE);`)
- [ ] document m_evt_t memref'd behaviour!!!
- [ ] Document stats and thresh activity_freq (num_action_per_ms)

## 6.1.0 (7.0.0?)

### Liburing

- [ ] Follow progress and possibly make some fixes
- [ ] Fix examples: 
  - [ ] Easy sample goes mad with STDIN_FILENO registered (it requires "enter" to be pressed before receiving any event)
  - [ ] Poll sample does not work

### Thread-safe (?)

https://www.gnu.org/software/libc/manual/html_node/Pipe-Atomicity.html

- [ ] Improve multithread support
- [ ] As writing an address in a pipe is atomic, may be each call on a module can be an "op" written to its pipe
- [ ] All modules have a cmd_piped_fd; calling eg: m_mod_become(X) would internally be translated to "m_cmd_enqueue(M_BECOME, X)" and that would be atomic;
  then module reads from cmd_piped_fd and executes the requested op
- [ ] need a way to map m_mod_ API arguments though
- [ ] offer an api to run module's recv on their own thread (register flag: M_MOD_THREADED), it means their receive() will be run async (in a thpool)

### Replay API

- [ ] Publish to require a m_mem_t. Thus we have access to object size.
- [ ] add a m_replay() api to replay any message received in a previous run; M_SRC_TYPE_FD cannot be stored...
- [ ] add a m_evt_t flag that tells if we are running from a replay  
- [ ] add a "--m.replay-from" cmdline switch to default main
- [ ] Basic flow: normally start program and when start looping, before actually polling, flush all messages loaded from file/db
- [ ] Then, if "--m.store" is enabled, store any message received while looping

### Generic

- [ ] Fix m_src_flags with 64b values (right now there is no value over 32b thus it is not a real issue)

## Ideas

### Srcs

- [ ] new kernel hooks (like uretprobe/uprobe//kprobe/tracepoint) src?
- [ ] new remote src? (like a webhook?)

### Remote API

- [ ] M_MOD_REMOTE_SRV(name, port, certificate) to create a remote module that listens on port X eventually with key Y 
- [ ] M_MOD_REMOTE_CL(name, ip:port, certificate) to create a module that wraps a remote module and acts as a router
- [ ] Use https://github.com/babelouest/ulfius ? (can a fd be fetched from it and polled async in a context's loop (as an additional module fd)?)

### Submodules

- [ ] M_SUBM(B, A) checks if A is registered, then registers B and calls m_mod_bind_to(A);
- [ ] Else, stores "B" in a map <"A","B"> to be later retrieved while registering "A"  
- [ ] deregister children at parent deregister
- [ ] bind children to parent states (ie: parent paused -> children paused; parent resumed -> children resumed...)
- [ ] m_forward -> like m_tell but to all children
