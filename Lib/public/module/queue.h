#pragma once

#include "module_cmn.h"

/** Queue interface **/

typedef enum {
    QUEUE_WRONG_PARAM = -4,
    QUEUE_MISSING,
    QUEUE_ERR,
    QUEUE_OMEM,
    QUEUE_OK
} queue_ret_code;

/* Callback for queue_iterate */
typedef queue_ret_code (*queue_cb)(void *, void *);

/* Fn for queue_set_dtor */
typedef void (*queue_dtor)(void *);

/* Incomplete struct declaration for queue */
typedef struct _queue queue_t;

/* Incomplete struct declaration for queue iterator */
typedef struct _queue_itr queue_itr_t;

_public_ queue_t *queue_new(const queue_dtor fn);
_public_ queue_itr_t *queue_itr_new(const queue_t *q);
_public_ queue_itr_t *queue_itr_next(queue_itr_t *itr);
_public_ void *queue_itr_get_data(const queue_itr_t *itr);
_public_ queue_ret_code queue_itr_set_data(const queue_itr_t *itr, void *value);
_public_ queue_ret_code queue_iterate(const queue_t *q, const queue_cb fn, void *userptr);
_public_ queue_ret_code queue_enqueue(queue_t *q, void *data);
_public_ void *queue_dequeue(queue_t *q);
_public_ void *queue_peek(const queue_t *q);
_public_ queue_ret_code queue_clear(queue_t *q);
_public_ queue_ret_code queue_free(queue_t *q);
_public_ ssize_t queue_length(const queue_t *q);
