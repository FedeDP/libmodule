#pragma once

#include "module_cmn.h"

/** Queue interface **/

/* Callback for queue_iterate */
typedef int (*mod_queue_cb)(void *, void *);

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
_public_ int queue_itr_set_data(const mod_queue_itr_t *itr, void *value);
_public_ int queue_iterate(const mod_queue_t *q, const mod_queue_cb fn, void *userptr);
_public_ int queue_enqueue(mod_queue_t *q, void *data);
_public_ void *queue_dequeue(mod_queue_t *q);
_public_ void *queue_peek(const mod_queue_t *q);
_public_ int queue_remove(mod_queue_t *q);
_public_ int queue_clear(mod_queue_t *q);
_public_ int queue_free(mod_queue_t **q);
_public_ ssize_t queue_length(const mod_queue_t *q);
