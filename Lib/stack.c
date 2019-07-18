#include "poll_priv.h"

#define STACK_PARAM_ASSERT(cond)    MOD_RET_ASSERT(cond, STACK_WRONG_PARAM);

typedef struct _elem {
    void *userptr;
    struct _elem *prev;
} stack_elem;

struct _stack {
    size_t len;
    stack_dtor dtor;
    stack_elem *data;
};

struct _stack_itr {
    stack_elem *elem;
};

/** Public API **/

stack_t *stack_new(const stack_dtor fn) {
    stack_t *s = memhook._calloc(1, sizeof(stack_t));
    if (s) {
        s->dtor = fn;
    }
    return s;
}

stack_itr_t *stack_itr_new(const stack_t *s) {
    MOD_RET_ASSERT(stack_length(s) > 0, NULL);
    
    stack_itr_t *itr = memhook._malloc(sizeof(stack_itr_t));
    if (itr) {
        itr->elem = s->data;
    }
    return itr;
}

stack_itr_t *stack_itr_next(stack_itr_t *itr) {
    MOD_RET_ASSERT(itr, NULL);
    
    itr->elem = itr->elem->prev;
    if (!itr->elem) {
        memhook._free(itr);
        itr = NULL;
    }
    return itr;
}

void *stack_itr_get_data(const stack_itr_t *itr) {
    MOD_RET_ASSERT(itr, NULL);
    
    return itr->elem->userptr;
}

stack_ret_code stack_itr_set_data(const stack_itr_t *itr, void *value) {
    STACK_PARAM_ASSERT(itr);
    STACK_PARAM_ASSERT(value);
    
    itr->elem->userptr = value;
    return STACK_OK;
}

stack_ret_code stack_iterate(const stack_t *s, const stack_cb fn, void *userptr) {
    STACK_PARAM_ASSERT(fn);
    MOD_RET_ASSERT(stack_length(s) > 0, STACK_MISSING);
    
    stack_elem *elem = s->data;
    while (elem) {
        stack_ret_code rc = fn(userptr, elem->userptr);
        if (rc < STACK_OK) {
            /* Stop right now with error */
            return rc;
        }
        if (rc > STACK_OK) {
            /* Stop right now with MAP_OK */
            return STACK_OK;
        }
        elem = elem->prev;
    }
    return STACK_OK;
}

stack_ret_code stack_push(stack_t *s, void *data) {
    STACK_PARAM_ASSERT(s);
    STACK_PARAM_ASSERT(data);
    
    stack_elem *elem = memhook._malloc(sizeof(stack_elem));
    if (elem) {
        s->len++;
        elem->userptr = data;
        elem->prev = s->data;
        s->data = elem;
        return STACK_OK;
    }
    return STACK_OMEM;
}

void *stack_pop(stack_t *s) {
    MOD_RET_ASSERT(stack_length(s) > 0, NULL);

    stack_elem *elem = s->data;
    s->data = s->data->prev;
    void *data = elem->userptr;
    memhook._free(elem);
    s->len--;
    return data;
}

void *stack_peek(const stack_t *s) {
    MOD_RET_ASSERT(stack_length(s) > 0, NULL);
    
    return s->data->userptr; // return most recent element data
}

stack_ret_code stack_clear(stack_t *s) {
    STACK_PARAM_ASSERT(s);
    
    stack_elem *elem = NULL;
    while ((elem = s->data) && s->len > 0) {
        void *data = stack_pop(s);
        if (s->dtor) {
            s->dtor(data);
        }
    }
    return STACK_OK;
}

stack_ret_code stack_free(stack_t *s) {
    stack_ret_code ret = stack_clear(s);
    if (ret == STACK_OK) {
        free(s);
    }
    return ret;
}

ssize_t stack_length(const stack_t *s) {
    STACK_PARAM_ASSERT(s);
    
    return s->len;
}
