#include "test_ctx.h"
#include <module/ctx.h>
#include <module/mod.h>
#include <stdlib.h>
#include <string.h>

#define CTX "testCtx"

m_ctx_t *test_ctx = NULL;

static void logger(const m_mod_t *self, const char *fmt, va_list args) {
    const char *name = NULL;
    const char *context = NULL;
    if (self) {
        name = m_mod_name(self);
        context = m_ctx_name(m_mod_ctx(self));
    }
    printf("%s@%s:\t* ", name ? name : "null", context ? context : "null");
    vprintf(fmt, args);
}

void test_ctx_register_NULL_name(void **state) {
    (void) state; /* unused */

    int ret = m_ctx_register(NULL, &test_ctx, 0, 0, NULL);
    assert_false(ret == 0);
}

void test_ctx_register(void **state) {
    (void) state; /* unused */

    int ret = m_ctx_register(CTX, &test_ctx, M_CTX_PERSIST, 0, NULL);
    assert_true(ret == 0);
}

void test_ctx_deregister_NULL_name(void **state) {
    (void) state; /* unused */

    int ret = m_ctx_deregister(NULL);
    assert_false(ret == 0);
}

void test_ctx_deregister(void **state) {
    (void) state; /* unused */

    int ret = m_ctx_deregister(&test_ctx);
    assert_true(ret == 0);
    assert_null(test_ctx);
}

void test_ctx_set_logger_NULL_logger(void **state) {
    (void) state; /* unused */
    
    int ret = m_ctx_set_logger(test_ctx, NULL);
    assert_false(ret == 0);
}

void test_ctx_set_logger(void **state) {
    (void) state; /* unused */
    
    int ret = m_ctx_set_logger(test_ctx, logger);
    assert_true(ret == 0);
}

void test_ctx_dump(void **state) {
    (void) state; /* unused */
    
    int ret = m_ctx_dump(test_ctx);
    assert_true(ret == 0);
}

void test_ctx_quit_no_loop(void **state) {
    (void) state; /* unused */
    
    int ret = m_ctx_quit(test_ctx, 0);
    assert_false(ret == 0);
}

void test_ctx_loop(void **state) {
    (void) state; /* unused */

    int ret = m_ctx_loop(test_ctx);
    assert_true(ret == 0);
}

static void recv_noop(m_mod_t *mod, const m_queue_t *const evts) {
    
}

void test_ctx_dispatch(void **state) {
    (void) state; /* unused */
    
    int ret = m_mod_src_register(test_mod, M_PS_CTX_STARTED, M_SRC_ONESHOT, NULL);
    assert_true(ret == 0);
    
    ret = m_mod_become(test_mod, recv_noop);
    assert_true(ret == 0);
    
    ret = m_ctx_dispatch(test_ctx);
    assert_true(ret == 0);  // loop started

    ret = m_ctx_dispatch(test_ctx);
    assert_true(ret == 1); // number of messages dispatched: M_PS_CTX_STARTED.

    ret = m_ctx_quit(test_ctx, 150);
    assert_true(ret == 0);

    ret = m_ctx_dispatch(test_ctx);
    assert_true(ret == 150); // loop stopped with exit code 0
    
    ret = m_mod_unbecome(test_mod);
    assert_true(ret == 0);
}

static void my_recv(m_mod_t *mod, const m_queue_t *const evts) {
    m_itr_foreach(evts, {
        m_evt_t *msg = m_itr_get(m_itr);
        if (msg->type == M_SRC_TYPE_PS && msg->ps_evt->topic && strcmp(msg->ps_evt->topic, M_PS_CTX_STARTED) == 0) {
            m_mod_deregister(&mod);
        }
    });
}

void test_ctx_mod_deregister_during_loop(void **state) {
    (void) state; /* unused */

    test_ctx = NULL;

    int ret = m_ctx_register("test", &test_ctx, 0, 0, NULL);
    assert_true(ret == 0);
    assert_non_null(test_ctx);

    m_mod_hook_t hook = { .on_evt = my_recv };
    m_mod_t *mod = NULL;
    ret = m_mod_register("testName", test_ctx, &mod, &hook, 0, NULL);
    assert_true(ret == 0);
    assert_non_null(mod);
    assert_true(m_mod_is(mod, M_MOD_IDLE));
    
    /* Register the module to the desired system message */
    m_mod_src_register(mod, M_PS_CTX_STARTED, 0, NULL);

    ret = m_ctx_loop(test_ctx);
    assert_int_equal(ret, 0);
}
