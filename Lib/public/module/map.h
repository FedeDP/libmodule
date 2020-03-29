#pragma once

#include "module_cmn.h"

/** Hashmap interface **/

typedef enum {
    MAP_EPERM = -6,
    MAP_WRONG_PARAM,
    MAP_ERR,
    MAP_MISSING,
    MAP_FULL,
    MAP_OMEM,
    MAP_OK
} mod_map_ret;

/* Callback for map_iterate; first parameter is userdata, second and third are {key,value} tuple */
typedef mod_map_ret (*mod_map_cb)(void *, const char *, void *);

/* Fn for map_set_dtor */
typedef void (*mod_map_dtor)(void *);

/* Incomplete struct declaration for hashmap */
typedef struct _map mod_map_t;

/* Incomplete struct declaration for hashmap iterator */
typedef struct _map_itr mod_map_itr_t;

_public_ mod_map_t *map_new(const bool keysdup, const mod_map_dtor fn);
_public_ mod_map_itr_t *map_itr_new(const mod_map_t *m);
_public_ mod_map_itr_t *map_itr_next(mod_map_itr_t *itr);
_public_ mod_map_ret map_itr_remove(mod_map_itr_t *itr);
_public_ const char *map_itr_get_key(const mod_map_itr_t *itr);
_public_ void *map_itr_get_data(const mod_map_itr_t *itr);
_public_ mod_map_ret map_itr_set_data(const mod_map_itr_t *itr, void *value);
_public_ mod_map_ret map_iterate(mod_map_t *m, const mod_map_cb fn, void *userptr);
_public_ mod_map_ret map_put(mod_map_t *m, const char *key, void *value);
_public_ void *map_get(const mod_map_t *m, const char *key);
_public_ bool map_has_key(const mod_map_t *m, const char *key);
_public_ mod_map_ret map_remove(mod_map_t *m, const char *key);
_public_ mod_map_ret map_clear(mod_map_t *m);
_public_ mod_map_ret map_free(mod_map_t **m);
_public_ ssize_t map_length(const mod_map_t *m);
