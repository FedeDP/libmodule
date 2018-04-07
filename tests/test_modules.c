#include "test_modules.h"
#include <modules.h>

static void logger(const char *module_name, const char *context_name, 
                   const char *fmt, va_list args, const void *userdata) {
    printf("%s@%s:\t*", module_name, context_name);
    vprintf(fmt, args);
                   }

void test_modules_ctx_set_logger_NULL_ctx(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = modules_ctx_set_logger(NULL, logger);
    assert_false(ret == MOD_OK);
}

void test_modules_ctx_set_logger_NULL_logger(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = modules_ctx_set_logger(ctx, NULL);
    assert_false(ret == MOD_OK);
}

void test_modules_ctx_set_logger(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = modules_ctx_set_logger(ctx, logger);
    assert_true(ret == MOD_OK);
}

void test_modules_ctx_set_logger_no_ctx(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = modules_ctx_set_logger(ctx, logger);
    assert_false(ret == MOD_OK);
}

void test_modules_ctx_quit_NULL_ctx(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = modules_ctx_quit(NULL);
    assert_false(ret == MOD_OK);
}

void test_modules_ctx_quit_no_loop(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = modules_ctx_quit(ctx);
    assert_false(ret == MOD_OK);
}

void test_modules_ctx_loop_NULL_ctx(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = modules_ctx_loop(NULL);
    assert_false(ret == MOD_OK);
}

void test_modules_ctx_loop_no_fds(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = modules_ctx_loop(ctx);
    assert_false(ret == MOD_OK);
}
