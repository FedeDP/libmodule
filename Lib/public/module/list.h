#pragma once

#include "module_cmn.h"

/** Linked-List interface **/

typedef enum {
    LIST_WRONG_PARAM = -4,
    LIST_MISSING,
    LIST_ERR,
    LIST_OMEM,
    LIST_OK
} list_ret_code;

/* Callback for list_iterate; first parameter is userdata, second is list data */
typedef list_ret_code (*list_cb)(void *, void *);

/* Fn for list_set_dtor */
typedef void (*list_dtor)(void *);

/* Callback for list comapre; first parameter is userdata, second is list data */
typedef int (*list_comp)(void *, void *);

/* Incomplete struct declaration for list */
typedef struct _list list_t;

/* Incomplete struct declaration for list iterator */
typedef struct _list_itr list_itr_t;

_public_ list_t *list_new(const list_dtor fn);
_public_ list_itr_t *list_itr_new(const list_t *l);
_public_ list_itr_t *list_itr_next(list_itr_t *itr);
_public_ void *list_itr_get_data(const list_itr_t *itr);
_public_ list_ret_code list_itr_set_data(list_itr_t *itr, void *value);
_public_ list_ret_code list_itr_insert(list_itr_t *itr, void *value);
_public_ list_ret_code list_itr_remove(list_itr_t *itr);
_public_ list_ret_code list_iterate(const list_t *l, const list_cb fn, void *userptr);
_public_ list_ret_code list_insert(list_t *l, void *data, const list_comp comp);
_public_ list_ret_code list_remove(list_t *l, void *data, const list_comp comp);
_public_ list_ret_code list_clear(list_t *l);
_public_ list_ret_code list_free(list_t *l);
_public_ ssize_t list_length(const list_t *l);

