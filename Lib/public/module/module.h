#pragma once

#include "module_cmn.h"

/* Interface Macros */
#define MODULE_CTX(name, ctx) \
    static void init(void); \
    static int check(void); \
    static int evaluate(void); \
    static void receive(const msg_t *msg, const void *userdata); \
    static void destroy(void); \
    static const self_t *_self = NULL; \
    static void _ctor3_ constructor(void) { \
        if (check()) { \
            userhook hook = { init, evaluate, receive, destroy }; \
            module_register(name, ctx, &_self, &hook); \
        } \
    } \
    static void _dtor1_ destructor(void) { module_deregister(&_self); } \
    static void _ctor2_ module_pre_start(void)

#define MODULE(name) MODULE_CTX(name, MODULE_DEFAULT_CTX)

/* Defines for easy API (with no need bothering with both _self and ctx) */
#define m_is(state)                             module_is(_self, state)
#define m_start(fd)                             module_start(_self)
#define m_pause()                               module_pause(_self)
#define m_resume()                              module_resume(_self)
#define m_stop()                                module_stop(_self)
#define m_become(x)                             module_become(_self, receive_##x)
#define m_unbecome()                            module_become(_self, receive)
#define m_set_userdata(userdata)                module_set_userdata(_self, userdata)
#define m_add_fd(fd)                            module_add_fd(_self, fd)
#define m_rm_fd(fd, close_fd)                   module_rm_fd(_self, fd, close_fd)
#define m_update_fd(old, new, close_old)        module_update_fd(_self, old, new, close_old)
#define m_log(...)                              module_log(_self, ##__VA_ARGS__)
#define m_subscribe(topic)                      module_subscribe(_self, topic)
#define m_tell(recipient, msg)                  module_tell(_self, recipient, msg)
#define m_reply(sender, msg)                    module_reply(_self, sender, msg)
#define m_publish(topic, msg)                   module_publish(_self, topic, msg)
#define m_broadcast(msg)                        module_publish(_self, NULL, msg)

/* Module interface functions */

#ifdef __cplusplus
extern "C"{
#endif

/* Module registration */
_public_ module_ret_code module_register(const char *name, const char *ctx_name, const self_t **self, userhook *hook);
_public_ module_ret_code module_deregister(const self_t **self);

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
_public_ module_ret_code module_get_name(const self_t *mod_self, char **name);
_public_ module_ret_code module_get_context(const self_t *mod_self, char **ctx);

/* Module PubSub interface */
_public_ module_ret_code module_subscribe(const self_t *self, const char *topic);
_public_ module_ret_code module_tell(const self_t *self, const char *recipient, const char *message);
_public_ module_ret_code module_reply(const self_t *self, const self_t *sender, const char *message);
_public_ module_ret_code module_publish(const self_t *self, const char *topic, const char *message);

#ifdef __cplusplus
}
#endif
