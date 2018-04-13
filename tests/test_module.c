#include "test_module.h"
#include <module.h>

static void init(void);
static int evaluate(void);
static void recv(const msg_t *msg, const void *userdata);
static void destroy(void);

const self_t *self = NULL;

void test_module_register_NULL_name(void **state) {
    (void) state; /* unused */

    userhook hook = (userhook) { init, evaluate, recv, destroy };
    module_ret_code ret = module_register(NULL, ctx, &self, &hook);
    assert_false(ret == MOD_OK);
    assert_null(self);
}

void test_module_register_NULL_ctx(void **state) {
    (void) state; /* unused */
    
    userhook hook = (userhook) { init, evaluate, recv, destroy };
    module_ret_code ret = module_register("testName", NULL, &self, &hook);
    assert_false(ret == MOD_OK);
    assert_null(self);
}

void test_module_register_NULL_self(void **state) {
    (void) state; /* unused */
    
    userhook hook = (userhook) { init, evaluate, recv, destroy };
    module_ret_code ret = module_register("testName", ctx, NULL, &hook);
    assert_false(ret == MOD_OK);
    assert_null(self);
}

void test_module_register_NULL_hook(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = module_register("testName", ctx, &self, NULL);    
    assert_false(ret == MOD_OK);
    assert_null(self);
}

void test_module_register(void **state) {
    (void) state; /* unused */
    
    userhook hook = (userhook) { init, evaluate, recv, destroy };
    module_ret_code ret = module_register("testName", ctx, &self, &hook);
    assert_true(ret == MOD_OK);
    assert_non_null(self);
    assert_true(module_is(self, RUNNING));
}

void test_module_register_already_registered(void **state) {
    (void) state; /* unused */
    
    userhook hook = (userhook) { init, evaluate, recv, destroy };
    module_ret_code ret = module_register("testName", ctx, &self, &hook);
    assert_false(ret == MOD_OK);
    assert_non_null(self);
    assert_true(module_is(self, RUNNING));
}

void test_module_register_same_name(void **state) {
    (void) state; /* unused */
    
    const self_t *self2 = NULL;
    
    userhook hook = (userhook) { init, evaluate, recv, destroy };
    module_ret_code ret = module_register("testName", ctx, &self2, &hook);
    assert_false(ret == MOD_OK);
    assert_null(self2);
}

void test_module_deregister_NULL_self(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = module_deregister(NULL);
    assert_false(ret == MOD_OK);
    assert_non_null(self);
}

void test_module_deregister(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = module_deregister(&self);
    assert_true(ret == MOD_OK);
    assert_null(self);
}

void test_module_pause_NULL_self(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = module_pause(NULL);
    assert_false(ret == MOD_OK);
    assert_true(module_is(self, RUNNING));
}

void test_module_pause(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = module_pause(self);
    assert_true(ret == MOD_OK);
    assert_true(module_is(self, PAUSED));
}

void test_module_resume_NULL_self(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = module_resume(NULL);
    assert_false(ret == MOD_OK);
    assert_true(module_is(self, PAUSED));
}

void test_module_resume(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = module_resume(self);
    assert_true(ret == MOD_OK);
    assert_true(module_is(self, RUNNING));
}

void test_module_stop_NULL_self(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = module_stop(NULL);
    assert_false(ret == MOD_OK);
    assert_false(module_is(self, STOPPED));
}

void test_module_stop(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = module_stop(self);
    assert_true(ret == MOD_OK);
    assert_true(module_is(self, STOPPED));
}

void test_module_start_NULL_self(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = module_start(NULL);
    assert_false(ret == MOD_OK);
    assert_false(module_is(self, RUNNING));
}

void test_module_start(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = module_start(self);
    assert_true(ret == MOD_OK);
    assert_true(module_is(self, RUNNING));
}

void test_module_log_NULL_self(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = module_log(NULL, "Hi\n");
    assert_false(ret == MOD_OK);
}

void test_module_log(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = module_log(self, "Hi\n");
    assert_true(ret == MOD_OK);
}

void test_module_set_userdata_NULL_self(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = module_set_userdata(NULL, NULL);
    assert_false(ret == MOD_OK);
}

void test_module_set_userdata(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = module_set_userdata(self, NULL);
    assert_true(ret == MOD_OK);
}

void test_module_become_NULL_self(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = module_become(NULL, NULL);
    assert_false(ret == MOD_OK);
}

void test_module_become_NULL_func(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = module_become(self, NULL);
    assert_false(ret == MOD_OK);
}

void test_module_become(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = module_become(self, recv);
    assert_true(ret == MOD_OK);
}

static void init(void) {
    
}

static int evaluate(void) {
    return 1;
}

static void recv(const msg_t *msg, const void *userdata) {
    
}

static void destroy(void) {
    
}
