#include "test_queue.h"
#include <module/queue.h>
#include <module/itr.h>

static m_queue_t *my_q;
static int val1 = 1;
static int val2 = 2;
static char val3[] = "Hello World";

void test_queue_enqueue(void **state) {
    (void) state; /* unused */
    
    /* NULL map */
    int ret = queue_enqueue(my_q, &val1);
    assert_false(ret == 0);
    
    my_q = queue_new(NULL);
    
    /* NULL value */
    ret = queue_enqueue(my_q, NULL);
    assert_false(ret == 0);
    
    ret = queue_enqueue(my_q, &val1);
    assert_true(ret == 0);
    
    ret = queue_enqueue(my_q, &val2);
    assert_true(ret == 0);
    
    ret = queue_enqueue(my_q, val3);
    assert_true(ret == 0);
}

void test_queue_peek(void **state) {
    (void) state; /* unused */
    
    void *v = queue_peek(NULL);
    assert_null(v);
    
    v = queue_peek(my_q);
    assert_non_null(v);
    assert_int_equal(*(int *)v, 1);
}

void test_queue_length(void **state) {
    (void) state; /* unused */
    
    int len = queue_length(NULL);
    assert_false(len > 0);
    
    len = queue_length(my_q);
    assert_int_equal(len, 3);
}

void test_queue_iterator(void **state) {
    (void) state; /* unused */
    
    /* NULL queue */
    m_queue_itr_t *itr = queue_itr_new(NULL);
    assert_null(itr);
    
    itr = m_itr_new(my_q);
    assert_non_null(itr);
    
    int count = queue_length(my_q);
    while (itr) {
        printf("%p\n", m_itr_get(itr));
        if (count % 2 == 0) {
            m_itr_rm(itr);
        }
        m_itr_next(&itr);
        count--;
    }
    
    assert_int_equal(count, 0);
    assert_null(itr);
}

void test_queue_dequeue(void **state) {
    (void) state; /* unused */
    
    void *ptr = queue_dequeue(NULL);
    assert_null(ptr);
    
    ptr = queue_dequeue(my_q);
    assert_non_null(ptr);
    assert_int_equal(*(int *)ptr, 1);
    
    int len = queue_length(my_q);
    assert_int_equal(len, 1); // one element left
}

void test_queue_clear(void **state) {
    (void) state; /* unused */
    
    int ret = queue_clear(NULL);
    assert_false(ret == 0);
    
    ret = queue_clear(my_q);
    assert_true(ret == 0);
    
    int len = queue_length(my_q);
    assert_int_equal(len, 0);
}

void test_queue_free(void **state) {
    (void) state; /* unused */
    
    int ret = queue_free(NULL);
    assert_false(ret == 0);
    
    ret = queue_free(&my_q);
    assert_true(ret == 0);
    assert_null(my_q);
}

