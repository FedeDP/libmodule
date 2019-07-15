#pragma once

#include "module_cmn.h"

/** Stack interface **/

typedef enum {
    STACK_WRONG_PARAM = -4,
    STACK_MISSING,
    STACK_ERR,
    STACK_OMEM,
    STACK_OK
} stack_ret_code;

/* Callback for stack_iterate */
typedef stack_ret_code (*stack_cb)(void *, void *);

/* Fn for stack_set_dtor */
typedef void (*stack_dtor)(void *);

/* Incomplete struct declaration for stack */
typedef struct _stack stack_t;

/* Incomplete struct declaration for stack iterator */
typedef struct _stack_itr stack_itr_t;

#ifdef __cplusplus
extern "C"{
#endif

_public_ stack_t *stack_new(void);
_public_ stack_itr_t *stack_itr_new(const stack_t *s);
_public_ stack_itr_t *stack_itr_next(stack_itr_t *itr);
_public_ void *stack_itr_get_data(const stack_itr_t *itr);
_public_ stack_ret_code stack_itr_set_data(const stack_itr_t *itr, void *value);
_public_ stack_ret_code stack_iterate(const stack_t *s, const stack_cb fn, void *userptr);
_public_ stack_ret_code stack_push(stack_t *s, void *data, bool autofree);
_public_ void *stack_pop(stack_t *s);
_public_ void *stack_peek(const stack_t *s);
_public_ stack_ret_code stack_clear(stack_t *s);
_public_ stack_ret_code stack_free(stack_t *s);
_public_ int stack_length(const stack_t *s);
_public_ stack_ret_code stack_set_dtor(stack_t *s, stack_dtor fn);

#ifdef __cplusplus
}
#endif
