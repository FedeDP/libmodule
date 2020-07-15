#pragma once

#include "commons.h"
#include <pthread.h>

typedef void *(*m_thpool_task)(void *);

typedef struct _thpool m_thpool_t;

_public_ m_thpool_t *m_thpool_new(const uint8_t thread_count, const pthread_attr_t *attrs);
_public_ int m_thpool_add(m_thpool_t *pool, m_thpool_task task, void *arg);
_public_ bool m_thpool_joinable(m_thpool_t *pool);
_public_ ssize_t m_thpool_length(m_thpool_t *pool);
_public_ ssize_t m_thpool_clear(m_thpool_t *pool);
_public_ int m_thpool_wait(m_thpool_t *pool);
_public_ int m_thpool_free(m_thpool_t **pool);
