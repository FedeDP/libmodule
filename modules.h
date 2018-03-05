#pragma once

#include <stdio.h>

#define MODULE(name) \
    static void init(); \
    static int check(); \
    static int state_change(); \
    static void callback(); \
    static void destroy(); \
    static self_t *self; \
    void __attribute__((constructor)) name() { self = modules_set_self(#name, init, check, state_change, callback, destroy); }

/* Struct that holds self module informations, static to each module */
typedef struct {
    const char *name;                     // module's name
    int id;                               // module's ID
} self_t;

typedef void(*mod_void_cb)(void); 
typedef int(*mod_bool_cb)(void);

self_t *modules_set_self(const char *name, mod_void_cb init, mod_bool_cb check, 
                      mod_bool_cb state_change, mod_void_cb callback, mod_void_cb destroy);
void modules_loop(void);
