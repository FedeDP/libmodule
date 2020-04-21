#pragma once

#include "module_cmn.h"

/** Linked-List interface **/

/* Callback for tree_iterate; first parameter is userdata, second is tree data */
typedef int (*m_btree_cb)(void *, void *);

/* Fn for tree_set_dtor */
typedef void (*m_btree_dtor)(void *);

/* Callback for tree compare; first parameter is userdata, second is tree data */
typedef int (*m_btree_cmp)(void *, void *);

/* Incomplete struct declaration for tree */
typedef struct _btree m_btree_t;

/* Incomplete struct declaration for tree iterator */
typedef struct _btree_itr m_btree_itr_t;

typedef enum {
    M_BTREE_PRE,
    M_BTREE_POST,
    M_BTREE_IN
} m_btree_order;

_public_ m_btree_t *m_btree_new(const m_btree_cmp comp, const m_btree_dtor fn);
_public_ int m_btree_insert(m_btree_t *l, void *data);
_public_ int m_btree_remove(m_btree_t *l, void *data);
_public_ void *m_btree_find(m_btree_t *l, void *data);
_public_ int m_btree_traverse(m_btree_t *l, m_btree_order type, m_btree_cb cb, void *userptr);
_public_ m_btree_itr_t *m_btree_itr_new(const m_btree_t *l);
_public_ m_btree_itr_t *m_btree_itr_next(m_btree_itr_t *itr);
_public_ void *m_btree_itr_get_data(const m_btree_itr_t *itr);
_public_ int m_btree_itr_remove(m_btree_itr_t *itr);
_public_ int m_btree_clear(m_btree_t *l);
_public_ int m_btree_free(m_btree_t **l);
_public_ ssize_t m_btree_length(const m_btree_t *l);


