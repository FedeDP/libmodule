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
} map_ret_code;

/* Callback for map_iterate */
typedef map_ret_code (*map_cb)(void *, const char *, void *);

/* Fn for map_set_dtor */
typedef void (*map_dtor)(void *);

/* Incomplete struct declaration for hashmap */
typedef struct _map map_t;

/* Incomplete struct declaration for hashmap iterator */
typedef struct _map_itr map_itr_t;

_public_ map_t *map_new(const bool keysdup, const map_dtor fn);
_public_ map_itr_t *map_itr_new(const map_t *m);
_public_ map_itr_t *map_itr_next(map_itr_t *itr);
_public_ map_ret_code map_itr_remove(map_itr_t *itr);
_public_ const char *map_itr_get_key(const map_itr_t *itr);
_public_ void *map_itr_get_data(const map_itr_t *itr);
_public_ map_ret_code map_itr_set_data(const map_itr_t *itr, void *value);
_public_ map_ret_code map_iterate(map_t *m, const map_cb fn, void *userptr);
_public_ map_ret_code map_put(map_t *m, const char *key, void *value);
_public_ void *map_get(const map_t *m, const char *key);
_public_ bool map_has_key(const map_t *m, const char *key);
_public_ map_ret_code map_remove(map_t *m, const char *key);
_public_ map_ret_code map_clear(map_t *m);
_public_ map_ret_code map_free(map_t *m);
_public_ ssize_t map_length(const map_t *m);
