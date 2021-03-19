#pragma once

#ifndef LIBMODULE_TH_H
    #define LIBMODULE_TH_H
#endif
#include "cmn.h"
#include <pthread.h>

typedef void *(*m_thpool_task)(void *);

typedef struct _thpool m_thpool_t;

m_thpool_t *m_thpool_new(uint8_t thread_count, const pthread_attr_t *attrs);
int m_thpool_add(m_thpool_t *pool, m_thpool_task task, void *arg);
bool m_thpool_joinable(m_thpool_t *pool);
ssize_t m_thpool_length(m_thpool_t *pool);
ssize_t m_thpool_clear(m_thpool_t *pool);
int m_thpool_free(m_thpool_t **pool, bool wait_all);
