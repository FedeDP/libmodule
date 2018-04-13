#include "test_commons.h"

void test_module_register_NULL_name(void **state);
void test_module_register_NULL_ctx(void **state);
void test_module_register_NULL_self(void **state);
void test_module_register_NULL_hook(void **state);
void test_module_register(void **state);
void test_module_register_already_registered(void **state);
void test_module_register_same_name(void **state);
void test_module_deregister_NULL_self(void **state);
void test_module_deregister(void **state);
void test_module_pause_NULL_self(void **state);
void test_module_pause(void **state);
void test_module_resume_NULL_self(void **state);
void test_module_resume(void **state);
void test_module_stop_NULL_self(void **state);
void test_module_stop(void **state);
void test_module_start_NULL_self(void **state);
void test_module_start(void **state);
void test_module_log_NULL_self(void **state);
void test_module_log(void **state);
void test_module_set_userdata_NULL_self(void **state);
void test_module_set_userdata(void **state);
void test_module_become_NULL_self(void **state);
void test_module_become_NULL_func(void **state);
void test_module_become(void **state);
