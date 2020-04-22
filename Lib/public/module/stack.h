#pragma once

#include "commons.h"

/** Stack interface **/

/* Callback for stack_iterate */
typedef int (*m_stack_cb)(void *, void *);

/* Fn for stack_set_dtor */
typedef void (*m_stack_dtor)(void *);

/* Incomplete struct declaration for stack */
typedef struct _stack m_stack_t;

/* Incomplete struct declaration for stack iterator */
typedef struct _stack_itr m_stack_itr_t;

_public_ m_stack_t *stack_new(const m_stack_dtor fn);
_public_ m_stack_itr_t *stack_itr_new(const m_stack_t *s);
_public_ m_stack_itr_t *stack_itr_next(m_stack_itr_t *itr);
_public_ void *stack_itr_get_data(const m_stack_itr_t *itr);
_public_ int stack_itr_set_data(const m_stack_itr_t *itr, void *value);
_public_ int stack_iterate(const m_stack_t *s, const m_stack_cb fn, void *userptr);
_public_ int stack_push(m_stack_t *s, void *data);
_public_ void *stack_pop(m_stack_t *s);
_public_ void *stack_peek(const m_stack_t *s);
_public_ int stack_clear(m_stack_t *s);
_public_ int stack_free(m_stack_t **s);
_public_ ssize_t stack_length(const m_stack_t *s);
