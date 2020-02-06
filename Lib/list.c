#include "poll_priv.h"

#define LIST_PARAM_ASSERT(cond)    MOD_RET_ASSERT(cond, LIST_WRONG_PARAM);

typedef struct _elem {
    void *userptr;
    struct _elem *next;
} list_node;

struct _list {
    size_t len;
    list_dtor dtor;
    list_node *data;
};

struct _list_itr {
    list_node **elem;
    list_t *l;
};

static list_ret_code insert_node(list_t *l, list_node **elem, void *data);
static list_ret_code remove_node(list_t *l, list_node **elem);

static list_ret_code insert_node(list_t *l, list_node **elem, void *data) {
    list_node *node = memhook._malloc(sizeof(list_node));
    if (node) {
        node->next = *elem;
        node->userptr = data;
        *elem = node;
        l->len++;
        return LIST_OK;
    }
    return LIST_OMEM;
}

static list_ret_code remove_node(list_t *l, list_node **elem) {
    list_node *tmp = *elem;
    if (tmp) {
        *elem = (*elem)->next;
        if (l->dtor) {
            l->dtor(tmp->userptr);
        }
        memhook._free(tmp);
        l->len--;
        return LIST_OK;
    }
    return LIST_MISSING;
}

/** Public API **/

list_t *list_new(const list_dtor fn) {
    list_t *l = memhook._calloc(1, sizeof(list_t));
    if (l) {
        l->dtor = fn;
    }
    return l;
}

list_itr_t *list_itr_new(const list_t *l) {
    MOD_RET_ASSERT(list_length(l) > 0, NULL);
    
    list_itr_t *itr = memhook._malloc(sizeof(list_itr_t));
    if (itr) {
        itr->elem = (list_node **)&(l->data);
        itr->l = (list_t *)l;
    }
    return itr;
}

list_itr_t *list_itr_next(list_itr_t *itr) {
    MOD_RET_ASSERT(itr, NULL);
    
    if (*itr->elem) {
        itr->elem = &((*itr->elem)->next);
    }
    if (!*(itr->elem)) {
        memhook._free(itr);
        itr = NULL;
    }
    return itr;
}

void *list_itr_get_data(const list_itr_t *itr) {
    MOD_RET_ASSERT(itr, NULL);
    MOD_RET_ASSERT(*itr->elem, NULL);
    
    return (*itr->elem)->userptr;
}

list_ret_code list_itr_set_data(list_itr_t *itr, void *value) {
    LIST_PARAM_ASSERT(itr);
    LIST_PARAM_ASSERT(value);
    MOD_RET_ASSERT(*itr->elem, LIST_ERR);
    
    (*itr->elem)->userptr = value;
    return LIST_OK;
}

list_ret_code list_itr_insert(list_itr_t *itr, void *value) {
    LIST_PARAM_ASSERT(itr);
    LIST_PARAM_ASSERT(value);
    
    return insert_node(itr->l, itr->elem, value);
}

list_ret_code list_itr_remove(list_itr_t *itr) {
    LIST_PARAM_ASSERT(itr);
    MOD_RET_ASSERT(*itr->elem, LIST_ERR);
    
    return remove_node(itr->l, itr->elem);
}

list_ret_code list_iterate(const list_t *l, const list_cb fn, void *userptr) {
    LIST_PARAM_ASSERT(fn);
    MOD_RET_ASSERT(list_length(l) > 0, LIST_MISSING);
    
    list_node *elem = l->data;
    while (elem) {
        list_ret_code rc = fn(userptr, elem->userptr);
        if (rc < LIST_OK) {
            /* Stop right now with error */
            return rc;
        }
        if (rc > LIST_OK) {
            /* Stop right now with LIST_OK */
            return LIST_OK;
        }
        elem = elem->next;
    }
    return LIST_OK;
}

list_ret_code list_insert(list_t *l, void *data, const list_comp comp) {
    LIST_PARAM_ASSERT(l);
    LIST_PARAM_ASSERT(data);
    
    list_node **tmp = &l->data;
    for (int i = 0; i < l->len; i++) {
        if (comp && comp(data, (*tmp)->userptr) == 0) {
            break;
        }
        tmp = &(*tmp)->next;
    }
    
    return insert_node(l, tmp, data);
}

list_ret_code list_remove(list_t *l, void *data, const list_comp comp) {
    LIST_PARAM_ASSERT(l);
    LIST_PARAM_ASSERT(data);
    LIST_PARAM_ASSERT(comp);
    
    list_node **tmp = &l->data;
    for (int i = 0; i < l->len; i++) {
        if (comp(data, (*tmp)->userptr) == 0) {
            break;
        }
        tmp = &(*tmp)->next;
    }
    return remove_node(l, tmp);
}

list_ret_code list_clear(list_t *l) {
    LIST_PARAM_ASSERT(l);
    
    for (list_itr_t *itr = list_itr_new(l); itr; itr = list_itr_next(itr)) {        
        list_itr_remove(itr);
    }
    return LIST_OK;
}

list_ret_code list_free(list_t *l) {
    LIST_PARAM_ASSERT(l);
    
    list_ret_code ret = list_clear(l);
    if (ret == LIST_OK) {
        memhook._free(l);
    }
    return ret;
}

ssize_t list_length(const list_t *l) {
    LIST_PARAM_ASSERT(l);
    
    return l->len;
}
