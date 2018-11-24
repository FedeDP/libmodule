#include "poll_priv.h"

typedef struct _elem {
    void *userptr;
    bool autofree;
    struct _elem *prev;
} stack_elem;

struct _stack {
    int len;
    stack_elem *data;
};

stack_t *stack_new(void) {
    stack_t *s = memhook._malloc(sizeof(stack_t));
    if (s) {
        s->len = 0;
        s->data = NULL;
    }
    return s;
}

stack_ret_code stack_iterate(const stack_t *s, const stack_cb fn, void *userptr) {
    MOD_ASSERT(s, "NULL stack.", STACK_WRONG_PARAM);
    MOD_ASSERT(fn, "NULL cb.", STACK_WRONG_PARAM);
    MOD_ASSERT(stack_length(s) > 0, "Empty stack.", STACK_MISSING);
    
    stack_ret_code status = STACK_OK;
    stack_elem *elem = s->data;
    while (elem && status == STACK_OK) {
        status = fn(userptr, elem->userptr);
        elem = elem->prev;
    }
    return status;
}

stack_ret_code stack_push(stack_t *s, void *data, bool autofree) {
    MOD_ASSERT(s, "NULL stack.", STACK_WRONG_PARAM);
    MOD_ASSERT(data, "NULL data.", STACK_WRONG_PARAM);
    
    stack_elem *elem = memhook._malloc(sizeof(stack_elem));
    if (elem) {
        s->len++;
        elem->userptr = data;
        elem->prev = s->data;
        elem->autofree = autofree;
        s->data = elem;
        return STACK_OK;
    }
    return STACK_OMEM;
}

void *stack_pop(stack_t *s) {
    MOD_ASSERT(s, "NULL stack.", NULL);
    MOD_ASSERT(stack_length(s) > 0, "Empty stack.", NULL);

    stack_elem *elem = s->data;
    s->data = s->data->prev;
    void *data = elem->userptr;
    memhook._free(elem);
    s->len--;
    return data;
}

void *stack_peek(const stack_t *s) {
    MOD_ASSERT(s, "NULL stack.", NULL);
    MOD_ASSERT(stack_length(s) > 0, "Empty stack.", NULL);
    
    return s->data->userptr; // return most recent element data
}

stack_ret_code stack_free(stack_t *s) {
    MOD_ASSERT(s, "NULL stack.", STACK_WRONG_PARAM);
    
    stack_elem *elem = NULL;
    while ((elem = s->data) && s->len > 0) {
        const bool autofree = elem->autofree;
        void *data = stack_pop(s);
        if (autofree) {
            memhook._free(data);
        }
    }
    free(s);
    return STACK_OK;
}

int stack_length(const stack_t *s) {
    MOD_ASSERT(s, "NULL stack.", STACK_WRONG_PARAM);
    
    return s->len;
}
