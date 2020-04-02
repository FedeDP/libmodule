#include "test_modules.h"
#include <module/modules_easy.h>
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
    
    mod_ret ret = modules_ctx_set_logger(NULL, logger);
    assert_false(ret == MOD_OK);
}

void test_modules_ctx_set_logger_NULL_logger(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = modules_ctx_set_logger(CTX, NULL);
    assert_false(ret == MOD_OK);
}

void test_modules_ctx_set_logger(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = modules_ctx_set_logger(CTX, logger);
    assert_true(ret == MOD_OK);
}

void test_modules_ctx_set_logger_no_ctx(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = modules_ctx_set_logger(CTX, logger);
    assert_false(ret == MOD_OK);
}

void test_modules_ctx_dump_no_ctx(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = modules_ctx_dump(CTX);
    assert_false(ret == MOD_OK);
}

void test_modules_ctx_dump(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = modules_ctx_dump(CTX);
    assert_true(ret == MOD_OK);
}

void test_modules_ctx_quit_NULL_ctx(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = modules_ctx_quit(NULL, 0);
    assert_false(ret == MOD_OK);
}

void test_modules_ctx_quit_no_loop(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = modules_ctx_quit(CTX, 0);
    assert_false(ret == MOD_OK);
}

void test_modules_ctx_loop_NULL_ctx(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = modules_ctx_loop(NULL);
    assert_false(ret == MOD_OK);
}

void test_modules_ctx_loop_no_maxevents(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = modules_ctx_loop_events(CTX, 0);
    assert_false(ret == MOD_OK);
}

void test_modules_ctx_loop(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = modules_ctx_loop(CTX); // modules_quit() is called with "number of USER PS messages" recv'd.
    assert_true(ret == 3);
}

void test_modules_ctx_dispatch_NULL_param(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = modules_ctx_dispatch(CTX, NULL);
    assert_false(ret == MOD_OK);
}

void test_modules_ctx_dispatch_NULL_ctx(void **state) {
    (void) state; /* unused */
    
    int r;
    mod_ret ret = modules_ctx_dispatch(NULL, &r);
    assert_false(ret == MOD_OK);
}

void test_modules_ctx_dispatch(void **state) {
    (void) state; /* unused */
    
    int r;
    mod_ret ret = modules_ctx_dispatch(CTX, &r);
    assert_true(ret == MOD_OK && r == 0);  // loop started
    
    ret = modules_ctx_dispatch(CTX, &r);
    assert_true(ret == MOD_OK && r == 2); // number of messages dispatched: LOOP_STARTED msg + fuse init message
    
    ret = modules_ctx_quit(CTX, 0);
    assert_true(ret == MOD_OK);
    
    ret = modules_ctx_dispatch(CTX, &r);
    assert_true(ret == MOD_ERR && r == 0); // loop stopped with exit code 0
}
