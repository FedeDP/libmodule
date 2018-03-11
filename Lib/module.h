#pragma once

#include <stdio.h>

/* Convenience macros */

#define MOD_OK      0
#define MOD_ERR     -1

#define _public_    __attribute__ ((visibility("default")))
#define _ctor0_     __attribute__((constructor (101)))
#define _ctor1_     __attribute__((constructor (102)))
#define _dtor0_     __attribute__((destructor (101)))
#define _dtor1_     __attribute__((destructor (102)))

#define _weak_      __attribute__((weak))

#define DEFAULT_CTX "default"

/* Interface Macros */
#define MODULE_CTX(name, ctx) \
    static int init(void); \
    static int check(void); \
    static int evaluate(void); \
    static void recv(message_t *msg, const void *userdata); \
    static void destroy(void); \
    static const void *self = NULL; \
    void _ctor1_ ctx##_##name() { \
        if (check()) { \
            static userhook hook = { init, evaluate, recv, destroy }; \
            module_register(#name, #ctx, &self, &hook); \
        } \
    } \
    void _dtor1_ ctx##_destroy_##name() { module_deregister(&self); }

#define MODULE(name) MODULE_CTX(name, default)
   
/* Defines for easy API (with no need bothering with both self and ctx) */
#define m_is(x)                     module_is(self, x)
#define m_start(fd)                 module_start(self, fd)
#define m_pause()                   module_pause(self)
#define m_resume()                  module_resume(self)
#define m_stop()                    module_stop(self)
#define m_become(x)                 module_become(self, recv_##x)
#define m_unbecome()                module_become(self, recv)
#define m_set_userdata(x)           module_set_userdata(self, x)
#define m_log(fmt, ...)             module_log(self, fmt, ##__VA_ARGS__)

#define modules_on_error(on_error)  modules_ctx_on_error(DEFAULT_CTX, on_error)
#define modules_loop()              modules_ctx_loop(DEFAULT_CTX)
#define modules_quit()              modules_ctx_quit(DEFAULT_CTX)

/** Structs types **/

/* Modules states */
enum module_states { IDLE, RUNNING, PAUSED };

typedef struct {
    int fd;
    const char *message;
    const char *sender;
} message_t;

/* Callbacks typedefs */
typedef void(*error_cb)(const char *error_msg, const char *ctx);
typedef int(*init_cb)(void);
typedef int(*evaluate_cb)(void);
typedef void(*recv_cb)(message_t *msg, const void *userdata);
typedef void(*destroy_cb)(void);

/* Struct that holds user defined callbacks */
typedef struct {
    init_cb init;                           // module's init function (should return a FD)
    evaluate_cb evaluate;                   // module's state changed function
    recv_cb recv;                           // module's recv function
    destroy_cb destroy;                     // module's destroy function
} userhook;

/* Module interface functions */
_public_ int module_register(const char *name, const char *ctx_name, const void **self, userhook *hook);
_public_ int module_deregister(const void **self);
/* FIXME: do not export this for now as its support is not complete */
int module_binds_to(const void *self, const char *parent);
_public_ int module_is(const void *self, const enum module_states st);
_public_ int module_start(const void *self, int fd);
_public_ int module_pause(const void *self);
_public_ int module_resume(const void *self);
_public_ int module_stop(const void *self);
_public_ int module_become(const void *self,  recv_cb new_recv);
_public_ int module_log(const void *self, const char *fmt, ...);
_public_ int module_set_userdata(const void *self, const void *userdata);

/* Modules interface functions */
_public_ void _ctor0_ _weak_ modules_pre_start(void);
_public_ void modules_ctx_on_error(const char *ctx_name, error_cb on_error);
_public_ int modules_ctx_loop(const char *ctx_name);
_public_ void modules_ctx_quit(const char *ctx_name);
