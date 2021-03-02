#include "test_ctx.h"
#include <module/ctx.h>
#include <module/mod.h>
#include <stdlib.h>

#define CTX "testCtx"

m_ctx_t *test_ctx = NULL;

static void logger(const m_mod_t *self, const char *fmt, va_list args) {
    const char *name = NULL;
    const char *context = NULL;
    if (self) {
        name = m_mod_name(self);
        context = m_ctx_name(m_mod_ctx(self));
    }
    printf("%s@%s:\t* ", name ? name : "null", context ? context : "null");
    vprintf(fmt, args);
}

void test_ctx_register_NULL_name(void **state) {
    (void) state; /* unused */

    int ret = m_ctx_register(NULL, &test_ctx, 0, NULL);
    assert_false(ret == 0);
}

void test_ctx_register(void **state) {
    (void) state; /* unused */

    int ret = m_ctx_register(CTX, &test_ctx, M_CTX_PERSIST, NULL);
    assert_true(ret == 0);
}

void test_ctx_deregister_NULL_name(void **state) {
    (void) state; /* unused */

    int ret = m_ctx_deregister(NULL);
    assert_false(ret == 0);
}

void test_ctx_deregister(void **state) {
    (void) state; /* unused */

    int ret = m_ctx_deregister(&test_ctx);
    assert_true(ret == 0);
    assert_null(test_ctx);
}

void test_ctx_set_logger_NULL_logger(void **state) {
    (void) state; /* unused */
    
    int ret = m_ctx_set_logger(test_ctx, NULL);
    assert_false(ret == 0);
}

void test_ctx_set_logger(void **state) {
    (void) state; /* unused */
    
    int ret = m_ctx_set_logger(test_ctx, logger);
    assert_true(ret == 0);
}

void test_ctx_dump(void **state) {
    (void) state; /* unused */
    
    int ret = m_ctx_dump(test_ctx);
    assert_true(ret == 0);
}

void test_ctx_quit_no_loop(void **state) {
    (void) state; /* unused */
    
    int ret = m_ctx_quit(test_ctx, 0);
    assert_false(ret == 0);
}

void test_ctx_loop(void **state) {
    (void) state; /* unused */
    
    int ret = m_ctx_loop(test_ctx);
    assert_true(ret == 0);
}

void test_ctx_dispatch(void **state) {
    (void) state; /* unused */

    int ret = m_ctx_dispatch(test_ctx);
    assert_true(ret == 0);  // loop started

    ret = m_ctx_dispatch(test_ctx);
    assert_true(ret == 1); // number of messages dispatched: LOOP_STARTED.

    ret = m_ctx_quit(test_ctx, 0);
    assert_true(ret == 0);

    ret = m_ctx_dispatch(test_ctx);
    assert_true(ret == 0); // loop stopped with exit code 0
}
