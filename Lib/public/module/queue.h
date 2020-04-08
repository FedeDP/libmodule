#pragma once

#include "module_cmn.h"

/** Queue interface **/

typedef enum {
    QUEUE_WRONG_PARAM = -4,
    QUEUE_MISSING,
    QUEUE_ERR,
    QUEUE_OMEM,
    QUEUE_OK
} mod_queue_ret;

/* Callback for queue_iterate */
typedef mod_queue_ret (*mod_queue_cb)(void *, void *);

/* Fn for queue_set_dtor */
typedef void (*mod_queue_dtor)(void *);

/* Incomplete struct declaration for queue */
typedef struct _queue mod_queue_t;

/* Incomplete struct declaration for queue iterator */
typedef struct _queue_itr mod_queue_itr_t;

_public_ mod_queue_t *queue_new(const mod_queue_dtor fn);
_public_ mod_queue_itr_t *queue_itr_new(const mod_queue_t *q);
_public_ mod_queue_itr_t *queue_itr_next(mod_queue_itr_t *itr);
_public_ void *queue_itr_get_data(const mod_queue_itr_t *itr);
_public_ mod_queue_ret queue_itr_set_data(const mod_queue_itr_t *itr, void *value);
_public_ mod_queue_ret queue_iterate(const mod_queue_t *q, const mod_queue_cb fn, void *userptr);
_public_ mod_queue_ret queue_enqueue(mod_queue_t *q, void *data);
_public_ void *queue_dequeue(mod_queue_t *q);
_public_ void *queue_peek(const mod_queue_t *q);
_public_ mod_queue_ret queue_remove(mod_queue_t *q);
_public_ mod_queue_ret queue_clear(mod_queue_t *q);
_public_ mod_queue_ret queue_free(mod_queue_t **q);
_public_ ssize_t queue_length(const mod_queue_t *q);
