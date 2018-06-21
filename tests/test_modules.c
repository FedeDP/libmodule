#include "test_modules.h"
#include <module/modules.h>
#include <module/module.h>
#include <stdlib.h>

static void logger(const self_t *self, const char *fmt, va_list args, const void *userdata) {
    char *name = NULL, *context = NULL;
    if (module_get_name(self, &name) == MOD_OK && 
        module_get_context(self, &context) == MOD_OK) {
        
        printf("%s@%s:\t*", name, context);
        vprintf(fmt, args);
        free(name);
        free(context);
    }
}

void test_modules_ctx_set_logger_NULL_ctx(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = modules_ctx_set_logger(NULL, logger);
    assert_false(ret == MOD_OK);
}

void test_modules_ctx_set_logger_NULL_logger(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = modules_ctx_set_logger(CTX, NULL);
    assert_false(ret == MOD_OK);
}

void test_modules_ctx_set_logger(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = modules_ctx_set_logger(CTX, logger);
    assert_true(ret == MOD_OK);
}

void test_modules_ctx_set_logger_no_ctx(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = modules_ctx_set_logger(CTX, logger);
    assert_false(ret == MOD_OK);
}

void test_modules_ctx_quit_NULL_ctx(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = modules_ctx_quit(NULL);
    assert_false(ret == MOD_OK);
}

void test_modules_ctx_quit_no_loop(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = modules_ctx_quit(CTX);
    assert_false(ret == MOD_OK);
}

void test_modules_ctx_loop_NULL_ctx(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = modules_ctx_loop(NULL);
    assert_false(ret == MOD_OK);
}

void test_modules_ctx_loop_no_maxevents(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = modules_ctx_loop_events(CTX, 0);
    assert_false(ret == MOD_OK);
}

void test_modules_ctx_loop_no_fds(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = modules_ctx_loop(CTX);
    assert_false(ret == MOD_OK);
}