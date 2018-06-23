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
        
        /*
         * Check that a module cannot be registered more that one time 
         * if module_deregister does not get called before
         */
        cmocka_unit_test(test_module_register_already_registered),
        /* 
         * Check that another module register fails 
         * if module has same name and same context 
         */
        cmocka_unit_test(test_module_register_same_name),
        
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
        
        /* Test module logger */
        cmocka_unit_test(test_module_log_NULL_self),
        cmocka_unit_test(test_module_log),
        
        /* Test module set userdata */
        cmocka_unit_test(test_module_set_userdata_NULL_self),
        cmocka_unit_test(test_module_set_userdata),
        
        /* Test module become */
        cmocka_unit_test(test_module_become_NULL_self),
        cmocka_unit_test(test_module_become_NULL_func),
        cmocka_unit_test(test_module_become),
        
        /* Test fd add/rm */
        cmocka_unit_test(test_module_add_wrong_fd),
        cmocka_unit_test(test_module_add_fd_NULL_self),
        cmocka_unit_test(test_module_add_fd),
        cmocka_unit_test(test_module_rm_wrong_fd),
        cmocka_unit_test(test_module_rm_wrong_fd_2),
        cmocka_unit_test(test_module_rm_fd_NULL_self),
        cmocka_unit_test(test_module_rm_fd),
        
        /* Test module subscribe */
        cmocka_unit_test(test_module_subscribe_NULL_topic),
        cmocka_unit_test(test_module_subscribe_NULL_self),
        cmocka_unit_test(test_module_subscribe),
        
        /* Test module tell */
        cmocka_unit_test(test_module_tell_NULL_recipient),
        cmocka_unit_test(test_module_tell_unhexistent_recipient),
        cmocka_unit_test(test_module_tell_NULL_self),
        cmocka_unit_test(test_module_tell_NULL_msg),
        cmocka_unit_test(test_module_tell),
        
        /* Test module publish */
        cmocka_unit_test(test_module_publish_NULL_self),
        cmocka_unit_test(test_module_publish_NULL_msg),
        cmocka_unit_test(test_module_publish_NULL_topic),
        cmocka_unit_test(test_module_publish),
        
        /* Test module_deregister failures */
        cmocka_unit_test(test_module_deregister_NULL_self),
        
        /* Actually deregister our module */
        cmocka_unit_test(test_module_deregister),
        
        /* We have no more our module and its ctx */
        cmocka_unit_test(test_modules_ctx_loop_no_maxevents),
        
        /* Test modules_ API: it should fail now */
        cmocka_unit_test(test_modules_ctx_set_logger_no_ctx), // here context is already destroyed
        cmocka_unit_test(test_modules_ctx_loop_no_fds),  // here context is already destroyed
        cmocka_unit_test(test_modules_ctx_quit_no_loop) // here context is already destroyed
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
