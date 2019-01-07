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

/* Incomplete struct declaration for stack */
typedef struct _stack stack_t;

#ifdef __cplusplus
extern "C"{
#endif

_public_ stack_t *stack_new(void);
_public_ stack_ret_code stack_iterate(const stack_t *s, const stack_cb fn, void *userptr);
_public_ stack_ret_code stack_push(stack_t *s, void *data, bool autofree);
_public_ void *stack_pop(stack_t *s);
_public_ void *stack_peek(const stack_t *s);
_public_ stack_ret_code stack_clear(stack_t *s);
_public_ stack_ret_code stack_free(stack_t *s);
_public_ int stack_length(const stack_t *s);

#ifdef __cplusplus
}
#endif