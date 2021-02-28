#include "test_ctx.h"
#include <module/ctx.h>
#include <module/mod.h>
#include <stdlib.h>

static void logger(const m_mod_t *self, const char *fmt, va_list args) {
    const char *name = NULL;
    if (self) {
        name = m_mod_name(self);
    }
    printf("%s@test:\t* ", name ? name : "null");
    vprintf(fmt, args);
}

void test_ctx_set_logger_NULL_logger(void **state) {
    (void) state; /* unused */
    
    int ret = m_ctx_set_logger(NULL);
    assert_false(ret == 0);
}

void test_ctx_set_logger(void **state) {
    (void) state; /* unused */
    
    int ret = m_ctx_set_logger(logger);
    assert_true(ret == 0);
}

void test_ctx_dump(void **state) {
    (void) state; /* unused */
    
    int ret = m_ctx_dump();
    assert_true(ret == 0);
}

void test_ctx_quit_no_loop(void **state) {
    (void) state; /* unused */
    
    int ret = m_ctx_quit(0);
    assert_false(ret == 0);
}

void test_ctx_loop(void **state) {
    (void) state; /* unused */
    
    int ret = m_ctx_loop(); // m_ctx_quit() is called with "number of USER PS messages" recv'd.
    assert_true(ret == 3);
}

void test_ctx_dispatch(void **state) {
    (void) state; /* unused */

    int ret = m_ctx_dispatch();
    assert_true(ret == 0);  // loop started

    ret = m_ctx_dispatch();
    assert_true(ret == 1); // number of messages dispatched: LOOP_STARTED.

    ret = m_ctx_quit(0);
    assert_true(ret == 0);

    ret = m_ctx_dispatch();
    assert_true(ret == 0); // loop stopped with exit code 0
}
