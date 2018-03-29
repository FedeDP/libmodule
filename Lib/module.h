#pragma once

#include <limits.h>
#include <module_cmn.h>

/* Convenience macros */
#define MODULE_DONT_POLL INT_MIN

/* Interface Macros */
#define MODULE_CTX(name, ctx) \
    static void _ctor2_ module_pre_start(void); \
    static int init(void); \
    static int check(void); \
    static int evaluate(void); \
    static void receive(const msg_t *msg, const void *userdata); \
    static void destroy(void); \
    static const self_t *self = NULL; \
    static void _ctor3_ constructor(void) { \
        if (check()) { \
            static userhook hook = { init, evaluate, receive, destroy }; \
            module_register(name, ctx, &self, &hook); \
        } \
    } \
    static void _dtor1_ destructor(void) { module_deregister(&self); }

#define MODULE(name) MODULE_CTX(name, DEFAULT_CTX)
   
/* Defines for easy API (with no need bothering with both self and ctx) */
#define m_is(state)                     module_is(self, state)
#define m_start(fd)                     module_start(self, fd)
#define m_pause()                       module_pause(self)
#define m_resume()                      module_resume(self)
#define m_stop()                        module_stop(self)
#define m_become(x)                     module_become(self, receive_##x)
#define m_unbecome()                    module_become(self, receive)
#define m_set_userdata(userdata)        module_set_userdata(self, userdata)
#define m_update_fd(fd, close_old)      module_update_fd(self, fd, close_old)
#define m_log(fmt, ...)                 module_log(self, fmt, ##__VA_ARGS__)
#define m_subscribe(topic)              module_subscribe(self, topic)
#define m_tell(recipient, msg)          module_tell(self, recipient, msg)
#define m_publish(topic, msg)           module_publish(self, topic, msg)
#define m_broadcast(msg)                module_publish(self, NULL, msg)

/* Module interface functions */

/* Module registration */
_public_ module_ret_code module_register(const char *name, const char *ctx_name, const self_t **self, userhook *hook);
_public_ module_ret_code module_deregister(const self_t **self);
/* Do not export this function for now as its support is not complete */
module_ret_code module_binds_to(const self_t *self, const char *parent);

/* Module state getters */
_public_ int module_is(const self_t *self, const enum module_states st);

/* Module state setters */
_public_ module_ret_code module_start(const self_t *self, int fd);
_public_ module_ret_code module_pause(const self_t *self);
_public_ module_ret_code module_resume(const self_t *self);
_public_ module_ret_code module_stop(const self_t *self);

/* Module generic functions */
_public_ module_ret_code module_become(const self_t *self,  recv_cb new_recv);
_public_ module_ret_code module_log(const self_t *self, const char *fmt, ...);
_public_ module_ret_code module_set_userdata(const self_t *self, const void *userdata);
_public_ module_ret_code module_update_fd(const self_t *self, int new_fd, int close_old);

/* Module PubSub interface */
_public_ module_ret_code module_subscribe(const self_t *self, const char *topic);
_public_ module_ret_code module_tell(const self_t *self, const char *recipient, const char *message);
_public_ module_ret_code module_publish(const self_t *self, const char *topic, const char *message);
