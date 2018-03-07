#pragma once

#include <stdio.h>

/* Convenience macros */

#define _public_    __attribute__ ((visibility("default")))
#define _ctor0_      __attribute__((constructor (101)))
#define _ctor1_      __attribute__((constructor (102)))
#define _dtor0_      __attribute__((destructor (101)))
#define _dtor1_      __attribute__((destructor (102)))

/* Interface Macros */
#define m_log(fmt, ...) module_log(self, fmt, ##__VA_ARGS__)

#define MODULE(name) \
    static int init(void); \
    static int check(void); \
    static int evaluate(void); \
    static void recv(message_t *msg, const void *userdata); \
    static void destroy(void); \
    static const self_t *self; \
    void _ctor1_ name() { \
        if (!check()) { \
            static userhook hook = { init, evaluate, recv, destroy }; \
            module_register(#name, &self, &hook); \
        } \
    } \
    void _dtor1_ destroy_##name() { module_deregister(self); }

/* Defines for exposed module state getters/setters */
#define m_is(x)             module_is(self, x)
#define m_start(fd)         module_start(self, fd)
#define m_pause()           module_pause(self)
#define m_resume()          module_resume(self)
#define m_stop()            module_stop(self)
#define m_become(x)         module_become(self, recv_##x)
#define m_unbecome()        module_become(self, recv)
#define m_set_userdata(x)   module_set_userdata(self, x)

/** Structs types **/

/* Modules states */
enum module_states { IDLE, STARTED, PAUSED, DESTROYED };

/* Struct that holds self module informations, static to each module */
typedef struct {
    const char *name;                       // module's name
    int id;                                 // module's ID
} self_t;

typedef struct {
    int fd;
    const char *message;
    const char *from;
} message_t;

/* Callbacks typedefs */
typedef void(*error_cb)(const char *error_msg);
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
_public_ void module_register(const char *name, const self_t **self, userhook *hook);
_public_ void module_deregister(const self_t *self);
_public_ void module_log(const self_t *self, const char *fmt, ...);

_public_ void module_set_userdata(const self_t *self, const void *userdata);
_public_ int module_is(const self_t *self, const enum module_states s);
_public_ int module_start(const self_t *self, int fd);
_public_ int module_pause(const self_t *self);
_public_ int module_resume(const self_t *self);
_public_ int module_stop(const self_t *self);
_public_ void module_become(const self_t *self,  recv_cb new_recv);

/* Modules interface functions */
_public_ void modules_on_error(error_cb on_error);
_public_ void modules_loop(void);
_public_ void modules_quit(void);
