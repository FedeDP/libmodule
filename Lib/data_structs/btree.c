#include "poll_priv.h"
#include "btree.h"

typedef struct _elem {
    void *userptr;
    struct _elem *parent;
    struct _elem *right;
    struct _elem *left;
} btree_node;

struct _btree {
    size_t len;
    btree_node *root;
    m_btree_cmp comp;
    m_btree_dtor dtor;
};

struct _btree_itr {
    bool removed;
    btree_node **curr;
    btree_node *prev;
    m_btree_t *l;
};

static inline int insert_node(m_btree_t *l, btree_node **elem, btree_node *parent, void *data);
static inline btree_node **find_min_subtree(btree_node **node);
static inline int remove_node(m_btree_t *l, btree_node **elem);
static int ptrcmp(void *userdata, void *node_data);
static inline int traverse_preorder(btree_node *node, m_btree_cb cb, void *userptr);
static inline int traverse_postorder(btree_node *node, m_btree_cb cb, void *userptr);
static inline int traverse_inorder(btree_node *node, m_btree_cb cb, void *userptr);
static inline btree_node **btree_find(m_btree_t *l, void *data, btree_node **parent);
static inline btree_node **btree_next(btree_node **node);

static inline int insert_node(m_btree_t *l, btree_node **elem, btree_node *parent, void *data) {
    btree_node *node = memhook._calloc(1, sizeof(btree_node));
    MOD_ALLOC_ASSERT(node);
    node->userptr = data;
    node->parent = parent;
    *elem = node;
    l->len++;
    return 0;
}

static inline btree_node **find_min_subtree(btree_node **node) {
    while ((*node)->left) {
        node = &(*node)->left;
    }
    return node;
}

static inline int remove_node(m_btree_t *l, btree_node **elem) {
    btree_node *node = *elem;
    if (node) {
        /* Nodes with at most one child */
        if (!node->left || !node->right) {
            if (node->left) {
                *elem = node->left;
            } else {
                *elem = node->right;
            }
            if (*elem) {
                /* Update node parent */
                (*elem)->parent = node->parent;
            }
            if (l->dtor) {
                l->dtor(node->userptr);
            }
            memhook._free(node);
            l->len--;
            return 0;
        }
        /*
         * Nodes with two children: get inorder successor
         * (smallest in the right subtree)
         */
        btree_node **tmp = find_min_subtree(&node->right);
        node->userptr = (*tmp)->userptr; // switch userdata
        return remove_node(l, tmp); // remove useless left-most node in the right subtree
    }
    return -ENOENT;
}

static int ptrcmp(void *userdata, void *node_data) {
    return (userdata - node_data);
}

static inline int traverse_preorder(btree_node *node, m_btree_cb cb, void *userptr) {
    if (node) {
        int ret = cb(userptr, node->userptr);
        if (ret == 0) {
            ret = traverse_preorder(node->left, cb, userptr);
            if (ret == 0) {
                ret = traverse_preorder(node->right, cb, userptr);
            }
        }
        return ret;
    }
    return 0;
}

static inline int traverse_postorder(btree_node *node, m_btree_cb cb, void *userptr) {
    if (node) {
        int ret = traverse_postorder(node->left, cb, userptr);
        if (ret == 0) {
            ret = traverse_postorder(node->right, cb, userptr);
            if (ret == 0) {
                ret = cb(userptr, node->userptr);
            }
        }
        return ret;
    }
    return 0;
}

static inline int traverse_inorder(btree_node *node, m_btree_cb cb, void *userptr) {
    if (node) {
        int ret = traverse_inorder(node->left, cb, userptr);
        if (ret == 0) {
            ret = cb(userptr, node->userptr);
            if (ret == 0) {
                ret = traverse_inorder(node->right, cb, userptr);
            }
        }
        return ret;
    }
    return 0;
}

static inline btree_node **btree_find(m_btree_t *l, void *data, btree_node **parent) {
    btree_node **tmp = &l->root;
    while (*tmp) {
        const int ret = l->comp(data, (*tmp)->userptr);
        if (ret) {
            if (parent) {
                *parent = *tmp;
            }
            if (ret > 0) {
                tmp = &(*tmp)->right;
            } else {
                tmp = &(*tmp)->left;
            }
        } else {
            break;
        }
    }
    return tmp;
}

/* Thanks to: https://www.cs.odu.edu/~zeil/cs361/latest/Public/treetraversal/index.html#working-with-parents */
static inline btree_node **btree_next(btree_node **node) {
    if (*node) {
        if ((*node)->right) {
            node = find_min_subtree(&(*node)->right);
        } else {
            /*
             * We already processed the left subtree, and
             * there is no right subtree. 
             * Move up the tree, looking for a parent for which curr is a right child,
             * stopping if the parent becomes NULL. 
             * A non-NULL parent is the successor. 
             * If parent is NULL, the original node was the last node inorder, 
             * and its successor is the end of the list
             */
            btree_node **parent = &(*node)->parent;
            while (*parent && *node == (*parent)->right) {
                node = parent;
                parent = &(*node)->parent;
            }
            /*
             * If we were previously at the right-most node in
             * the tree, the iterator specifies the end of the list
             */
            node = parent;
        }
    }
    return node;
}

/** Public API **/

m_btree_t *m_btree_new(const m_btree_cmp comp, const m_btree_dtor fn) {
    m_btree_t *l = memhook._calloc(1, sizeof(m_btree_t));
    if (l) {
        l->dtor = fn;
        l->comp = comp ? comp : ptrcmp;
    }
    return l;
}

int m_btree_insert(m_btree_t *l, void *data) {
    MOD_PARAM_ASSERT(l);
    MOD_PARAM_ASSERT(data);
    
    btree_node *parent = NULL;
    btree_node **node = btree_find(l, data, &parent);
    if (*node) {
        return -EEXIST;
    }
    return insert_node(l, node, parent, data);
}

int m_btree_remove(m_btree_t *l, void *data) {
    MOD_PARAM_ASSERT(m_btree_length(l) > 0);
    MOD_PARAM_ASSERT(data);
    
    btree_node **node = btree_find(l, data, NULL);
    if (node) {
        return remove_node(l, node);
    }
    return -ENOENT;
}

void *m_btree_find(m_btree_t *l, void *data) {
    MOD_RET_ASSERT(l, NULL);
    MOD_RET_ASSERT(data, NULL);
    
    btree_node **node = btree_find(l, data, NULL);
    if (*node) {
        return (*node)->userptr;
    }
    return NULL;
}

int m_btree_traverse(m_btree_t *l, m_btree_order type, m_btree_cb cb, void *userptr) {
    int ret;
    switch (type) {
    case M_BTREE_PRE:
        ret = traverse_preorder(l->root, cb, userptr);
        break;
    case M_BTREE_POST:
        ret = traverse_postorder(l->root, cb, userptr);
        break;
    case M_BTREE_IN:
        ret = traverse_inorder(l->root, cb, userptr);
        break;
    default:
        ret = -EINVAL;
        break;
    }
    return ret >= 0 ? 0 : ret;
}

m_btree_itr_t *m_btree_itr_new(const m_btree_t *l) {
    MOD_RET_ASSERT(m_btree_length(l) > 0, NULL);

    m_btree_itr_t *itr = memhook._calloc(1, sizeof(m_btree_itr_t));
    if (itr) {
        itr->curr = find_min_subtree((btree_node **)&l->root);
        itr->l = (m_btree_t *)l;
    }
    return itr;
}

m_btree_itr_t *m_btree_itr_next(m_btree_itr_t *itr) {
    MOD_RET_ASSERT(itr, NULL);

    if (!itr->removed) {
        itr->prev = *itr->curr;
        itr->curr = btree_next(itr->curr);
    } else if (itr->prev) {
        /* If we have a previous element, find new next of that element */
        itr->curr = btree_next(&itr->prev);
    } else if (itr->l->root) {
        /* 
         * If we haven't got a previous elem, it means we have freed
         * first iterator (or we are freeing all iterators)
         * Next will be new root's min_subtree 
         */
        itr->curr = find_min_subtree((btree_node **)&itr->l->root);
    } else {
        /* Everything was removed. End iteration. */
        itr->curr = &itr->l->root;
    }
    itr->removed = false;
    if (!*(itr->curr)) {
        memhook._free(itr);
        itr = NULL;
    }
    return itr;
}

int m_btree_itr_remove(m_btree_itr_t *itr) {
    MOD_PARAM_ASSERT(itr);
    MOD_PARAM_ASSERT(*(itr->curr));
    MOD_PARAM_ASSERT(!itr->removed);
    
    /*
     * remove_node() does expect address of pointer to
     * node as seen by its parent (ie: &parent->{left,right}).
     * While iterating, our node can be reached through 
     * its child 'parent' pointer, (ie: &child->parent);
     * This happens eg: while climbing up left subtree.
     * 
     * Normalize to expected remove_node() pointer.
     */
    btree_node **node = NULL;
    if ((*itr->curr)->parent) {
        node = &(*itr->curr)->parent;
        if (itr->curr == &(*node)->right) {
            node = &(*node)->right;
        } else {
            node = &(*node)->left;
        }
    } else {
        node = &(itr->l->root);
    }
    int ret = remove_node(itr->l, node);
    if (ret == 0) {
        itr->removed = true;
    }
    return ret;
}

void *m_btree_itr_get_data(const m_btree_itr_t *itr) {
    MOD_RET_ASSERT(itr, NULL);
    MOD_RET_ASSERT(*(itr->curr), NULL);

    return (*itr->curr)->userptr;
}

int m_btree_clear(m_btree_t *l) {
    MOD_PARAM_ASSERT(m_btree_length(l) > 0);
    
    for (m_btree_itr_t *itr = m_btree_itr_new(l); itr; itr = m_btree_itr_next(itr)) {
        m_btree_itr_remove(itr);
    }
    l->root = NULL;
    return 0;
}

int m_btree_free(m_btree_t **l) {
    MOD_PARAM_ASSERT(l);
    
    m_btree_clear(*l);
    memhook._free(*l);
    *l = NULL;
    return 0;
}

ssize_t m_btree_length(const m_btree_t *l) {
    MOD_PARAM_ASSERT(l);
    
    return l->len;
}

