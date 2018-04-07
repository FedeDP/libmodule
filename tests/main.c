#include "test_module.h"
#include "test_modules.h"

int main(void) {
    const struct CMUnitTest tests[] = {
        /* Test module_register failures */
        cmocka_unit_test(test_module_register_NULL_name),
        cmocka_unit_test(test_module_register_NULL_ctx),
        cmocka_unit_test(test_module_register_NULL_self),
        cmocka_unit_test(test_module_register_NULL_hook),
        
        /* Actually register a module */
        cmocka_unit_test(test_module_register),
        
        /* We have a module and its ctx now! */
        
        /* Test modules_ API */
        cmocka_unit_test(test_modules_ctx_set_logger_NULL_ctx),
        cmocka_unit_test(test_modules_ctx_set_logger_NULL_logger),
        cmocka_unit_test(test_modules_ctx_set_logger),
        cmocka_unit_test(test_modules_ctx_loop_NULL_ctx),
        cmocka_unit_test(test_modules_ctx_loop_no_fds),
        cmocka_unit_test(test_modules_ctx_quit_NULL_ctx),
        cmocka_unit_test(test_modules_ctx_quit_no_loop),
        
        /* Test module state setters */
        cmocka_unit_test(test_module_pause_NULL_self),
        cmocka_unit_test(test_module_pause),
        cmocka_unit_test(test_module_resume_NULL_self),
        cmocka_unit_test(test_module_resume),
        cmocka_unit_test(test_module_stop_NULL_self),
        cmocka_unit_test(test_module_stop),
        cmocka_unit_test(test_module_start_NULL_self),
        cmocka_unit_test(test_module_start),
        
        /* Test module_deregister failures */
        cmocka_unit_test(test_module_deregister_NULL_self),
        
        /* Actually deregister our module */
        cmocka_unit_test(test_module_deregister),
        
        /* We have no more our module and its ctx */
        
        /* Test modules_ API: it should fail now */
        cmocka_unit_test(test_modules_ctx_set_logger_no_ctx), // here context is already destroyed
        cmocka_unit_test(test_modules_ctx_loop_no_fds),  // here context is already destroyed
        cmocka_unit_test(test_modules_ctx_quit_no_loop) // here context is already destroyed
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
