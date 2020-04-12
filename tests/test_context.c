#include "test_context.h"
#include <module/context_easy.h>
#include <module/module.h>
#include <stdlib.h>

static void logger(const self_t *self, const char *fmt, va_list args) {
    const char *name = NULL;
    const char *context = NULL;
    if (self) {
        name = module_name(self);
        context = module_ctx(self);
    }
    printf("%s@%s:\t* ", name ? name : "null", context ? context : "null");
    vprintf(fmt, args);
}

void test_modules_ctx_set_logger_NULL_ctx(void **state) {
    (void) state; /* unused */
    
    int ret = m_context_set_logger(NULL, logger);
    assert_false(ret == 0);
}

void test_modules_ctx_set_logger_NULL_logger(void **state) {
    (void) state; /* unused */
    
    int ret = m_context_set_logger(CTX, NULL);
    assert_false(ret == 0);
}

void test_modules_ctx_set_logger(void **state) {
    (void) state; /* unused */
    
    int ret = m_context_set_logger(CTX, logger);
    assert_true(ret == 0);
}

void test_modules_ctx_set_logger_no_ctx(void **state) {
    (void) state; /* unused */
    
    int ret = m_context_set_logger(CTX, logger);
    assert_false(ret == 0);
}

void test_modules_ctx_dump_no_ctx(void **state) {
    (void) state; /* unused */
    
    int ret = m_context_dump(CTX);
    assert_false(ret == 0);
}

void test_modules_ctx_dump(void **state) {
    (void) state; /* unused */
    
    int ret = m_context_dump(CTX);
    assert_true(ret == 0);
}

void test_modules_ctx_quit_NULL_ctx(void **state) {
    (void) state; /* unused */
    
    int ret = m_context_quit(NULL, 0);
    assert_false(ret == 0);
}

void test_modules_ctx_quit_no_loop(void **state) {
    (void) state; /* unused */
    
    int ret = m_context_quit(CTX, 0);
    assert_false(ret == 0);
}

void test_modules_ctx_loop_NULL_ctx(void **state) {
    (void) state; /* unused */
    
    int ret = m_context_loop(NULL);
    assert_false(ret == 0);
}

void test_modules_ctx_loop_no_maxevents(void **state) {
    (void) state; /* unused */
    
    int ret = m_context_loop_events(CTX, 0);
    assert_false(ret == 0);
}

void test_modules_ctx_loop(void **state) {
    (void) state; /* unused */
    
    int ret = m_context_loop(CTX); // modules_quit() is called with "number of USER PS messages" recv'd.
    assert_true(ret == 3);
}

void test_modules_ctx_dispatch_NULL_ctx(void **state) {
    (void) state; /* unused */
    
    int ret = m_context_dispatch(NULL);
    assert_false(ret == 0);
}

void test_modules_ctx_dispatch(void **state) {
    (void) state; /* unused */
    
    int ret = m_context_dispatch(CTX);
    assert_true(ret == 0);  // loop started
    
    ret = m_context_dispatch(CTX);
#ifdef WITH_FS
    assert_true(ret == 2); // number of messages dispatched: LOOP_STARTED msg + fuse init message
#else
    assert_true(ret == 1); // number of messages dispatched: LOOP_STARTED
#endif
    ret = m_context_quit(CTX, 0);
    assert_true(ret == 0);
    
    ret = m_context_dispatch(CTX);
    assert_true(ret == 0); // loop stopped with exit code 0
}
