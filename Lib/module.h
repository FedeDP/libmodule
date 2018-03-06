#pragma once

#include <stdio.h>

/* Interface Macros */
#define module_log(fmt, ...); \
    do { \
        printf("%s: ", self->name); \
        printf(fmt, ##__VA_ARGS__); \
    } while(0)

#define MODULE(name) \
    static void init(); \
    static int check(); \
    static int state_change(); \
    static void callback(); \
    static void destroy(); \
    static const self_t *self; \
    void __attribute__((constructor)) name() { \
        static userhook hook = { init, check, state_change, callback, destroy }; \
        self = module_set_self(#name, &hook); }

/* Defines for exposed module state getters/setters */
#define m_is(x)         module_is(self, x)
#define m_pause()       module_pause(self)
#define m_resume()      module_resume(self)
#define m_get_hook()    module_get_hook(self)

/** Structs types **/

/* Modules states */
enum module_states { IDLE, DISABLED, STARTED, PAUSED, DESTROYED };

/* Struct that holds self module informations, static to each module */
typedef struct {
    const char *name;                     // module's name
    int id;                               // module's ID
} self_t;

/* Struct that holds user defined callbacks */
typedef struct {
    void (*init)(void);                   // module's init function
    int (*check)(void);                   // module's check-before-init 
    int (*stateChange)(void);             // module's state changed function
    void (*destroy)(void);                // module's destroy function
    void (*pollCb)(void);                 // module's poll callback
} userhook;

/** Interface functions */
const self_t *module_set_self(const char *name, userhook *hook);
void modules_loop(void);

int module_is(const self_t *self, const enum module_states s);
void module_pause(const self_t *self);
void module_resume(const self_t *self);
userhook *const module_get_hook(const self_t *self);
