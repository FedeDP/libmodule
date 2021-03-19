#include "test_thpool.h"
#include "module/thpool.h"
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>

#define NUM_THREADS     8
#define NUM_JOBS        64

static void *inc(void *udata);

static atomic_int ctr;

void test_thpool(void **state) {
    (void) state; /* unused */

    m_thpool_t *pool = m_thpool_new(NUM_THREADS, NULL);
    assert_non_null(pool);
    for (int i = 0; i < NUM_JOBS; i++) {
        int ret = m_thpool_add(pool, inc, NULL);
        assert_int_equal(ret, 0);
    }

    /* wait any enqueued job */
    int ret = m_thpool_free(&pool, true);
    assert_int_equal(ret, 0);

    assert_int_equal(ctr, NUM_JOBS);
}

static void *inc(void *udata) {
    ctr++;
    return NULL;
}