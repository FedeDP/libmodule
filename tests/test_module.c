#include "test_module.h"
#include <module/module.h>
#include <module/modules.h>
#include <unistd.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <errno.h>
#include <string.h>

static bool init(void);
static bool init_false(void);
static bool evaluate(void);
static void recv(const msg_t *msg, const void *userdata);
static void destroy(void);

self_t *self = NULL;
const self_t *testSelf = NULL;

void test_module_register_NULL_name(void **state) {
    (void) state; /* unused */

    userhook_t hook = (userhook_t) { init, evaluate, recv, destroy };
    mod_ret ret = module_register(NULL, CTX, &self, &hook, 0);
    assert_false(ret == MOD_OK);
    assert_null(self);
}

void test_module_register_NULL_ctx(void **state) {
    (void) state; /* unused */
    
    userhook_t hook = (userhook_t) { init, evaluate, recv, destroy };
    mod_ret ret = module_register("testName", NULL, &self, &hook, 0);
    assert_false(ret == MOD_OK);
    assert_null(self);
}

void test_module_register_NULL_self(void **state) {
    (void) state; /* unused */
    
    userhook_t hook = (userhook_t) { init, evaluate, recv, destroy };
    mod_ret ret = module_register("testName", CTX, NULL, &hook, 0);
    assert_false(ret == MOD_OK);
    assert_null(self);
}

void test_module_register_NULL_hook(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_register("testName", CTX, &self, NULL, 0);    
    assert_false(ret == MOD_OK);
    assert_null(self);
}

void test_module_register(void **state) {
    (void) state; /* unused */
    
    userhook_t hook = (userhook_t) { init, evaluate, recv, destroy };
    mod_ret ret = module_register("testName", CTX, &self, &hook, 0);
    assert_true(ret == MOD_OK);
    assert_non_null(self);
    assert_true(module_is(self, IDLE));
}

void test_module_register_already_registered(void **state) {
    (void) state; /* unused */
    
    userhook_t hook = (userhook_t) { init, evaluate, recv, destroy };
    mod_ret ret = module_register("testName", CTX, &self, &hook, 0);
    assert_false(ret == MOD_OK);
    assert_non_null(self);
    assert_true(module_is(self, IDLE));
}

void test_module_register_same_name(void **state) {
    (void) state; /* unused */
    
    self_t *self2 = NULL;
    
    userhook_t hook = (userhook_t) { init, evaluate, recv, destroy };
    mod_ret ret = module_register("testName", CTX, &self2, &hook, 0);
    assert_false(ret == MOD_OK);
    assert_null(self2);
}

void test_module_deregister_NULL_self(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_deregister(NULL);
    assert_false(ret == MOD_OK);
    assert_non_null(self);
}

void test_module_deregister(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_deregister(&self);
    assert_true(ret == MOD_OK);
    assert_null(self);
}

void test_module_false_init(void **state) {
    (void) state; /* unused */
    
    userhook_t hook = (userhook_t) { init_false, evaluate, recv, destroy };
    mod_ret ret = module_register("testName", CTX, &self, &hook, 0);
    assert_true(ret == MOD_OK);
    assert_non_null(self);
    assert_true(module_is(self, IDLE));
    
    ret = module_start(self);
    assert_false(ret == MOD_OK);
    assert_true(module_is(self, STOPPED));
    
    ret = module_deregister(&self);
    assert_true(ret == MOD_OK);
    assert_null(self);
}

void test_module_pause_NULL_self(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_pause(NULL);
    assert_false(ret == MOD_OK);
    assert_true(module_is(self, RUNNING));
}

void test_module_pause(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_pause(self);
    assert_true(ret == MOD_OK);
    assert_true(module_is(self, PAUSED));
    
    /* Test module_tell for paused modules */
    ret = module_tell(self, self, (unsigned char *)"Paused!", strlen("Paused!"), false);
    assert_true(ret == MOD_OK);
}

void test_module_resume_NULL_self(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_resume(NULL);
    assert_false(ret == MOD_OK);
    assert_true(module_is(self, PAUSED));
}

void test_module_resume(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_resume(self);
    assert_true(ret == MOD_OK);
    assert_true(module_is(self, RUNNING));
}

void test_module_stop_NULL_self(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_stop(NULL);
    assert_false(ret == MOD_OK);
    assert_false(module_is(self, STOPPED));
}

void test_module_stop(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_stop(self);
    assert_true(ret == MOD_OK);
    assert_true(module_is(self, STOPPED));
}

void test_module_start_NULL_self(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_start(NULL);
    assert_false(ret == MOD_OK);
    assert_false(module_is(self, RUNNING));
}

void test_module_start(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_start(self);
    assert_true(ret == MOD_OK);
    assert_true(module_is(self, RUNNING));
}

void test_module_log_NULL_self(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_log(NULL, "Hi\n");
    assert_false(ret == MOD_OK);
}

void test_module_log(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_log(self, "Hi\n");
    assert_true(ret == MOD_OK);
}

void test_module_dump_NULL_self(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_dump(NULL);
    assert_false(ret == MOD_OK);
}

void test_module_dump(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_dump(self);
    assert_true(ret == MOD_OK);
}

void test_module_set_userdata_NULL_self(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_set_userdata(NULL, NULL);
    assert_false(ret == MOD_OK);
}

void test_module_set_userdata(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_set_userdata(self, NULL);
    assert_true(ret == MOD_OK);
}

void test_module_become_NULL_self(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_become(NULL, NULL);
    assert_false(ret == MOD_OK);
}

void test_module_become_NULL_func(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_become(self, NULL);
    assert_false(ret == MOD_OK);
}

void test_module_become(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_become(self, recv);
    assert_true(ret == MOD_OK);
}

void test_module_unbecome_NULL_self(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_unbecome(NULL);
    assert_false(ret == MOD_OK);
}

void test_module_unbecome(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_unbecome(self);
    assert_true(ret == MOD_OK);
    
    /* We don't have any more recv in stack */
    ret = module_unbecome(self);
    assert_false(ret == MOD_OK);
}

void test_module_add_wrong_fd(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_register_fd(self, -1, SRC_FD_AUTOCLOSE, NULL);
    assert_false(ret == MOD_OK);
}

void test_module_add_fd_NULL_self(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_register_fd(NULL, STDIN_FILENO, SRC_FD_AUTOCLOSE, NULL);
    assert_false(ret == MOD_OK);
}

void test_module_add_fd(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_register_fd(self, STDIN_FILENO, SRC_FD_AUTOCLOSE, NULL);
    assert_true(ret == MOD_OK);
}

void test_module_rm_wrong_fd(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_deregister_fd(self, -1);
    assert_false(ret == MOD_OK);
}

void test_module_rm_wrong_fd_2(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_deregister_fd(self, STDIN_FILENO + 1);
    assert_false(ret == MOD_OK);
}

void test_module_rm_fd_NULL_self(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_deregister_fd(NULL, STDIN_FILENO);
    assert_false(ret == MOD_OK);
}

static int fd_is_valid(int fd) {
    return fcntl(fd, F_GETFD) != -1 || errno != EBADF;
}

void test_module_rm_fd(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_deregister_fd(self, STDIN_FILENO);
    assert_true(ret == MOD_OK);
    
    /* Fd is now closed (module_deregister_fd with 1 flag) thus is no more valid */
    assert_false(fd_is_valid(STDIN_FILENO));
}

void test_module_subscribe_NULL_topic(void **state) {
    (void) state; /* unused */
    
    // module_register_source won't even let you use NULL as topic as it only expects "int" or "(const) char *"
    mod_ret ret = module_register_sub(self, NULL, 0, NULL);
    assert_false(ret == MOD_OK);
}

void test_module_subscribe_NULL_self(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_register_src(NULL, "topic", 0, NULL);
    assert_false(ret == MOD_OK);
}

void test_module_subscribe(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_register_src(self, "topic", 0, NULL);
    assert_true(ret == MOD_OK);
}

void test_module_ref_NULL_name(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_ref(self, NULL, &testSelf);
    assert_false(ret == MOD_OK);
}

void test_module_ref_unexhistent_name(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_ref(self, "testName2", &testSelf);
    assert_false(ret == MOD_OK);
}

void test_module_ref_NULL_ref(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_ref(self, "testName2", NULL);
    assert_false(ret == MOD_OK);
}

void test_module_ref(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_ref(self, "testName", &testSelf);
    assert_true(ret == MOD_OK);
}

void test_module_tell_NULL_recipient(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_tell(self, NULL, (unsigned char *)"hi!", strlen("hi!"), 0);
    assert_false(ret == MOD_OK);
}

void test_module_tell_NULL_self(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_tell(NULL, testSelf, (unsigned char *)"hi!", strlen("hi!"), 0);
    assert_false(ret == MOD_OK);
}

void test_module_tell_NULL_msg(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_tell(self, testSelf, NULL, 10, 0);
    assert_false(ret == MOD_OK);
}

void test_module_tell_wrong_size(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_tell(self, testSelf, (unsigned char *)"hi!", -1, 0);
    assert_false(ret == MOD_OK);
}

/* Test automatic free of message too! */
void test_module_tell(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_tell(self, testSelf, (unsigned char *)"hi1!", strlen("hi!"), PS_DUP_DATA);
    assert_true(ret == MOD_OK);
}

void test_module_publish_NULL_self(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_publish(NULL, "topic", (unsigned char *)"hi!", strlen("hi!"), 0);
    assert_false(ret == MOD_OK);
}

void test_module_publish_NULL_msg(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_publish(self, "topic", NULL, 10, 0);
    assert_false(ret == MOD_OK);
}

void test_module_publish_NULL_topic(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_publish(self, NULL, (unsigned char *)"hi!", strlen("hi!"), 0);
    assert_false(ret == MOD_OK);
}

void test_module_publish_wrong_size(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_publish(self, "topic", (unsigned char *)"hi!", -1, 0);
    assert_false(ret == MOD_OK);
}

void test_module_publish(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_publish(self, "topic", (unsigned char *)"hi2!", strlen("hi!"), 0);
    assert_true(ret == MOD_OK);
}

void test_module_broadcast_NULL_self(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_broadcast(NULL, (unsigned char *)"hi!", strlen("hi!"), 0);
    assert_false(ret == MOD_OK);
}

void test_module_broadcast_NULL_msg(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_broadcast(self, NULL, strlen("hi!"), 0);
    assert_false(ret == MOD_OK);
}

void test_module_broadcast_wrong_size(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_broadcast(self, (unsigned char *)"hi!", -1, 0);
    assert_false(ret == MOD_OK);
}

void test_module_broadcast(void **state) {
    (void) state; /* unused */
    
    mod_ret ret = module_broadcast(self, (unsigned char *)"hi3!", strlen("hi!"), 0);
    assert_true(ret == MOD_OK);
}

static bool init(void) {
    return true;
}

static bool init_false(void) {
    return false;
}

static bool evaluate(void) {
    return true;
}

static void recv(const msg_t *msg, const void *userdata) {
    static int ctr = 0;
    if (msg->type == TYPE_PS && msg->ps_msg->type == USER) {
        ctr++;
        if (!strcmp((char *)msg->ps_msg->data, "hi3!")) {
            modules_ctx_quit(CTX, ctr);
        }
    }
}

static void destroy(void) {
    
}
