#pragma once

#include "module.h"

/* Interface Macros */
#define MODULE_CTX(name, ctx) \
static void init(void); \
static bool check(void); \
static bool evaluate(void); \
static void receive(const msg_t *const msg, const void *userdata); \
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
#define m_start()                               module_start(_self)
#define m_pause()                               module_pause(_self)
#define m_resume()                              module_resume(_self)
#define m_stop()                                module_stop(_self)
#define m_become(x)                             module_become(_self, receive_##x)
#define m_unbecome()                            module_become(_self, receive)
#define m_set_userdata(userdata)                module_set_userdata(_self, userdata)
#define m_register_fd(fd, autoclose, data)      module_register_fd(_self, fd, autoclose, data)
#define m_deregister_fd(fd)                     module_deregister_fd(_self, fd)
#define m_log(...)                              module_log(_self, ##__VA_ARGS__)
#define m_register_topic(topic)                 module_register_topic(_self, topic)
#define m_deregister_topic(topic)               module_deregister_topic(_self, topic)
#define m_subscribe(topic)                      module_subscribe(_self, topic)
#define m_unsubscribe(topic)                    module_unsubscribe(_self, topic)
#define m_tell(recipient, msg, size)            module_tell(_self, recipient, msg, size)
#define m_reply(sender, msg, size)              module_reply(_self, sender, msg, size)
#define m_publish(topic, msg, size)             module_publish(_self, topic, msg, size)
#define m_broadcast(msg, size)                  module_broadcast(_self, msg, size)
#define m_tell_str(recipient, msg)              module_tell(_self, recipient, (unsigned char *)msg, strlen(msg))
#define m_reply_str(sender, msg)                module_reply(_self, sender, (unsigned char *)msg, strlen(msg))
#define m_publish_str(topic, msg)               module_publish(_self, topic, (unsigned char *)msg, strlen(msg))
#define m_broadcast_str(msg)                    module_broadcast(_self, (unsigned char *)msg, strlen(msg))
