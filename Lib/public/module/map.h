#pragma once

#include "commons.h"

/** Hashmap interface **/

/* Callback for map_iterate; first parameter is userdata, second and third are {key,value} tuple */
typedef int (*m_map_cb)(void *, const char *, void *);

/* Fn for map_set_dtor */
typedef void (*m_map_dtor)(void *);

/* Incomplete struct declaration for hashmap */
typedef struct _map m_map_t;

/* Incomplete struct declaration for hashmap iterator */
typedef struct _map_itr m_map_itr_t;

typedef enum {
    M_MAP_KEY_DUP           = 0x01,         // Should map keys be dupped?
    M_MAP_KEY_AUTOFREE      = 0x02,         // Should map keys be freed automatically?
    M_MAP_VAL_ALLOW_UPDATE  = 0X04          // Does map object allow for updating values?
} m_map_flags;

_public_ m_map_t *m_map_new(const m_map_flags flags, const m_map_dtor fn);
_public_ m_map_itr_t *m_map_itr_new(const m_map_t *m);
_public_ int m_map_itr_next(m_map_itr_t **itr);
_public_ int m_map_itr_remove(m_map_itr_t *itr);
_public_ const char *m_map_itr_get_key(const m_map_itr_t *itr);
_public_ void *m_map_itr_get_data(const m_map_itr_t *itr);
_public_ int m_map_itr_set_data(const m_map_itr_t *itr, void *value);
_public_ int m_map_iterate(m_map_t *m, const m_map_cb fn, void *userptr);
_public_ int m_map_put(m_map_t *m, const char *key, void *value);
_public_ void *m_map_get(const m_map_t *m, const char *key);
_public_ bool m_map_has_key(const m_map_t *m, const char *key);
_public_ int m_map_remove(m_map_t *m, const char *key);
_public_ int m_map_clear(m_map_t *m);
_public_ int m_map_free(m_map_t **m);
_public_ ssize_t m_map_length(const m_map_t *m);
