#include "poll_priv.h"
#include "queue.h"

#define QUEUE_PARAM_ASSERT(cond)    MOD_RET_ASSERT(cond, QUEUE_WRONG_PARAM);

typedef struct _elem {
    void *userptr;
    struct _elem *prev;
} queue_elem;

struct _queue {
    size_t len;
    queue_dtor dtor;
    queue_elem *head;
    queue_elem *tail;
};

struct _queue_itr {
    queue_elem *elem;
};

/** Public API **/

queue_t *queue_new(const queue_dtor fn) {
    queue_t *q = memhook._calloc(1, sizeof(queue_t));
    if (q) {
        q->dtor = fn;
    }
    return q;
}

queue_itr_t *queue_itr_new(const queue_t *q) {
    MOD_RET_ASSERT(queue_length(q) > 0, NULL);
    
    queue_itr_t *itr = memhook._malloc(sizeof(queue_itr_t));
    if (itr) {
        itr->elem = q->head;
    }
    return itr;
}

queue_itr_t *queue_itr_next(queue_itr_t *itr) {
    MOD_RET_ASSERT(itr, NULL);
    
    itr->elem = itr->elem->prev;
    if (!itr->elem) {
        memhook._free(itr);
        itr = NULL;
    }
    return itr;
}

void *queue_itr_get_data(const queue_itr_t *itr) {
    MOD_RET_ASSERT(itr, NULL);
    
    return itr->elem->userptr;
}

queue_ret_code queue_itr_set_data(const queue_itr_t *itr, void *value) {
    QUEUE_PARAM_ASSERT(itr);
    QUEUE_PARAM_ASSERT(value);
    
    itr->elem->userptr = value;
    return QUEUE_OK;
}

queue_ret_code queue_iterate(const queue_t *q, const queue_cb fn, void *userptr) {
    QUEUE_PARAM_ASSERT(fn);
    MOD_RET_ASSERT(queue_length(q) > 0, QUEUE_MISSING);
    
    queue_elem *elem = q->head;
    while (elem) {
        queue_ret_code rc = fn(userptr, elem->userptr);
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

queue_ret_code queue_enqueue(queue_t *q, void *data) {
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

void *queue_dequeue(queue_t *q) {
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

void *queue_peek(const queue_t *q) {
    MOD_RET_ASSERT(queue_length(q) > 0, NULL);
    
    return q->head->userptr;
}

queue_ret_code queue_clear(queue_t *q) {
    QUEUE_PARAM_ASSERT(q);
    
    queue_elem *elem = NULL;
    while ((elem = q->head) && q->len > 0) {
        void *data = queue_dequeue(q);
        if (q->dtor) {
            q->dtor(data);
        }
    }
    return QUEUE_OK;
}

queue_ret_code queue_free(queue_t *q) {
    queue_ret_code ret = queue_clear(q);
    if (ret == QUEUE_OK) {
        memhook._free(q);
    }
    return ret;
}

ssize_t queue_length(const queue_t *q) {
    QUEUE_PARAM_ASSERT(q);
    
    return q->len;
}

