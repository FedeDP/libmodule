#include "test_btree.h"
#include <module/bst.h>
#include <module/itr.h>

static int int_cmp(void *userdata, void *node_data) {
    int a = *((int *)userdata);
    int b = *((int *)node_data);
    return (a - b);
}

static int traverse_pre_cb(void *userptr, void *node_data) {
    static int counter = 0;
    int val = *((int *) node_data);
    printf("%d\n", val);
    if (++counter == 5) {
        return 5; // check that 5 is not forwarded but traverse is stopped
    }
    return 0;
}

static int traverse_post_cb(void *userptr, void *node_data) {
    int val = *((int *) node_data);
    printf("%d\n", val);
    return -1; // check that -1 is forwarded
}

static int traverse_in_cb(void *userptr, void *node_data) {
    int val = *((int *) node_data);
    printf("%d\n", val);
    return 0;
}

void test_btree(void **state) {
    (void) state; /* unused */
    
    m_bst_t *bt = m_bst_new(int_cmp, NULL);
    assert_non_null(bt);
    
    srand(time(NULL));
    int arr[100];
    for (int i = 0; i < 100; i++) {
        do {
            arr[i] = rand() % 10000;
        }
        while (m_bst_insert(bt, &arr[i]) == -EEXIST);
    }
    size_t len = m_bst_length(bt);
    assert_int_equal(len, 100);
    
    /* Remove some nodes */
    int ret = m_bst_remove(bt, &arr[5]);
    assert_int_equal(ret, 0);
   
    ret = m_bst_remove(bt, &arr[25]);
    assert_int_equal(ret, 0);
    
    ret = m_bst_remove(bt, &arr[45]);
    assert_int_equal(ret, 0);
    
    ret = m_bst_remove(bt, &arr[65]);
    assert_int_equal(ret, 0);
    
    ret = m_bst_remove(bt, &arr[85]);
    assert_int_equal(ret, 0);
    
    len = m_bst_length(bt);
    assert_int_equal(len, 95);
    
    ret = m_bst_clear(bt);
    assert_int_equal(ret, 0);
    
    len = m_bst_length(bt);
    assert_int_equal(len, 0);
    
    for (int i = 0; i < 10; i++) {
        m_bst_insert(bt, &arr[i]);
    }
    len = m_bst_length(bt);
    assert_int_equal(len, 10);
    
    printf("PREORDER (only first 5):\n");
    ret = m_bst_traverse(bt, M_BST_PRE, traverse_pre_cb, NULL);
    assert_int_equal(ret, 0);
    
    printf("POSTORDER (only first):\n");
    ret = m_bst_traverse(bt, M_BST_POST, traverse_post_cb, NULL);
    assert_int_equal(ret, -1);
    
    printf("INORDER:\n");
    ret = m_bst_traverse(bt, M_BST_IN, traverse_in_cb, NULL);
    assert_int_equal(ret, 0);
    
    printf("ITERATOR (inorder):\n");
    m_itr_foreach(bt, {
        int *val = m_itr_get(itr);
        printf("%d\n", *val);
        if (rand() % 2 == 1) {
            m_bst_itr_remove(itr);
        }
    });
    printf("END OF ITR\n");
    
    ret = m_bst_free(&bt);
    assert_int_equal(ret, 0);
    assert_null(bt);
}
