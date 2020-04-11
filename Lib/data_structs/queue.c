#include "poll_priv.h"
#include "queue.h"

typedef struct _elem {
    void *userptr;
    struct _elem *prev;
} queue_elem;

struct _queue {
    size_t len;
    m_queue_dtor dtor;
    queue_elem *head;
    queue_elem *tail;
};

struct _queue_itr {
    queue_elem *elem;
};

/** Public API **/

m_queue_t *queue_new(const m_queue_dtor fn) {
    m_queue_t *q = memhook._calloc(1, sizeof(m_queue_t));
    if (q) {
        q->dtor = fn;
    }
    return q;
}

m_queue_itr_t *queue_itr_new(const m_queue_t *q) {
    MOD_RET_ASSERT(queue_length(q) > 0, NULL);
    
    m_queue_itr_t *itr = memhook._malloc(sizeof(m_queue_itr_t));
    if (itr) {
        itr->elem = q->head;
    }
    return itr;
}

m_queue_itr_t *queue_itr_next(m_queue_itr_t *itr) {
    MOD_RET_ASSERT(itr, NULL);
    
    itr->elem = itr->elem->prev;
    if (!itr->elem) {
        memhook._free(itr);
        itr = NULL;
    }
    return itr;
}

void *queue_itr_get_data(const m_queue_itr_t *itr) {
    MOD_RET_ASSERT(itr, NULL);
    
    return itr->elem->userptr;
}

int queue_itr_set_data(const m_queue_itr_t *itr, void *value) {
    MOD_PARAM_ASSERT(itr);
    MOD_PARAM_ASSERT(value);
    
    itr->elem->userptr = value;
    return 0;
}

int queue_iterate(const m_queue_t *q, const m_queue_cb fn, void *userptr) {
    MOD_PARAM_ASSERT(fn);
    MOD_PARAM_ASSERT(queue_length(q) > 0);
    
    queue_elem *elem = q->head;
    while (elem) {
        int rc = fn(userptr, elem->userptr);
        if (rc < 0) {
            /* Stop right now with error */
            return rc;
        }
        if (rc > 0) {
            /* Stop right now with 0 */
            return 0;
        }
        elem = elem->prev;
    }
    return 0;
}

int queue_enqueue(m_queue_t *q, void *data) {
    MOD_PARAM_ASSERT(q);
    MOD_PARAM_ASSERT(data);
    
    queue_elem *elem = memhook._calloc(1, sizeof(queue_elem));
    MOD_ALLOC_ASSERT(elem);
    q->len++;
    elem->userptr = data;
    if (q->tail) {
        q->tail->prev = elem;
    }
    q->tail = elem;
    if (!q->head) {
        q->head = q->tail;
    }
    return 0;
}

void *queue_dequeue(m_queue_t *q) {
    MOD_RET_ASSERT(queue_length(q) > 0, NULL);
    
    queue_elem *elem = q->head;
    if (q->tail == q->head) {
        q->tail = NULL;
    }
    q->head = q->head->prev;
    
    void *data = elem->userptr;
    memhook._free(elem);
    q->len--;
    return data;
}

void *queue_peek(const m_queue_t *q) {
    MOD_RET_ASSERT(queue_length(q) > 0, NULL);
    
    return q->head->userptr;
}

int queue_remove(m_queue_t *q) {
    void *data = queue_dequeue(q);
    if (data) {
        if (q->dtor) {
            q->dtor(data);
        }
        return 0;
    }
    return -EINVAL;
}

int queue_clear(m_queue_t *q) {
    MOD_PARAM_ASSERT(queue_length(q) > 0);
    
    queue_elem *elem = NULL;
    while ((elem = q->head) && q->len > 0) {
        queue_remove(q);
    }
    return 0;
}

int queue_free(m_queue_t **q) {
    MOD_PARAM_ASSERT(q);
    
    queue_clear(*q);
    memhook._free(*q);
    *q = NULL;
    return 0;
}

ssize_t queue_length(const m_queue_t *q) {
    MOD_PARAM_ASSERT(q);
    
    return q->len;
}

