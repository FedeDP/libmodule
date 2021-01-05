#pragma once

#ifndef LIBMODULE_STRUCT_H
    #define LIBMODULE_STRUCT_H
#endif
#include "cmn.h"

/** Stack interface **/

/* Callback for stack_iterate */
typedef int (*m_stack_cb)(void *, void *);

/* Fn for stack_set_dtor */
typedef void (*m_stack_dtor)(void *);

/* Incomplete struct declaration for stack */
typedef struct _stack m_stack_t;

/* Incomplete struct declaration for stack iterator */
typedef struct _stack_itr m_stack_itr_t;

_public_ m_stack_t *m_stack_new(const m_stack_dtor fn);
_public_ m_stack_itr_t *m_stack_itr_new(const m_stack_t *s);
_public_ int m_stack_itr_next(m_stack_itr_t **itr);
_public_ int m_stack_itr_remove(m_stack_itr_t *itr);
_public_ void *m_stack_itr_get_data(const m_stack_itr_t *itr);
_public_ int m_stack_itr_set_data(const m_stack_itr_t *itr, void *value);
_public_ size_t m_stack_itr_idx(const m_stack_itr_t *itr);
_public_ int m_stack_iterate(const m_stack_t *s, const m_stack_cb fn, void *userptr);
_public_ int m_stack_push(m_stack_t *s, void *data);
_public_ void *m_stack_pop(m_stack_t *s);
_public_ void *m_stack_peek(const m_stack_t *s);
_public_ int m_stack_clear(m_stack_t *s);
_public_ int m_stack_remove(m_stack_t *s);
_public_ int m_stack_free(m_stack_t **s);
_public_ ssize_t m_stack_length(const m_stack_t *s);
