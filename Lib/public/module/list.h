#pragma once

#include "module_cmn.h"

/** Linked-List interface **/

/* Callback for list_iterate; first parameter is userdata, second is list data */
typedef int (*m_list_cb)(void *, void *);

/* Fn for list_set_dtor */
typedef void (*m_list_dtor)(void *);

/* Callback for list compare; first parameter is userdata, second is list data */
typedef int (*m_list_comp)(void *, void *);

/* Incomplete struct declaration for list */
typedef struct _list m_list_t;

/* Incomplete struct declaration for list iterator */
typedef struct _list_itr m_list_itr_t;

_public_ m_list_t *list_new(const m_list_dtor fn);
_public_ m_list_itr_t *list_itr_new(const m_list_t *l);
_public_ m_list_itr_t *list_itr_next(m_list_itr_t *itr);
_public_ void *list_itr_get_data(const m_list_itr_t *itr);
_public_ int list_itr_set_data(m_list_itr_t *itr, void *value);
_public_ int list_itr_insert(m_list_itr_t *itr, void *value);
_public_ int list_itr_remove(m_list_itr_t *itr);
_public_ int list_iterate(const m_list_t *l, const m_list_cb fn, void *userptr);
_public_ int list_insert(m_list_t *l, void *data, const m_list_comp comp);
_public_ int list_remove(m_list_t *l, void *data, const m_list_comp comp);
_public_ void *list_find(m_list_t *l, void *data, const m_list_comp comp);
_public_ int list_clear(m_list_t *l);
_public_ int list_free(m_list_t **l);
_public_ ssize_t list_length(const m_list_t *l);

