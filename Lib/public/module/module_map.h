#pragma once

#include "module_cmn.h"

/* Module hashmap interface */

typedef enum {
    MAP_ERR = -4,
    MAP_MISSING,
    MAP_FULL,
    MAP_OMEM,
    MAP_OK
} module_map_code;

/* Callback for module_map_iterate */
typedef module_map_code (*map_cb)(void *, void *);

/* Incomplete struct declaration for hashmap */
typedef struct _map map_t;

#ifdef __cplusplus
extern "C"{
#endif

_public_ map_t *module_map_new(void);
_public_ module_map_code module_map_iterate(map_t *m, const map_cb f, void *item);
_public_ module_map_code module_map_put(map_t *m, const char *key, void *value);
_public_ module_map_code module_map_get(const map_t *m, const char *key, void **arg);
_public_ module_map_code module_map_remove(map_t *m, const char *key);
_public_ module_map_code module_map_free(map_t *m);
_public_ int module_map_length(const map_t *m);

#ifdef __cplusplus
}
#endif
