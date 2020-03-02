#pragma once

#include "module_cmn.h"

/** Linked-List interface **/

typedef enum {
    LIST_WRONG_PARAM = -4,
    LIST_MISSING,
    LIST_ERR,
    LIST_OMEM,
    LIST_OK
} mod_list_ret;

/* Callback for list_iterate; first parameter is userdata, second is list data */
typedef mod_list_ret (*mod_list_cb)(void *, void *);

/* Fn for list_set_dtor */
typedef void (*mod_list_dtor)(void *);

/* Callback for list compare; first parameter is userdata, second is list data */
typedef int (*mod_list_comp)(void *, void *);

/* Incomplete struct declaration for list */
typedef struct _list mod_list_t;

/* Incomplete struct declaration for list iterator */
typedef struct _list_itr mod_list_itr_t;

_public_ mod_list_t *list_new(const mod_list_dtor fn);
_public_ mod_list_itr_t *list_itr_new(const mod_list_t *l);
_public_ mod_list_itr_t *list_itr_next(mod_list_itr_t *itr);
_public_ void *list_itr_get_data(const mod_list_itr_t *itr);
_public_ mod_list_ret list_itr_set_data(mod_list_itr_t *itr, void *value);
_public_ mod_list_ret list_itr_insert(mod_list_itr_t *itr, void *value);
_public_ mod_list_ret list_itr_remove(mod_list_itr_t *itr);
_public_ mod_list_ret list_iterate(const mod_list_t *l, const mod_list_cb fn, void *userptr);
_public_ mod_list_ret list_insert(mod_list_t *l, void *data, const mod_list_comp comp);
_public_ mod_list_ret list_remove(mod_list_t *l, void *data, const mod_list_comp comp);
_public_ mod_list_ret list_clear(mod_list_t *l);
_public_ mod_list_ret list_free(mod_list_t *l);
_public_ ssize_t list_length(const mod_list_t *l);

