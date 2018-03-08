#pragma once

#include <stdio.h>

/* Convenience macros */

#define _public_    __attribute__ ((visibility("default")))
#define _ctor0_      __attribute__((constructor (101)))
#define _ctor1_      __attribute__((constructor (102)))
#define _dtor0_      __attribute__((destructor (101)))
#define _dtor1_      __attribute__((destructor (102)))

#define DEFAULT_CTX "default"

/* Interface Macros */
#define m_log(fmt, ...) module_log(self, fmt, ##__VA_ARGS__)

#define CTX_MODULE(name, ctx) \
    static int init(void); \
    static int check(void); \
    static int evaluate(void); \
    static void recv(message_t *msg, const void *userdata); \
    static void destroy(void); \
    static const void *self = NULL; \
    void _ctor1_ ctx##_##name() { \
        if (!check()) { \
            static userhook hook = { init, evaluate, recv, destroy }; \
            module_register(#name, #ctx, &self, &hook); \
        } \
    } \
    void _dtor1_ ctx##_destroy_##name() { module_deregister(&self); }

#define MODULE(name) CTX_MODULE(name, default)
   
/* Defines for exposed module state getters/setters */
#define m_is(x)                     module_is(self, x)
#define m_start(fd)                 module_start(self, fd)
#define m_pause()                   module_pause(self)
#define m_resume()                  module_resume(self)
#define m_stop()                    module_stop(self)
#define m_become(x)                 module_become(self, recv_##x)
#define m_unbecome()                module_become(self, recv)
#define m_set_userdata(x)           module_set_userdata(self, x)

/* Defines for simple api (without ctx) */
#define modules_on_error(on_error)  modules_ctx_on_error(DEFAULT_CTX, on_error)
#define modules_loop()              modules_ctx_loop(DEFAULT_CTX)
#define modules_quit()              modules_ctx_quit(DEFAULT_CTX)

/** Structs types **/

/* Modules states */
enum module_states { IDLE, RUNNING, PAUSED };

typedef struct {
    int fd;
    const char *message;
    const char *from;
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
_public_ void module_register(const char *name, const char *ctx_name, const void **self, userhook *hook);
_public_ void module_deregister(const void **self);
_public_ void module_log(const void *self, const char *fmt, ...);

_public_ void module_set_userdata(const void *self, const void *userdata);
_public_ int module_is(const void *self, const enum module_states st);
_public_ int module_start(const void *self, int fd);
_public_ int module_pause(const void *self);
_public_ int module_resume(const void *self);
_public_ int module_stop(const void *self);
_public_ void module_become(const void *self,  recv_cb new_recv);

/* Modules interface functions */
_public_ void modules_ctx_on_error(const char *ctx_name, error_cb on_error);
_public_ void modules_ctx_loop(const char *ctx_name);
_public_ void modules_ctx_quit(const char *ctx_name);
