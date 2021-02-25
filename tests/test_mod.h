#include "test_commons.h"

void test_mod_register_NULL_name(void **state);
void test_mod_register_NULL_self(void **state);
void test_mod_register_NULL_hook(void **state);
void test_mod_register(void **state);
void test_mod_register_already_registered(void **state);
void test_mod_register_same_name(void **state);
void test_mod_deregister_NULL_self(void **state);
void test_mod_deregister(void **state);
void test_mod_false_init(void **state);
void test_mod_pause_NULL_self(void **state);
void test_mod_pause(void **state);
void test_mod_resume_NULL_self(void **state);
void test_mod_resume(void **state);
void test_mod_stop_NULL_self(void **state);
void test_mod_stop(void **state);
void test_mod_start_NULL_self(void **state);
void test_mod_start(void **state);
void test_mod_log_NULL_self(void **state);
void test_mod_log(void **state);
void test_mod_dump_NULL_self(void **state);
void test_mod_dump(void **state);
void test_mod_become_NULL_self(void **state);
void test_mod_become_NULL_func(void **state);
void test_mod_become(void **state);
void test_mod_unbecome_NULL_self(void **state);
void test_mod_unbecome(void **state);
void test_mod_add_wrong_fd(void **state);
void test_mod_add_fd_NULL_self(void **state);
void test_mod_add_fd(void **state);
void test_mod_rm_wrong_fd(void **state);
void test_mod_rm_wrong_fd_2(void **state);
void test_mod_rm_fd_NULL_self(void **state);
void test_mod_rm_fd(void **state);
void test_mod_subscribe_NULL_topic(void **state);
void test_mod_subscribe_NULL_self(void **state);
void test_mod_subscribe(void **state);
void test_mod_ref_NULL_name(void **state);
void test_mod_ref_unexhistent_name(void **state);
void test_mod_ref(void **state);
void test_mod_unref_NULL_ref(void **state);
void test_mod_unref(void **state);
void test_mod_tell_NULL_recipient(void **state);
void test_mod_tell_NULL_self(void **state);
void test_mod_tell_NULL_msg(void **state);
void test_mod_tell(void **state);
void test_mod_publish_NULL_self(void **state);
void test_mod_publish_NULL_msg(void **state);
void test_mod_publish_NULL_topic(void **state);
void test_mod_publish(void **state);
void test_mod_broadcast_NULL_self(void **state);
void test_mod_broadcast_NULL_msg(void **state);
void test_mod_broadcast(void **state);
