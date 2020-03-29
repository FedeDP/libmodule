#include "poll_priv.h"

#define LIST_PARAM_ASSERT(cond)    MOD_RET_ASSERT(cond, LIST_WRONG_PARAM);

typedef struct _elem {
    void *userptr;
    struct _elem *next;
} list_node;

struct _list {
    size_t len;
    mod_list_dtor dtor;
    list_node *data;
};

struct _list_itr {
    list_node **elem;
    mod_list_t *l;
    ssize_t diff;
};

static mod_list_ret insert_node(mod_list_t *l, list_node **elem, void *data);
static mod_list_ret remove_node(mod_list_t *l, list_node **elem);

static mod_list_ret insert_node(mod_list_t *l, list_node **elem, void *data) {
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

static mod_list_ret remove_node(mod_list_t *l, list_node **elem) {
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

mod_list_t *list_new(const mod_list_dtor fn) {
    mod_list_t *l = memhook._calloc(1, sizeof(mod_list_t));
    if (l) {
        l->dtor = fn;
    }
    return l;
}

mod_list_itr_t *list_itr_new(const mod_list_t *l) {
    MOD_RET_ASSERT(list_length(l) > 0, NULL);
    
    mod_list_itr_t *itr = memhook._calloc(1, sizeof(mod_list_itr_t));
    if (itr) {
        itr->elem = (list_node **)&(l->data);
        itr->l = (mod_list_t *)l;
    }
    return itr;
}

mod_list_itr_t *list_itr_next(mod_list_itr_t *itr) {
    MOD_RET_ASSERT(itr, NULL);
    
    if (*itr->elem) {
        if (itr->diff >= 0) {
            itr->elem = &((*itr->elem)->next);
        } 
        itr->diff = 0;
    }
    if (!*(itr->elem)) {
        memhook._free(itr);
        itr = NULL;
    }
    return itr;
}

void *list_itr_get_data(const mod_list_itr_t *itr) {
    MOD_RET_ASSERT(itr, NULL);
    MOD_RET_ASSERT(*itr->elem, NULL);
    
    return (*itr->elem)->userptr;
}

mod_list_ret list_itr_set_data(mod_list_itr_t *itr, void *value) {
    LIST_PARAM_ASSERT(itr);
    LIST_PARAM_ASSERT(value);
    MOD_RET_ASSERT(*itr->elem, LIST_ERR);
    
    (*itr->elem)->userptr = value;
    return LIST_OK;
}

mod_list_ret list_itr_insert(mod_list_itr_t *itr, void *value) {
    LIST_PARAM_ASSERT(itr);
    LIST_PARAM_ASSERT(value);
    
    itr->diff++;
    return insert_node(itr->l, itr->elem, value);
}

mod_list_ret list_itr_remove(mod_list_itr_t *itr) {
    LIST_PARAM_ASSERT(itr);
    MOD_RET_ASSERT(*itr->elem, LIST_ERR);
    
    itr->diff--; // notify list to avoid skipping 1 element on next list_itr_next() call
    return remove_node(itr->l, itr->elem);
}

mod_list_ret list_iterate(const mod_list_t *l, const mod_list_cb fn, void *userptr) {
    LIST_PARAM_ASSERT(fn);
    MOD_RET_ASSERT(list_length(l) > 0, LIST_MISSING);
    
    list_node *elem = l->data;
    while (elem) {
        mod_list_ret rc = fn(userptr, elem->userptr);
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

mod_list_ret list_insert(mod_list_t *l, void *data, const mod_list_comp comp) {
    LIST_PARAM_ASSERT(l);
    LIST_PARAM_ASSERT(data);
    
    list_node **tmp = &l->data;
    for (int i = 0; i < l->len && comp; i++) {
        if (comp(data, (*tmp)->userptr) == 0) {
            break;
        }
        tmp = &(*tmp)->next;
    }
    
    return insert_node(l, tmp, data);
}

mod_list_ret list_remove(mod_list_t *l, void *data, const mod_list_comp comp) {
    LIST_PARAM_ASSERT(l);
    LIST_PARAM_ASSERT(data);
    
    list_node **tmp = &l->data;
    for (int i = 0; i < l->len; i++) {
        if ((comp && comp(data, (*tmp)->userptr) == 0) || (*tmp)->userptr == data) {
            
            break;
        }
        tmp = &(*tmp)->next;
    }
    return remove_node(l, tmp);
}

void *list_find(mod_list_t *l, void *data, const mod_list_comp comp) {
    MOD_RET_ASSERT(l, NULL);
    MOD_RET_ASSERT(data, NULL);
    
    list_node **tmp = &l->data;
    for (int i = 0; i < l->len; i++) {
        if ((comp && comp(data, (*tmp)->userptr) == 0) || (*tmp)->userptr == data) {
            return (*tmp)->userptr;
        }
        tmp = &(*tmp)->next;
    }
    return NULL;
}

mod_list_ret list_clear(mod_list_t *l) {
    LIST_PARAM_ASSERT(l);
    
    for (mod_list_itr_t *itr = list_itr_new(l); itr; itr = list_itr_next(itr)) {        
        list_itr_remove(itr);
    }
    return LIST_OK;
}

mod_list_ret list_free(mod_list_t **l) {
    LIST_PARAM_ASSERT(l);
    
    mod_list_ret ret = list_clear(*l);
    if (ret == LIST_OK) {
        memhook._free(*l);
        *l = NULL;
    }
    return ret;
}

ssize_t list_length(const mod_list_t *l) {
    LIST_PARAM_ASSERT(l);
    
    return l->len;
}
