#pragma once

#include <stdio.h>
#include <limits.h>

/* Convenience macros */

#define MODULE_DONT_POLL INT_MIN

#define _public_    __attribute__ ((visibility("default")))

/* 
 * ctors order: 
 * 1) modules_pre_start() + modules_init();
 * 2) each module_pre_start()
 * 3) each module ctor function
 */
#define _ctor0_     __attribute__((constructor (101)))
#define _ctor1_     __attribute__((constructor (102)))
#define _ctor2_     __attribute__((constructor (103)))
#define _dtor0_     __attribute__((destructor (101)))
#define _dtor1_     __attribute__((destructor (102)))

#define _weak_      __attribute__((weak))

#define DEFAULT_CTX "default"

/* Interface Macros */
#define MODULE_CTX(name, ctx) \
    static int init(void); \
    static int check(void); \
    static int evaluate(void); \
    static void recv(msg_t *msg, const void *userdata); \
    static void destroy(void); \
    static const self_t *self = NULL; \
    static void _ctor2_ constructor(void) { \
        if (check()) { \
            static userhook hook = { init, evaluate, recv, destroy }; \
            module_register(name, ctx, &self, &hook); \
        } \
    } \
    static void _dtor1_ destructor(void) { module_deregister(&self); }

#define MODULE(name) MODULE_CTX(name, DEFAULT_CTX)

#define MODULE_PRE_START() static void _ctor1_ module_pre_start(void)
   
/* Defines for easy API (with no need bothering with both self and ctx) */
#define m_is(x)                     module_is(self, x)
#define m_start(fd)                 module_start(self, fd)
#define m_pause()                   module_pause(self)
#define m_resume()                  module_resume(self)
#define m_stop()                    module_stop(self)
#define m_become(x)                 module_become(self, recv_##x)
#define m_unbecome()                module_become(self, recv)
#define m_set_userdata(userdata)    module_set_userdata(self, userdata)
#define m_update_fd(fd, close_old)  module_update_fd(self, fd, close_old)
#define m_log(fmt, ...)             module_log(self, fmt, ##__VA_ARGS__)
#define m_subscribe(topic)          module_subscribe(self, topic)
#define m_tell(msg, recipient)      module_tell(self, msg, recipient)
#define m_publish(topic, msg)       module_publish(self, topic, msg)
#define m_broadcast(msg)            module_publish(self, NULL, msg)

#define modules_loop()              modules_ctx_loop(DEFAULT_CTX)
#define modules_quit()              modules_ctx_quit(DEFAULT_CTX)

/** Structs types **/

/* Incomplete structure declaration to self handler */
typedef struct _self self_t;

/* Modules states */
enum module_states { IDLE = 0x1, RUNNING = 0x2, PAUSED = 0x4, STOPPED = 0x8 };

typedef enum {
    MOD_WRONG_STATE = -6,
    MOD_NO_PARENT,
    MOD_NO_CTX,
    MOD_NO_MOD,
    MOD_NO_SELF,
    MOD_ERR,
    MOD_OK
} module_ret_code;

typedef struct {
    const char *topic;
    const char *message;
    const char *sender;
} pubsub_msg_t;

typedef struct {
    int fd;
    pubsub_msg_t *message;
} msg_t;

/* Callbacks typedefs */
typedef int(*init_cb)(void);
typedef int(*evaluate_cb)(void);
typedef void(*recv_cb)(msg_t *msg, const void *userdata);
typedef void(*destroy_cb)(void);

/* Struct that holds user defined callbacks */
typedef struct {
    init_cb init;                           // module's init function (should return a FD)
    evaluate_cb evaluate;                   // module's state changed function
    recv_cb recv;                           // module's recv function
    destroy_cb destroy;                     // module's destroy function
} userhook;

/* Module interface functions */
_public_ module_ret_code module_register(const char *name, const char *ctx_name, const self_t **self, userhook *hook);
_public_ module_ret_code module_deregister(const self_t **self);
/* FIXME: do not export this for now as its support is not complete */
module_ret_code module_binds_to(const self_t *self, const char *parent);
_public_ int module_is(const self_t *self, const enum module_states st);
_public_ module_ret_code module_start(const self_t *self, int fd);
_public_ module_ret_code module_pause(const self_t *self);
_public_ module_ret_code module_resume(const self_t *self);
_public_ module_ret_code module_stop(const self_t *self);
_public_ module_ret_code module_become(const self_t *self,  recv_cb new_recv);
_public_ module_ret_code module_log(const self_t *self, const char *fmt, ...);
_public_ module_ret_code module_set_userdata(const self_t *self, const void *userdata);
_public_ module_ret_code module_update_fd(const self_t *self, int new_fd, int close_old);
_public_ module_ret_code module_subscribe(const self_t *self, const char *topic);
_public_ module_ret_code module_tell(const self_t *self, const char *message, const char *recipient);
_public_ module_ret_code module_publish(const self_t *self, const char *topic, const char *message);

/* Modules interface functions */
_public_ void _ctor0_ _weak_ modules_pre_start(void);
_public_ module_ret_code modules_ctx_loop(const char *ctx_name);
_public_ module_ret_code modules_ctx_quit(const char *ctx_name);
