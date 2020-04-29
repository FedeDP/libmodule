#include "test_commons.h"

void test_ctx_register_NULL_name(void **state);
void test_ctx_register(void **state);
void test_ctx_deregister_NULL_name(void **state);
void test_ctx_deregister(void **state);
void test_modules_ctx_set_logger_NULL_ctx(void **state);
void test_modules_ctx_set_logger_NULL_logger(void **state);
void test_modules_ctx_set_logger(void **state);
void test_modules_ctx_set_logger_no_ctx(void **state);
void test_modules_ctx_dump_no_ctx(void **state);
void test_modules_ctx_dump(void **state);
void test_modules_ctx_quit_NULL_ctx(void **state);
void test_modules_ctx_quit_no_loop(void **state);
void test_modules_ctx_loop_NULL_ctx(void **state);
void test_modules_ctx_loop_no_maxevents(void **state);
void test_modules_ctx_loop(void **state);
void test_modules_ctx_dispatch_NULL_ctx(void **state);
void test_modules_ctx_dispatch(void **state);
