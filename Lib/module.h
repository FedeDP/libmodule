#pragma once

#include <module_cmn.h>

/* Interface Macros */
#define MODULE_CTX(name, ctx) \
    static void init(void); \
    static int check(void); \
    static int evaluate(void); \
    static void receive(const msg_t *msg, const void *userdata); \
    static void destroy(void); \
    static const self_t *self = NULL; \
    static m_dep_t deps; \
    static void _ctor4_ constructor(void) { \
        if (!deps.size) { \
            if (check()) { \
                userhook hook = { init, evaluate, receive, destroy }; \
                module_register(name, ctx, &self, &hook); \
            } \
        } else { \
            module_enqueue_deps(name, &deps); \
        } \
    } \
    static void _dtor1_ destructor(void) { module_deregister(&self); } \
    static void _ctor2_ module_pre_start(void)

#define MODULE(name)        MODULE_CTX(name, DEFAULT_CTX)

#define ADD_VARIADIC_DEPS(t, ...) \
do { \
    char *_arr_[] = { __VA_ARGS__ }; \
    int i; \
    for (i = 0; i < sizeof(_arr_)/sizeof(char *); i++) { \
        deps.list = realloc(deps.list, (i + 1) * sizeof(m_dep)); \
        deps.list[i].name = strdup(_arr_[i]); \
        deps.list[i].type = t; \
    } \
    deps.size += i; \
    deps.fn = constructor; \
} while(0)

/* Ctor3 as they will be started before module constructor */
#define REQUIRE(...)    static void _ctor3_ set_deps_hard(void) { ADD_VARIADIC_DEPS(HARD, ##__VA_ARGS__); }
#define AFTER(...)      static void _ctor3_ set_deps_soft(void) { ADD_VARIADIC_DEPS(SOFT, ##__VA_ARGS__); }

/* Defines for easy API (with no need bothering with both self and ctx) */
#define m_is(state)                             module_is(self, state)
#define m_start(fd)                             module_start(self)
#define m_pause()                               module_pause(self)
#define m_resume()                              module_resume(self)
#define m_stop()                                module_stop(self)
#define m_become(x)                             module_become(self, receive_##x)
#define m_unbecome()                            module_become(self, receive)
#define m_set_userdata(userdata)                module_set_userdata(self, userdata)
#define m_add_fd(fd)                            module_add_fd(self, fd)
#define m_rm_fd(fd, close_fd)                   module_rm_fd(self, fd, close_fd)
#define m_update_fd(old, new, close_old)        module_update_fd(self, old, new, close_old)
#define m_log(...)                              module_log(self, ##__VA_ARGS__)
#define m_subscribe(topic)                      module_subscribe(self, topic)
#define m_tell(recipient, msg)                  module_tell(self, recipient, msg)
#define m_publish(topic, msg)                   module_publish(self, topic, msg)
#define m_broadcast(msg)                        module_publish(self, NULL, msg)

/* Module interface functions */

#ifdef __cplusplus
extern "C" {
#endif

/* Module registration */
_public_ module_ret_code module_register(const char *name, const char *ctx_name, const self_t **self, userhook *hook);
_public_ module_ret_code module_deregister(const self_t **self);
/* Do not export this function for now as its support is not complete */
module_ret_code module_binds_to(const self_t *self, const char *parent);
_public_ module_ret_code module_enqueue_deps(const char *name, const m_dep_t *deps);

/* Module state getters */
_public_ int module_is(const self_t *self, const enum module_states st);

/* Module state setters */
_public_ module_ret_code module_start(const self_t *self);
_public_ module_ret_code module_pause(const self_t *self);
_public_ module_ret_code module_resume(const self_t *self);
_public_ module_ret_code module_stop(const self_t *self);

/* Module generic functions */
_public_ module_ret_code module_become(const self_t *self,  recv_cb new_recv);
_public_ module_ret_code module_log(const self_t *self, const char *fmt, ...);
_public_ module_ret_code module_set_userdata(const self_t *self, const void *userdata);
_public_ module_ret_code module_add_fd(const self_t *self, int fd);
_public_ module_ret_code module_rm_fd(const self_t *self, int fd, int close_fd);
_public_ module_ret_code module_update_fd(const self_t *self, int old_fd, int new_fd, int close_old);

/* Module PubSub interface */
_public_ module_ret_code module_subscribe(const self_t *self, const char *topic);
_public_ module_ret_code module_tell(const self_t *self, const char *recipient, const char *message);
_public_ module_ret_code module_publish(const self_t *self, const char *topic, const char *message);

#ifdef __cplusplus
}
#endif
