#pragma once

#include <stdio.h>

/* 
 * ctors order: 
 * 1) modules_pre_start()
 * 2) modules_init()
 * 3) each module_pre_start()
 * 4) each module ctor function
 */
#define _ctor0_     __attribute__((constructor (101)))
#define _ctor1_     __attribute__((constructor (102)))
#define _ctor2_     __attribute__((constructor (103)))
#define _ctor3_     __attribute__((constructor (104)))
#define _dtor0_     __attribute__((destructor (101)))
#define _dtor1_     __attribute__((destructor (102)))

#define _weak_      __attribute__((weak))
#define _public_    __attribute__ ((visibility("default")))

#define DEFAULT_CTX "default"

/** Structs types **/

/* Incomplete structure declaration to self handler */
typedef struct _self self_t;

/* Modules states */
enum module_states { IDLE = 0x1, RUNNING = 0x2, PAUSED = 0x4, STOPPED = 0x8 };

typedef struct {
    const char *topic;
    const char *message;
    const char *sender;
} pubsub_msg_t;

typedef struct {
    const int fd;
    const pubsub_msg_t *msg;
} msg_t;

/* Callbacks typedefs */
typedef int(*init_cb)(void);
typedef int(*evaluate_cb)(void);
typedef void(*recv_cb)(const msg_t *msg, const void *userdata);
typedef void(*destroy_cb)(void);

/* Logger callback */
typedef void(*log_cb)(const char *module_name, const char *context_name, 
                      const char *fmt, va_list args, const void *userdata);

/* Struct that holds user defined callbacks */
typedef struct {
    init_cb init;                           // module's init function (should return a FD)
    evaluate_cb evaluate;                   // module's state changed function
    recv_cb recv;                           // module's recv function
    destroy_cb destroy;                     // module's destroy function
} userhook;

/* Module return codes */
typedef enum {
    MOD_WRONG_STATE = -6,
    MOD_NO_PARENT,
    MOD_NO_CTX,
    MOD_NO_MOD,
    MOD_NO_SELF,
    MOD_ERR,
    MOD_OK
} module_ret_code;
