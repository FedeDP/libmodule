#include "test_mod.h"
#include <module/mod.h>
#include <module/ctx.h>
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

mod_t *mod = NULL;
mod_t *testRef = NULL;

void test_mod_register_NULL_name(void **state) {
    (void) state; /* unused */

    userhook_t hook = (userhook_t) { init, evaluate, recv, destroy };
    int ret = m_mod_register(NULL, test_ctx, &mod, &hook, 0);
    assert_false(ret == 0);
    assert_null(mod);
}

void test_mod_register_NULL_self(void **state) {
    (void) state; /* unused */
    
    userhook_t hook = (userhook_t) { init, evaluate, recv, destroy };
    int ret = m_mod_register("testName", test_ctx, NULL, &hook, 0);
    assert_false(ret == 0);
    assert_null(mod);
}

void test_mod_register_NULL_hook(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_register("testName", test_ctx, &mod, NULL, 0);    
    assert_false(ret == 0);
    assert_null(mod);
}

void test_mod_register(void **state) {
    (void) state; /* unused */
    
    userhook_t hook = (userhook_t) { init, evaluate, recv, destroy };
    int ret = m_mod_register("testName", test_ctx, &mod, &hook, 0);
    assert_true(ret == 0);
    assert_non_null(mod);
    assert_true(m_mod_is(mod, IDLE));
}

void test_mod_register_already_registered(void **state) {
    (void) state; /* unused */
    
    userhook_t hook = (userhook_t) { init, evaluate, recv, destroy };
    int ret = m_mod_register("testName", test_ctx, &mod, &hook, 0);
    assert_false(ret == 0);
    assert_non_null(mod);
    assert_true(m_mod_is(mod, IDLE));
}

void test_mod_register_same_name(void **state) {
    (void) state; /* unused */
    
    mod_t *self2 = NULL;
    
    userhook_t hook = (userhook_t) { init, evaluate, recv, destroy };
    int ret = m_mod_register("testName", test_ctx, &self2, &hook, 0);
    assert_false(ret == 0);
    assert_null(self2);
}

void test_mod_deregister_NULL_self(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_deregister(NULL);
    assert_false(ret == 0);
    assert_non_null(mod);
}

void test_mod_deregister(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_deregister(&mod);
    assert_true(ret == 0);
    assert_null(mod);
}

void test_mod_false_init(void **state) {
    (void) state; /* unused */
    
    userhook_t hook = (userhook_t) { init_false, evaluate, recv, destroy };
    int ret = m_mod_register("testName", test_ctx, &mod, &hook, 0);
    assert_true(ret == 0);
    assert_non_null(mod);
    assert_true(m_mod_is(mod, IDLE));
    
    ret = m_mod_start(mod);
    assert_true(ret == 0);
    assert_true(m_mod_is(mod, STOPPED));
    
    ret = m_mod_deregister(&mod);
    assert_true(ret == 0);
    assert_null(mod);
}

void test_mod_pause_NULL_self(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_pause(NULL);
    assert_false(ret == 0);
    assert_true(m_mod_is(mod, RUNNING));
}

void test_mod_pause(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_pause(mod);
    assert_true(ret == 0);
    assert_true(m_mod_is(mod, PAUSED));
    
    /* Test module_tell for paused modules */
    ret = m_mod_tell(mod, mod, (unsigned char *)"Paused!", 0);
    assert_true(ret == 0);
}

void test_mod_resume_NULL_self(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_resume(NULL);
    assert_false(ret == 0);
    assert_true(m_mod_is(mod, PAUSED));
}

void test_mod_resume(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_resume(mod);
    assert_true(ret == 0);
    assert_true(m_mod_is(mod, RUNNING));
}

void test_mod_stop_NULL_self(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_stop(NULL);
    assert_false(ret == 0);
    assert_false(m_mod_is(mod, STOPPED));
}

void test_mod_stop(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_stop(mod);
    assert_true(ret == 0);
    assert_true(m_mod_is(mod, STOPPED));
}

void test_mod_start_NULL_self(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_start(NULL);
    assert_false(ret == 0);
    assert_false(m_mod_is(mod, RUNNING));
}

void test_mod_start(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_start(mod);
    assert_true(ret == 0);
    assert_true(m_mod_is(mod, RUNNING));
}

void test_mod_log_NULL_self(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_log(NULL, "Hi\n");
    assert_false(ret == 0);
}

void test_mod_log(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_log(mod, "Hi\n");
    assert_true(ret == 0);
}

void test_mod_dump_NULL_self(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_dump(NULL);
    assert_false(ret == 0);
}

void test_mod_dump(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_dump(mod);
    assert_true(ret == 0);
}

void test_mod_set_userdata_NULL_self(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_set_userdata(NULL, NULL);
    assert_false(ret == 0);
}

void test_mod_set_userdata(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_set_userdata(mod, NULL);
    assert_true(ret == 0);
}

void test_mod_become_NULL_self(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_become(NULL, NULL);
    assert_false(ret == 0);
}

void test_mod_become_NULL_func(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_become(mod, NULL);
    assert_false(ret == 0);
}

void test_mod_become(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_become(mod, recv);
    assert_true(ret == 0);
}

void test_mod_unbecome_NULL_self(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_unbecome(NULL);
    assert_false(ret == 0);
}

void test_mod_unbecome(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_unbecome(mod);
    assert_true(ret == 0);
    
    /* We don't have any more recv in stack */
    ret = m_mod_unbecome(mod);
    assert_false(ret == 0);
}

void test_mod_add_wrong_fd(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_register_fd(mod, -1, M_SRC_FD_AUTOCLOSE, NULL);
    assert_false(ret == 0);
}

void test_mod_add_fd_NULL_self(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_register_fd(NULL, STDIN_FILENO, M_SRC_FD_AUTOCLOSE, NULL);
    assert_false(ret == 0);
}

void test_mod_add_fd(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_register_fd(mod, STDIN_FILENO, M_SRC_FD_AUTOCLOSE, NULL);
    assert_true(ret == 0);
    
    /* Try to register again */
    ret = m_mod_register_fd(mod, STDIN_FILENO, M_SRC_FD_AUTOCLOSE, NULL);
    assert_true(ret == -EEXIST);
}

void test_mod_rm_wrong_fd(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_deregister_fd(mod, -1);
    assert_false(ret == 0);
}

void test_mod_rm_wrong_fd_2(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_deregister_fd(mod, STDIN_FILENO + 1);
    assert_false(ret == 0);
}

void test_mod_rm_fd_NULL_self(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_deregister_fd(NULL, STDIN_FILENO);
    assert_false(ret == 0);
}

static int fd_is_valid(int fd) {
    return fcntl(fd, F_GETFD) != -1 || errno != EBADF;
}

void test_mod_rm_fd(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_deregister_fd(mod, STDIN_FILENO);
    assert_true(ret == 0);
    
    /* Fd is now closed (module_deregister_fd with SRC_FD_AUTOCLOSE thus is no more valid */
    assert_false(fd_is_valid(STDIN_FILENO));
}

void test_mod_subscribe_NULL_topic(void **state) {
    (void) state; /* unused */
    
    // module_register_source won't even let you use NULL as topic as it only expects "int" or "(const) char *"
    int ret = m_mod_register_sub(mod, NULL, 0, NULL);
    assert_false(ret == 0);
}

void test_mod_subscribe_NULL_self(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_register_src(NULL, "topic", 0, NULL);
    assert_false(ret == 0);
}

void test_mod_subscribe(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_register_src(mod, "topic", 0, NULL);
    assert_true(ret == 0);
}

void test_mod_ref_NULL_name(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_ref(mod, NULL, &testRef);
    assert_false(ret == 0);
}

void test_mod_ref_unexhistent_name(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_ref(mod, "testName2", &testRef);
    assert_false(ret == 0);
}

void test_mod_ref_NULL_ref(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_ref(mod, "testName2", NULL);
    assert_false(ret == 0);
}

void test_mod_ref(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_ref(mod, "testName", &testRef);
    assert_true(ret == 0);
    assert_non_null(testRef);
}

void test_mod_unref_NULL_ref(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_unref(mod, NULL);
    assert_false(ret == 0);
}

void test_mod_unref(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_unref(mod, &testRef);
    assert_true(ret == 0);
    assert_null(testRef);
}

void test_mod_tell_NULL_recipient(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_tell(mod, NULL, (unsigned char *)"hi!", 0);
    assert_false(ret == 0);
}

void test_mod_tell_NULL_self(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_tell(NULL, testRef, (unsigned char *)"hi!", 0);
    assert_false(ret == 0);
}

void test_mod_tell_NULL_msg(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_tell(mod, testRef, NULL, 0);
    assert_false(ret == 0);
}

/* Test automatic free of message too! */
void test_mod_tell(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_tell(mod, testRef, strdup("hi1!"), M_PS_AUTOFREE);
    assert_true(ret == 0);
}

void test_mod_publish_NULL_self(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_publish(NULL, "topic", (unsigned char *)"hi!", 0);
    assert_false(ret == 0);
}

void test_mod_publish_NULL_msg(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_publish(mod, "topic", NULL, 0);
    assert_false(ret == 0);
}

void test_mod_publish_NULL_topic(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_publish(mod, NULL, (unsigned char *)"hi!", 0);
    assert_false(ret == 0);
}

void test_mod_publish(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_publish(mod, "topic", (unsigned char *)"hi2!", 0);
    assert_true(ret == 0);
}

void test_mod_broadcast_NULL_self(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_broadcast(NULL, (unsigned char *)"hi!", 0);
    assert_false(ret == 0);
}

void test_mod_broadcast_NULL_msg(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_broadcast(mod, NULL, 0);
    assert_false(ret == 0);
}

void test_mod_broadcast(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_broadcast(mod, (unsigned char *)"hi3!", 0);
    assert_true(ret == 0);
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
    if (msg->type == M_SRC_TYPE_PS && msg->ps_msg->type == USER) {
        ctr++;
        if (!strcmp((char *)msg->ps_msg->data, "hi3!")) {
            m_ctx_quit(test_ctx, ctr);
        }
    }
}

static void destroy(void) {
    
}
