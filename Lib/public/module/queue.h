#pragma once

#include "module_cmn.h"

/** Queue interface **/

/* Callback for queue_iterate */
typedef int (*m_queue_cb)(void *, void *);

/* Fn for queue_set_dtor */
typedef void (*m_queue_dtor)(void *);

/* Incomplete struct declaration for queue */
typedef struct _queue m_queue_t;

/* Incomplete struct declaration for queue iterator */
typedef struct _queue_itr m_queue_itr_t;

_public_ m_queue_t *queue_new(const m_queue_dtor fn);
_public_ m_queue_itr_t *queue_itr_new(const m_queue_t *q);
_public_ m_queue_itr_t *queue_itr_next(m_queue_itr_t *itr);
_public_ void *queue_itr_get_data(const m_queue_itr_t *itr);
_public_ int queue_itr_set_data(const m_queue_itr_t *itr, void *value);
_public_ int queue_iterate(const m_queue_t *q, const m_queue_cb fn, void *userptr);
_public_ int queue_enqueue(m_queue_t *q, void *data);
_public_ void *queue_dequeue(m_queue_t *q);
_public_ void *queue_peek(const m_queue_t *q);
_public_ int queue_remove(m_queue_t *q);
_public_ int queue_clear(m_queue_t *q);
_public_ int queue_free(m_queue_t **q);
_public_ ssize_t queue_length(const m_queue_t *q);
