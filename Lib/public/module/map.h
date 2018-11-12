#pragma once

#include "module_cmn.h"

/** Module hashmap interface **/

typedef enum {
    MAP_ERR = -4,
    MAP_MISSING,
    MAP_FULL,
    MAP_OMEM,
    MAP_OK
} map_ret_code;

/* Callback for map_iterate */
typedef map_ret_code (*map_cb)(void *, void *);

/* Incomplete struct declaration for hashmap */
typedef struct _map map_t;

#ifdef __cplusplus
extern "C"{
#endif

_public_ map_t *map_new(void);
_public_ map_ret_code map_iterate(map_t *m, const map_cb fn, void *userptr);
_public_ map_ret_code map_put(map_t *m, const char *key, void *value, const bool dupkey);
_public_ map_ret_code map_get(const map_t *m, const char *key, void **arg);
_public_ map_ret_code map_remove(map_t *m, const char *key);
_public_ map_ret_code map_free(map_t *m);
_public_ int map_length(const map_t *m);

#ifdef __cplusplus
}
#endif
