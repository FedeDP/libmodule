#include "test_module.h"

static void init(void);
static int evaluate(void);
static void recv(const msg_t *msg, const void *userdata);
static void destroy(void);

const self_t *self = NULL;

void test_module_register_NULL_name(void **state) {
    (void) state; /* unused */

    userhook hook = (userhook) { init, evaluate, recv, destroy };
    module_ret_code ret = module_register(NULL, "testCtx", &self, &hook);
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
    module_ret_code ret = module_register("testName", "testCtx", NULL, &hook);
    assert_false(ret == MOD_OK);
}

void test_module_register_NULL_hook(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = module_register("testName", "testCtx", &self, NULL);    
    assert_false(ret == MOD_OK);
    assert_null(self);
}

void test_module_register(void **state) {
    (void) state; /* unused */
    
    userhook hook = (userhook) { init, evaluate, recv, destroy };
    module_ret_code ret = module_register("testName", "testCtx", &self, &hook);
    assert_true(ret == MOD_OK);
    assert_non_null(self);
    assert_false(module_is(self, IDLE));
    assert_true(module_is(self, RUNNING));
    assert_false(module_is(self, PAUSED));
    assert_false(module_is(self, STOPPED));
}

void test_module_deregister_NULL_self(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = module_deregister(NULL);
    assert_false(ret == MOD_OK);
    assert_non_null(self);
    assert_false(module_is(self, IDLE));
    assert_true(module_is(self, RUNNING));
    assert_false(module_is(self, PAUSED));
    assert_false(module_is(self, STOPPED));
}

void test_module_deregister(void **state) {
    (void) state; /* unused */
    
    module_ret_code ret = module_deregister(&self);
    assert_true(ret == MOD_OK);
    assert_null(self);
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
