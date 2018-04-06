#include "test_module.h"

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_module_register_NULL_name),
        cmocka_unit_test(test_module_register_NULL_ctx),
        cmocka_unit_test(test_module_register_NULL_self),
        cmocka_unit_test(test_module_register_NULL_hook),
        cmocka_unit_test(test_module_register),
        cmocka_unit_test(test_module_deregister_NULL_self),
        cmocka_unit_test(test_module_deregister)
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
