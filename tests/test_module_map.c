#include "test_module_map.h"
#include <module/module_map.h>

static map_t *my_map;
static int val = 5;

void test_module_map_put(void **state) {
    (void) state; /* unused */
    
    /* NULL map */
    module_map_code ret = module_map_put(my_map, "key", &val);
    assert_false(ret == MAP_OK);
    
    my_map = module_map_new();
    
    /* NULL value */
    ret = module_map_put(my_map, "key", NULL);
    assert_false(ret == MAP_OK);
    
    /* NULL key */
    ret = module_map_put(my_map, NULL, &val);
    assert_false(ret == MAP_OK);
    
    ret = module_map_put(my_map, "key", &val);
    assert_true(ret == MAP_OK);
}

void test_module_map_get(void **state) {
    (void) state; /* unused */
    int *value;
    
    /* NULL map */
    module_map_code ret = module_map_get(NULL, "key", (void **)&value);
    assert_false(ret == MAP_OK);
    
    /* NULL key */
    ret = module_map_get(my_map, NULL, (void **)&value);
    assert_false(ret == MAP_OK);
    
    /* Unhexistent key */
    ret = module_map_get(my_map, "keykey", (void **)&value);
    assert_false(ret == MAP_OK);
    
    ret = module_map_get(my_map, "key", (void **)&value);
    assert_true(ret == MAP_OK);
    assert_int_equal(*value, 5);
}

void test_module_map_length(void **state) {
    (void) state; /* unused */
    
    int len = module_map_length(NULL);
    assert_false(len > 0);
    
    len = module_map_length(my_map);
    assert_int_equal(len, 1);
}

void test_module_map_remove(void **state) {
    (void) state; /* unused */
    
    /* NULL map */
    module_map_code ret = module_map_remove(NULL, "key");
    assert_false(ret == MOD_OK);
    
    /* NULL key */
    ret = module_map_remove(my_map, NULL);
    assert_false(ret == MOD_OK);
    
    /* NULL key */
    ret = module_map_remove(my_map, "key");
    assert_true(ret == MOD_OK);
    
    int len = module_map_length(my_map);
    assert_int_equal(len, 0);
}

void test_module_map_free(void **state) {
    (void) state; /* unused */
    
    /* NULL map */
    module_map_code ret = module_map_free(NULL);
    assert_false(ret == MOD_OK);
    
    ret = module_map_free(my_map);
    assert_true(ret == MOD_OK);
}
