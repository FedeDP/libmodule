#include "poll_priv.h"
#include "queue.h"

#define QUEUE_PARAM_ASSERT(cond)    MOD_RET_ASSERT(cond, QUEUE_WRONG_PARAM);

typedef struct _elem {
    void *userptr;
    struct _elem *prev;
} queue_elem;

struct _queue {
    size_t len;
    mod_queue_dtor dtor;
    queue_elem *head;
    queue_elem *tail;
};

struct _queue_itr {
    queue_elem *elem;
};

/** Public API **/

mod_queue_t *queue_new(const mod_queue_dtor fn) {
    mod_queue_t *q = memhook._calloc(1, sizeof(mod_queue_t));
    if (q) {
        q->dtor = fn;
    }
    return q;
}

mod_queue_itr_t *queue_itr_new(const mod_queue_t *q) {
    MOD_RET_ASSERT(queue_length(q) > 0, NULL);
    
    mod_queue_itr_t *itr = memhook._malloc(sizeof(mod_queue_itr_t));
    if (itr) {
        itr->elem = q->head;
    }
    return itr;
}

mod_queue_itr_t *queue_itr_next(mod_queue_itr_t *itr) {
    MOD_RET_ASSERT(itr, NULL);
    
    itr->elem = itr->elem->prev;
    if (!itr->elem) {
        memhook._free(itr);
        itr = NULL;
    }
    return itr;
}

void *queue_itr_get_data(const mod_queue_itr_t *itr) {
    MOD_RET_ASSERT(itr, NULL);
    
    return itr->elem->userptr;
}

mod_queue_ret queue_itr_set_data(const mod_queue_itr_t *itr, void *value) {
    QUEUE_PARAM_ASSERT(itr);
    QUEUE_PARAM_ASSERT(value);
    
    itr->elem->userptr = value;
    return QUEUE_OK;
}

mod_queue_ret queue_iterate(const mod_queue_t *q, const mod_queue_cb fn, void *userptr) {
    QUEUE_PARAM_ASSERT(fn);
    MOD_RET_ASSERT(queue_length(q) > 0, QUEUE_MISSING);
    
    queue_elem *elem = q->head;
    while (elem) {
        mod_queue_ret rc = fn(userptr, elem->userptr);
        if (rc < QUEUE_OK) {
            /* Stop right now with error */
            return rc;
        }
        if (rc > QUEUE_OK) {
            /* Stop right now with QUEUE_OK */
            return QUEUE_OK;
        }
        elem = elem->prev;
    }
    return QUEUE_OK;
}

mod_queue_ret queue_enqueue(mod_queue_t *q, void *data) {
    QUEUE_PARAM_ASSERT(q);
    QUEUE_PARAM_ASSERT(data);
    
    queue_elem *elem = memhook._calloc(1, sizeof(queue_elem));
    if (elem) {
        q->len++;
        elem->userptr = data;
        if (q->tail) {
            q->tail->prev = elem;
        }
        q->tail = elem;
        if (!q->head) {
            q->head = q->tail;
        }
        return QUEUE_OK;
    }
    return QUEUE_OMEM;
}

void *queue_dequeue(mod_queue_t *q) {
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

void *queue_peek(const mod_queue_t *q) {
    MOD_RET_ASSERT(queue_length(q) > 0, NULL);
    
    return q->head->userptr;
}

mod_queue_ret queue_remove(mod_queue_t *q) {
    void *data = queue_dequeue(q);
    if (data && q->dtor) {
        q->dtor(data);
        return QUEUE_OK;
    }
    return QUEUE_WRONG_PARAM;
}

mod_queue_ret queue_clear(mod_queue_t *q) {
    QUEUE_PARAM_ASSERT(q);
    
    queue_elem *elem = NULL;
    while ((elem = q->head) && q->len > 0) {
        queue_remove(q);
    }
    return QUEUE_OK;
}

mod_queue_ret queue_free(mod_queue_t **q) {
    QUEUE_PARAM_ASSERT(q);
    
    mod_queue_ret ret = queue_clear(*q);
    if (ret == QUEUE_OK) {
        memhook._free(*q);
        *q = NULL;
    }
    return ret;
}

ssize_t queue_length(const mod_queue_t *q) {
    QUEUE_PARAM_ASSERT(q);
    
    return q->len;
}

