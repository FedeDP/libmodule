#include "test_list.h"
#include <module/list.h>
#include <module/itr.h>

static m_list_t *my_l;
static int val1 = 1;
static int val2 = 2;
static int val3 = 3;

void test_list_insert(void **state) {
    (void) state; /* unused */
    
    /* NULL map */
    int ret = list_insert(my_l, &val1, NULL);
    assert_false(ret == 0);
    
    my_l = list_new(NULL);
    
    /* NULL value */
    ret = list_insert(my_l, NULL, NULL);
    assert_false(ret == 0);
    
    ret = list_insert(my_l, &val1, NULL);
    assert_true(ret == 0);
    
    ret = list_insert(my_l, &val2, NULL);
    assert_true(ret == 0);
    
    ret = list_insert(my_l, &val3, NULL);
    assert_true(ret == 0);
}

void test_list_length(void **state) {
    (void) state; /* unused */
    
    int len = list_length(NULL);
    assert_false(len > 0);
    
    len = list_length(my_l);
    assert_int_equal(len, 3);
}

void test_list_iterator(void **state) {
    (void) state; /* unused */
    
    /* NULL list */
    m_list_itr_t *itr = list_itr_new(NULL);
    assert_null(itr);
    
    itr = list_itr_new(my_l);
    assert_non_null(itr);
    
    int count = list_length(my_l);

    while (count) {
        count--;
        printf("%p\n", m_itr_get(itr));
        
        /* Insert a node */
        int ret = list_itr_insert(itr, &val1);
        assert_true(ret == 0);
        
        /* Remove previously inserted node */
        ret = m_itr_rm(itr);
        assert_true(ret == 0);
        
        m_itr_next(&itr);
    }
    
    assert_int_equal(count, 0);
    assert_null(itr);
}

static int int_match(void *my_data, void *list_data) {
    int a = *((int *)my_data);
    int b = *((int *)list_data);
    return !(a == b);
}

void test_list_find(void **state) {
    void *data = list_find(NULL, NULL, NULL);
    assert_null(data);
    
    data = list_find(my_l, NULL, NULL);
    assert_null(data);
    
    int c = 0;
    data = list_find(my_l, &c, NULL);
    assert_null(data);
    
    data = list_find(my_l, &val2, NULL);
    assert_non_null(data);
    assert_ptr_equal(data, &val2);
    
    c = val1;
    data = list_find(my_l, &c, int_match);
    assert_non_null(data);
    assert_ptr_equal(data, &val1);
}

void test_list_remove(void **state) {
    (void) state; /* unused */
    
    int ret = list_remove(NULL, NULL, NULL);
    assert_false(ret == 0);
    
    ret = list_remove(my_l, NULL, NULL);
    assert_false(ret == 0);
    
    ret = list_remove(my_l, &val1, NULL);
    assert_true(ret == 0);
    
    ret = list_remove(my_l, &val2, int_match);
    assert_true(ret == 0);
    
    int len = list_length(my_l);
    assert_int_equal(len, 1); // one element left, ie: val3
}

void test_list_clear(void **state) {
    (void) state; /* unused */
    
    int ret = list_clear(NULL);
    assert_false(ret == 0);
    
    ret = list_clear(my_l);
    assert_true(ret == 0);
    
    int len = list_length(my_l);
    assert_int_equal(len, 0);
}

void test_list_free(void **state) {
    (void) state; /* unused */
    
    int ret = list_free(NULL);
    assert_false(ret == 0);
    
    ret = list_free(&my_l);
    assert_true(ret == 0);
    assert_null(my_l);
}
