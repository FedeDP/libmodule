/*
 * Based upon work from Mathias Brossard <mathias@brossard.org>.
 * See: https://github.com/mbrossard/threadpool
 * 
 * Thank you!
 */

#include "priv.h"

#define M_THREADS_ASSERT(pool, ret) \
    M_RET_ASSERT(!pool->shutdown, -EPERM); \
    M_RET_ASSERT(pool->init_state & INITED_STARTED, -EPERM);  

typedef enum {
    INITED_THREADS  = 0x01,     // threads are allocated
    INITED_TASKS    = 0x02,     // tasks queue is allocated
    INITED_MUT      = 0x04,     // mutex has been inited
    INITED_COND     = 0x08,     // condition variable has been inited
    INITED_STARTED  = 0x10,     // threads have been actually started
} thpool_inited_t;

typedef struct {
    m_thpool_task fn;
    void *arg;
} thpool_task_t;

struct _thpool {
    uint8_t thread_count;
    thpool_inited_t init_state;     /* Nobody writes this but us during thpool_new. No need to use an atomic */
    bool shutdown;
    bool joinable;
    pthread_mutex_t lock;
    pthread_cond_t notify;
    pthread_t *threads;
    m_queue_t *tasks;
};

static void *thpool_thread(void *thpool);

static void *thpool_thread(void *thpool) {
    m_thpool_t *pool = (m_thpool_t *)thpool;
    
    while (true) {
        /* Lock must be taken to wait on conditional variable */
        pthread_mutex_lock(&(pool->lock));
        
        /* 
         * Wait on condition variable, check for spurious wakeups.
         * When returning from pthread_cond_wait(), we own the lock. 
         */
        while (m_queue_length(pool->tasks) == 0 && !pool->shutdown) {
            pthread_cond_wait(&(pool->notify), &(pool->lock));
        }
        
        if (pool->shutdown && m_queue_length(pool->tasks) == 0) {
            break;
        }
            
        thpool_task_t *task = m_queue_dequeue(pool->tasks);
        pthread_mutex_unlock(&(pool->lock));
        task->fn(task->arg);
    }
    
    pthread_mutex_unlock(&(pool->lock));
    pthread_exit(NULL);
}

_public_ m_thpool_t *m_thpool_new(const uint8_t thread_count, const pthread_attr_t *attrs) {
    M_RET_ASSERT(thread_count > 0, NULL);
    
    m_thpool_t *pool = memhook._calloc(1, sizeof(m_thpool_t));
    M_RET_ASSERT(pool, NULL);
    
    do {
        pool->threads = memhook._calloc(thread_count, sizeof(pthread_t));
        if (!pool->threads) {
            break;
        }
        pool->init_state |= INITED_THREADS;
        
        pool->tasks = m_queue_new(NULL);
        if (!pool->tasks) {
            break;
        }
        pool->init_state |= INITED_TASKS;
        
        /* Initialize mutex and conditional variable first */
        if (pthread_mutex_init(&(pool->lock), NULL) != 0) {
            break;
        }
        pool->init_state |= INITED_MUT;
        
        if (pthread_cond_init(&(pool->notify), NULL) != 0) {
            break;
        }
        pool->init_state |= INITED_COND;
        
        /* Start worker threads */
        for (int i = 0; i < thread_count; i++) {
            if (pthread_create(&(pool->threads[i]), attrs,
                thpool_thread, (void*)pool) == 0) {
                
                pool->thread_count++;
            } else {
                break;
            }
        }
        
        if (pool->thread_count == thread_count) {
            pool->init_state |= INITED_STARTED;
            if (attrs) {
                int detached_state;
                pthread_attr_getdetachstate(attrs, &detached_state);
                pool->joinable = detached_state == PTHREAD_CREATE_JOINABLE;
            } else {
                pool->joinable = true;
            }
        }
    } while (false);
    
    /* Something went wrong; destroy */
    if (!(pool->init_state & INITED_STARTED)) {
        m_thpool_free(&pool);
    }
    return pool;
}

_public_ int m_thpool_add(m_thpool_t *pool, m_thpool_task task, void *arg) {
    M_PARAM_ASSERT(pool);
    M_PARAM_ASSERT(task);
    M_THREADS_ASSERT(pool, -EPERM);
    
    int ret = pthread_mutex_lock(&pool->lock);
    if (ret) {
        return ret;
    }
        
    /* Add task to queue */
    thpool_task_t *new_task = memhook._calloc(1, sizeof(thpool_task_t));
    new_task->fn = task;
    new_task->arg = arg;
    m_queue_enqueue(pool->tasks, new_task);
    ret = pthread_cond_signal(&pool->notify);

    const int unlock_ret = pthread_mutex_unlock(&pool->lock);
    if (unlock_ret == 0) {
        return ret;
    }
    return unlock_ret;
}

_public_ bool m_thpool_joinable(m_thpool_t *pool) {
    M_RET_ASSERT(pool, false);
    M_THREADS_ASSERT(pool, false);
    
    return pool->joinable;
}

_public_ ssize_t m_thpool_length(m_thpool_t *pool) {
    M_PARAM_ASSERT(pool);
    M_THREADS_ASSERT(pool, -EPERM);
    
    int ret = pthread_mutex_lock(&pool->lock);
    if (ret) {
        return ret;
    }
    
    ssize_t len = m_queue_length(pool->tasks);
    
    const int unlock_ret = pthread_mutex_unlock(&pool->lock);
    if (unlock_ret == 0) {
        return len;
    }
    return unlock_ret;
}

_public_ ssize_t m_thpool_clear(m_thpool_t *pool) {
    M_PARAM_ASSERT(pool);
    M_THREADS_ASSERT(pool, -EPERM);
    
    int ret = pthread_mutex_lock(&pool->lock);
    if (ret) {
        return ret;
    }
    
    ret = m_queue_clear(pool->tasks);
    
    const int unlock_ret = pthread_mutex_unlock(&pool->lock);
    if (unlock_ret == 0) {
        return ret;
    }
    return unlock_ret;
}

_public_ int m_thpool_wait(m_thpool_t *pool) {
    M_PARAM_ASSERT(pool);
    M_THREADS_ASSERT(pool, -EPERM);
    M_PARAM_ASSERT(pool->joinable);
    
    int ret = pthread_mutex_lock(&pool->lock);
    if (ret) {
        return ret;
    }
    
    pool->shutdown = true;
    
    /* Wake up all worker threads and unlock mutex */
    ret = pthread_cond_broadcast(&pool->notify) + pthread_mutex_unlock(&pool->lock);
    if (!ret) {
        /* Join all worker threads */
        for (int i = 0; i < pool->thread_count; i++) {
            ret += pthread_join(pool->threads[i], NULL);
        }
        
        if (!ret) {
            /* There are no threads anymore */
            pool->init_state &= ~INITED_STARTED;
        }
    }
    return ret;
}

_public_ int m_thpool_free(m_thpool_t **pool) {
    M_PARAM_ASSERT(pool && *pool);

    m_thpool_t *p = *pool;
        
    for (int i = p->init_state; i > 0; i /= 2) {
        switch (i) {
        case INITED_COND:
            pthread_cond_destroy(&p->notify);
            break;
        case INITED_MUT:
            pthread_mutex_destroy(&p->lock);
            break;
        case INITED_TASKS:
            m_queue_free(&p->tasks);
            break;
        case INITED_THREADS:
            memhook._free(p->threads);
            break;
        default:
            break;
        }
    }
    memhook._free(p);
    *pool = NULL;
    return 0;
}
