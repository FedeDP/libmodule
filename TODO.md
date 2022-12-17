## 6.0.0

### TODO

#### Ctx

- [ ] drop `m_ctx_default` api

#### DOC

- [x] Fully rewrite documentation per-namespace
- [ ] Document that m_{mod,ctx}_deregister() should not be called inside user hook { on_start(), on_stop(), on_eval() } functions; m_mod_deregister() can be used from on_evt() though.  (IS THIS WHOLE SENTENCE TRUE?)
- [ ] document m_evt_t memref'd behaviour!!!
- [ ] Document stats and thresh activity_freq (num_action_per_ms)
- [ ] fix readthedocs on PR?

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

- [ ] m_mod_subscribe to kernel hooks (like uretprobe/uprobe//kprobe/tracepoint)?

### Remote API

- [ ] M_MOD_REMOTE_SRV(name, port, certificate) to create a remote module that listens on port X eventually with key Y 
- [ ] M_MOD_REMOTE_CL(name, ip:port, certificate) to create a module that wraps a remote module and acts as a router
- [ ] Use https://github.com/babelouest/ulfius ? (can a fd can be fetched from it and polled async in a context's loop (as an additional module fd)?)

### Submodules

- [ ] M_SUBM(B, A) checks if A is registered, then registers B and calls m_mod_bind_to(A);
- [ ] Else, stores "B" in a map <"A","B"> to be later retrieved while registering "A"  
- [ ] deregister children at parent deregister
- [ ] bind children to parent states (ie: parent paused -> children paused; parent resumed -> children resumed...)
- [ ] m_forward -> like m_tell but to all children
