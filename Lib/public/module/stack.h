#pragma once

#include "module_cmn.h"

/** Stack interface **/

/* Callback for stack_iterate */
typedef int (*mod_stack_cb)(void *, void *);

/* Fn for stack_set_dtor */
typedef void (*mod_stack_dtor)(void *);

/* Incomplete struct declaration for stack */
typedef struct _stack mod_stack_t;

/* Incomplete struct declaration for stack iterator */
typedef struct _stack_itr mod_stack_itr_t;

_public_ mod_stack_t *stack_new(const mod_stack_dtor fn);
_public_ mod_stack_itr_t *stack_itr_new(const mod_stack_t *s);
_public_ mod_stack_itr_t *stack_itr_next(mod_stack_itr_t *itr);
_public_ void *stack_itr_get_data(const mod_stack_itr_t *itr);
_public_ int stack_itr_set_data(const mod_stack_itr_t *itr, void *value);
_public_ int stack_iterate(const mod_stack_t *s, const mod_stack_cb fn, void *userptr);
_public_ int stack_push(mod_stack_t *s, void *data);
_public_ void *stack_pop(mod_stack_t *s);
_public_ void *stack_peek(const mod_stack_t *s);
_public_ int stack_clear(mod_stack_t *s);
_public_ int stack_free(mod_stack_t **s);
_public_ ssize_t stack_length(const mod_stack_t *s);
