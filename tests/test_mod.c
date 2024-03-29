#define _POSIX_C_SOURCE 200809L

#include "test_mod.h"
#include <module/mod.h>
#include <module/ctx.h>
#include <module/mem/mem.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>

static bool init_false(m_mod_t *mod);
static void mod_recv(m_mod_t *mod, const m_queue_t *const evts);
static void mod_recv_ready(m_mod_t *mod, const m_queue_t *const evts);

static int ctr;

m_mod_t *test_mod = NULL;
static m_mod_t *testRef = NULL;
static int fd;

void test_mod_register_NULL_name(void **state) {
    (void) state; /* unused */

    m_mod_hook_t hook = { .on_evt = mod_recv };
    int ret = m_mod_register(NULL, &test_mod, &hook, 0, NULL);
    assert_false(ret == 0);
    assert_null(test_mod);
}

void test_mod_register(void **state) {
    (void) state; /* unused */
    
    m_mod_hook_t hook = { .on_evt = mod_recv };
    int ret = m_mod_register("testName", &test_mod, &hook, 0, NULL);
    assert_true(ret == 0);
    assert_non_null(test_mod);
    assert_true(m_mod_is(test_mod, M_MOD_IDLE));
}

void test_mod_register_already_registered(void **state) {
    (void) state; /* unused */
    
    m_mod_hook_t hook = { .on_evt = mod_recv };
    int ret = m_mod_register("testName", &test_mod, &hook, 0, NULL);
    assert_false(ret == 0);
    assert_non_null(test_mod);
    assert_true(m_mod_is(test_mod, M_MOD_IDLE));
}

void test_mod_register_same_name(void **state) {
    (void) state; /* unused */
    
    m_mod_t *self2 = NULL;
    
    m_mod_hook_t hook = { .on_evt = mod_recv };
    int ret = m_mod_register("testName", &self2, &hook, 0, NULL);
    assert_false(ret == 0);
    assert_null(self2);
}

void test_mod_deregister_NULL_self(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_deregister(NULL);
    assert_false(ret == 0);
    assert_non_null(test_mod);
}

void test_mod_deregister(void **state) {
    (void) state; /* unused */
    
    /* Test memory referencing */
    m_mod_t *mod_ref = m_mem_ref(test_mod);
    int ret = m_mod_deregister(&test_mod);
    assert_true(ret == 0);
    assert_null(test_mod);
    assert_non_null(mod_ref),
    assert_true(m_mod_is(mod_ref, M_MOD_ZOMBIE));
    
    m_mem_unrefp((void **)&mod_ref);
    assert_null(mod_ref);
}

void test_mod_false_init(void **state) {
    (void) state; /* unused */
    
    m_mod_hook_t hook = { .on_start = init_false, .on_evt = mod_recv };
    int ret = m_mod_register("testName", &test_mod, &hook, 0, NULL);
    assert_true(ret == 0);
    assert_non_null(test_mod);
    assert_true(m_mod_is(test_mod, M_MOD_IDLE));
    
    ret = m_mod_start(test_mod);
    assert_true(ret == 0);
    assert_true(m_mod_is(test_mod, M_MOD_STOPPED));
    
    ret = m_mod_deregister(&test_mod);
    assert_true(ret == 0);
    assert_null(test_mod);
}

void test_mod_pause_NULL_self(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_pause(NULL);
    assert_false(ret == 0);
    assert_true(m_mod_is(test_mod, M_MOD_RUNNING));
}

void test_mod_pause(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_pause(test_mod);
    assert_true(ret == 0);
    assert_true(m_mod_is(test_mod, M_MOD_PAUSED));
    
    /* Test module_tell for paused modules */
    ret = m_mod_ps_tell(test_mod, test_mod, (unsigned char *)"Paused!", 0);
    assert_true(ret == 0);
}

void test_mod_resume_NULL_self(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_resume(NULL);
    assert_false(ret == 0);
    assert_true(m_mod_is(test_mod, M_MOD_PAUSED));
}

void test_mod_resume(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_resume(test_mod);
    assert_true(ret == 0);
    assert_true(m_mod_is(test_mod, M_MOD_RUNNING));
}

void test_mod_stop_NULL_self(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_stop(NULL);
    assert_false(ret == 0);
    assert_false(m_mod_is(test_mod, M_MOD_STOPPED));
}

void test_mod_stop(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_stop(test_mod);
    assert_true(ret == 0);
    assert_true(m_mod_is(test_mod, M_MOD_STOPPED));
}

void test_mod_start_NULL_self(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_start(NULL);
    assert_false(ret == 0);
    assert_false(m_mod_is(test_mod, M_MOD_RUNNING));
}

void test_mod_start(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_start(test_mod);
    assert_true(ret == 0);
    assert_true(m_mod_is(test_mod, M_MOD_RUNNING));
}

void test_mod_log_NULL_self(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_log(NULL, "Hi\n");
    assert_false(ret == 0);
}

void test_mod_log(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_log(test_mod, "Hi\n");
    assert_true(ret == 0);
}

void test_mod_dump_NULL_self(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_dump(NULL);
    assert_false(ret == 0);
}

void test_mod_dump(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_dump(test_mod);
    assert_true(ret == 0);
}

void test_mod_become_NULL_self(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_become(NULL, NULL);
    assert_false(ret == 0);
}

void test_mod_become_NULL_func(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_become(test_mod, NULL);
    assert_false(ret == 0);
}

void test_mod_become(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_become(test_mod, mod_recv);
    assert_true(ret == 0);
}

void test_mod_unbecome_NULL_self(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_unbecome(NULL);
    assert_false(ret == 0);
}

void test_mod_unbecome(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_unbecome(test_mod);
    assert_true(ret == 0);
    
    /* We don't have any more recv in stack */
    ret = m_mod_unbecome(test_mod);
    assert_false(ret == 0);
}

void test_mod_add_wrong_fd(void **state) {
    (void) state; /* unused */
    
    // Initialize fd
    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    int ret = m_mod_src_register_fd(test_mod, -1, M_SRC_FD_AUTOCLOSE, NULL);
    assert_false(ret == 0);
}

void test_mod_add_fd_NULL_self(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_src_register_fd(NULL, fd, M_SRC_FD_AUTOCLOSE, NULL);
    assert_false(ret == 0);
}

void test_mod_add_fd(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_src_register_fd(test_mod, fd, M_SRC_FD_AUTOCLOSE, NULL);
    assert_true(ret == 0);
    
    /* Try to register again */
    ret = m_mod_src_register_fd(test_mod, fd, M_SRC_FD_AUTOCLOSE, NULL);
    assert_true(ret == -EEXIST);
}

void test_mod_rm_wrong_fd(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_src_deregister_fd(test_mod, -1);
    assert_false(ret == 0);
}

void test_mod_rm_wrong_fd_2(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_src_deregister_fd(test_mod, fd + 1);
    assert_false(ret == 0);
}

void test_mod_rm_fd_NULL_self(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_src_deregister_fd(NULL, fd);
    assert_false(ret == 0);
}

static int fd_is_valid(int f) {
    return fcntl(f, F_GETFD) != -1 || errno != EBADF;
}

void test_mod_rm_fd(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_src_deregister_fd(test_mod, fd);
    assert_true(ret == 0);
    
    /* Fd is now closed (module_deregister_fd with SRC_FD_AUTOCLOSE thus is no more valid */
    assert_false(fd_is_valid(fd));
}

void test_mod_subscribe_NULL_topic(void **state) {
    (void) state; /* unused */
    
    // module_register_source won't even let you use NULL as topic as it only expects "int" or "(const) char *"
    int ret = m_mod_ps_subscribe(test_mod, NULL, 0, NULL);
    assert_false(ret == 0);
}

void test_mod_subscribe_NULL_self(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_src_register(NULL, "topic", 0, NULL);
    assert_false(ret == 0);
}

void test_mod_subscribe(void **state) {
    (void) state; /* unused */

    int ret = m_mod_src_register(test_mod, "topic", 0, NULL);
    assert_true(ret == 0);
}

void test_mod_ref_NULL_name(void **state) {
    (void) state; /* unused */
    
    testRef = m_mod_lookup(test_mod, NULL);
    assert_null(testRef);
}

void test_mod_ref_unexhistent_name(void **state) {
    (void) state; /* unused */
    
    testRef = m_mod_lookup(test_mod, "testName2");
    assert_null(testRef);
}

void test_mod_ref(void **state) {
    (void) state; /* unused */
    
    testRef = m_mod_lookup(test_mod, "testName");
    assert_non_null(testRef);
}

void test_mod_unref_NULL_ref(void **state) {
    (void) state; /* unused */
    
    void *data = m_mem_unref(NULL);
    assert_null(data);
}

void test_mod_unref(void **state) {
    (void) state; /* unused */
    
    testRef = m_mem_ref(testRef);
    testRef = m_mem_unref(testRef);
    assert_null(testRef);
}

void test_mod_tell_NULL_recipient(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_ps_tell(test_mod, NULL, (unsigned char *)"hi!", 0);
    assert_false(ret == 0);
}

void test_mod_tell_NULL_self(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_ps_tell(NULL, testRef, (unsigned char *)"hi!", 0);
    assert_false(ret == 0);
}

void test_mod_tell_NULL_msg(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_ps_tell(test_mod, testRef, NULL, 0);
    assert_false(ret == 0);
}

/* Test automatic free of message too! */
void test_mod_tell(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_ps_tell(test_mod, testRef, strdup("hi1!"), M_PS_AUTOFREE);
    assert_true(ret == 0);
}

void test_mod_publish_NULL_self(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_ps_publish(NULL, "topic", (unsigned char *)"hi!", 0);
    assert_false(ret == 0);
}

void test_mod_publish_NULL_msg(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_ps_publish(test_mod, "topic", NULL, 0);
    assert_false(ret == 0);
}

void test_mod_publish(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_ps_publish(test_mod, "topic", (unsigned char *)"hi2!", 0);
    assert_true(ret == 0);
}

void test_mod_broadcast_NULL_self(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_ps_publish(NULL, NULL, (unsigned char *)"hi!", 0);
    assert_false(ret == 0);
}

void test_mod_broadcast_NULL_msg(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_ps_publish(test_mod, NULL, NULL, 0);
    assert_false(ret == 0);
}

void test_mod_broadcast(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_ps_publish(test_mod, NULL, (unsigned char *)"hi3!", 0);
    assert_true(ret == 0);
}

static bool init_false(m_mod_t *mod) {
    return false;
}

/*
 * Stupidly simple test for stashing api:
 * stash any message until "hi3!" is received.
 * Then, set new behavior and unstash everything.
 * Check that all stashed messages are received.
 */
static void mod_recv(m_mod_t *mod, const m_queue_t *const evts) {
    m_itr_foreach(evts, {
        m_evt_t *msg = m_itr_get(m_itr);
        if (msg->type == M_SRC_TYPE_PS) {
            ctr++;
            int ret = m_mod_stash(mod, msg);
            assert_int_equal(ret, 0);
            if (!strcmp((char *) msg->ps_evt->data, "hi3!")) {
                ret = m_mod_become(mod, mod_recv_ready);
                assert_int_equal(ret, 0);

                ret = m_mod_unstash(mod, -1);
                assert_int_equal(ret, 3); // 3 messages unstashed
            }
        }
    });
}

static void mod_recv_ready(m_mod_t *mod, const m_queue_t *const evts) {
    m_itr_foreach(evts, {
        m_evt_t *msg = m_itr_get(m_itr);
        if (msg->type == M_SRC_TYPE_PS) {
            ctr--;
            if (!strcmp((char *) msg->ps_evt->data, "hi3!")) {
                m_ctx_quit(ctr);
            }
        }
    });
}
