#include "test_list.h"
#include <module/list.h>

static list_t *my_l;
static int val1 = 1;
static int val2 = 2;
static int val3 = 3;

void test_list_insert(void **state) {
    (void) state; /* unused */
    
    /* NULL map */
    list_ret_code ret = list_insert(my_l, &val1, NULL);
    assert_false(ret == LIST_OK);
    
    my_l = list_new(NULL);
    
    /* NULL value */
    ret = list_insert(my_l, NULL, NULL);
    assert_false(ret == LIST_OK);
    
    ret = list_insert(my_l, &val1, NULL);
    assert_true(ret == LIST_OK);
    
    ret = list_insert(my_l, &val2, NULL);
    assert_true(ret == LIST_OK);
    
    ret = list_insert(my_l, &val3, NULL);
    assert_true(ret == LIST_OK);
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
    list_itr_t *itr = list_itr_new(NULL);
    assert_null(itr);
    
    itr = list_itr_new(my_l);
    assert_non_null(itr);
    
    int count = list_length(my_l);
    while (itr) {
        count--;
        printf("%p\n", list_itr_get_data(itr));
        list_ret_code ret = list_itr_insert(itr, &val1);
        assert_true(ret == LIST_OK);
        ret = list_itr_remove(itr);
        assert_true(ret == LIST_OK);
        itr = list_itr_next(itr);
    }
    
    assert_int_equal(count, 0);
    assert_null(itr);
}

static int int_match(void *my_data, void *list_data) {
    int a = *((int *)my_data);
    int b = *((int *)list_data);
    return !(a == b);
}

void test_list_remove(void **state) {
    (void) state; /* unused */
    
    list_ret_code ret = list_remove(NULL, NULL, NULL);
    assert_false(ret == LIST_OK);
    
    ret = list_remove(my_l, NULL, NULL);
    assert_false(ret == LIST_OK);
    
    ret = list_remove(my_l, &val1, NULL);
    assert_false(ret == LIST_OK);
    
    ret = list_remove(my_l, &val1, int_match);
    assert_true(ret == LIST_OK);
    
    ret = list_remove(my_l, &val2, int_match);
    assert_true(ret == LIST_OK);
    
    int len = list_length(my_l);
    assert_int_equal(len, 1); // one element left, ie: val3
}

void test_list_clear(void **state) {
    (void) state; /* unused */
    
    list_ret_code ret = list_clear(NULL);
    assert_false(ret == LIST_OK);
    
    ret = list_clear(my_l);
    assert_true(ret == LIST_OK);
    
    int len = list_length(my_l);
    assert_int_equal(len, 0);
}

void test_list_free(void **state) {
    (void) state; /* unused */
    
    list_ret_code ret = list_free(NULL);
    assert_false(ret == LIST_OK);
    
    ret = list_free(my_l);
    assert_true(ret == LIST_OK);
}
