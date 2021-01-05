#pragma once

#ifndef LIBMODULE_STRUCT_H
    #define LIBMODULE_STRUCT_H
#endif
#include "cmn.h"

/** Queue interface **/

/* Callback for queue_iterate */
typedef int (*m_queue_cb)(void *, void *);

/* Fn for queue_set_dtor */
typedef void (*m_queue_dtor)(void *);

/* Incomplete struct declaration for queue */
typedef struct _queue m_queue_t;

/* Incomplete struct declaration for queue iterator */
typedef struct _queue_itr m_queue_itr_t;

_public_ m_queue_t *m_queue_new(const m_queue_dtor fn);
_public_ m_queue_itr_t *m_queue_itr_new(const m_queue_t *q);
_public_ int m_queue_itr_next(m_queue_itr_t **itr);
_public_ int m_queue_itr_remove(m_queue_itr_t *itr);
_public_ void *m_queue_itr_get_data(const m_queue_itr_t *itr);
_public_ int m_queue_itr_set_data(const m_queue_itr_t *itr, void *value);
_public_ size_t m_queue_itr_idx(const m_queue_itr_t *itr);
_public_ int m_queue_iterate(const m_queue_t *q, const m_queue_cb fn, void *userptr);
_public_ int m_queue_enqueue(m_queue_t *q, void *data);
_public_ void *m_queue_dequeue(m_queue_t *q);
_public_ void *m_queue_peek(const m_queue_t *q);
_public_ int m_queue_remove(m_queue_t *q);
_public_ int m_queue_clear(m_queue_t *q);
_public_ int m_queue_free(m_queue_t **q);
_public_ ssize_t m_queue_length(const m_queue_t *q);
