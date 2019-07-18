#include "test_map.h"
#include <module/stack.h>

static stack_t *my_st;
static int val1 = 1;
static int val2 = 2;
static char val3[] = "Hello World";

void test_stack_push(void **state) {
    (void) state; /* unused */
    
    /* NULL map */
    stack_ret_code ret = stack_push(my_st, &val1);
    assert_false(ret == STACK_OK);
    
    my_st = stack_new(NULL);
    
    /* NULL value */
    ret = stack_push(my_st, NULL);
    assert_false(ret == STACK_OK);
    
    ret = stack_push(my_st, &val1);
    assert_true(ret == STACK_OK);
    
    ret = stack_push(my_st, &val2);
    assert_true(ret == STACK_OK);
    
    ret = stack_push(my_st, val3);
    assert_true(ret == STACK_OK);
}

void test_stack_peek(void **state) {
    (void) state; /* unused */
    
    char *v = stack_peek(NULL);
    assert_null(v);
    
    v = stack_peek(my_st);
    assert_non_null(v);
    assert_string_equal(v, "Hello World");
}

void test_stack_length(void **state) {
    (void) state; /* unused */
    
    int len = stack_length(NULL);
    assert_false(len > 0);
    
    len = stack_length(my_st);
    assert_int_equal(len, 3);
}

void test_stack_iterator(void **state) {
    (void) state; /* unused */
    
    /* NULL stack */
    stack_itr_t *itr = stack_itr_new(NULL);
    assert_null(itr);
    
    itr = stack_itr_new(my_st);
    assert_non_null(itr);
    
    int count = stack_length(my_st);
    while (itr) {
        count--;
        printf("%p\n", stack_itr_get_data(itr));
        itr = stack_itr_next(itr);
    }
    
    assert_int_equal(count, 0);
}

void test_stack_pop(void **state) {
    (void) state; /* unused */
    
    void *ptr = stack_pop(NULL);
    assert_null(ptr);
    
    ptr = stack_pop(my_st);
    assert_non_null(ptr);
    assert_string_equal(ptr, "Hello World");
    
    ptr = stack_pop(my_st);
    assert_non_null(ptr);
    assert_int_equal(*(int *)ptr, 2);
    
    int len = stack_length(my_st);
    assert_int_equal(len, 1); // one element left
}

void test_stack_clear(void **state) {
    (void) state; /* unused */
    
    stack_ret_code ret = stack_clear(NULL);
    assert_false(ret == STACK_OK);
    
    ret = stack_clear(my_st);
    assert_true(ret == STACK_OK);
    
    int len = stack_length(my_st);
    assert_int_equal(len, 0);
}

void test_stack_free(void **state) {
    (void) state; /* unused */
    
    stack_ret_code ret = stack_free(NULL);
    assert_false(ret == STACK_OK);
    
    ret = stack_free(my_st);
    assert_true(ret == STACK_OK);
}
