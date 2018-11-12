#include "test_map.h"
#include <module/map.h>

static map_t *my_map;
static int val = 5;

void test_map_put(void **state) {
    (void) state; /* unused */
    
    /* NULL map */
    map_ret_code ret = map_put(my_map, "key", &val, false);
    assert_false(ret == MAP_OK);
    
    my_map = map_new();
    
    /* NULL value */
    ret = map_put(my_map, "key", NULL, false);
    assert_false(ret == MAP_OK);
    
    /* NULL key */
    ret = map_put(my_map, NULL, &val, false);
    assert_false(ret == MAP_OK);
    
    ret = map_put(my_map, "key", &val, false);
    assert_true(ret == MAP_OK);
}

void test_map_get(void **state) {
    (void) state; /* unused */
    int *value;
    
    /* NULL map */
    map_ret_code ret = map_get(NULL, "key", (void **)&value);
    assert_false(ret == MAP_OK);
    
    /* NULL key */
    ret = map_get(my_map, NULL, (void **)&value);
    assert_false(ret == MAP_OK);
    
    /* Unhexistent key */
    ret = map_get(my_map, "keykey", (void **)&value);
    assert_false(ret == MAP_OK);
    
    ret = map_get(my_map, "key", (void **)&value);
    assert_true(ret == MAP_OK);
    assert_int_equal(*value, 5);
}

void test_map_length(void **state) {
    (void) state; /* unused */
    
    int len = map_length(NULL);
    assert_false(len > 0);
    
    len = map_length(my_map);
    assert_int_equal(len, 1);
}

void test_map_remove(void **state) {
    (void) state; /* unused */
    
    /* NULL map */
    map_ret_code ret = map_remove(NULL, "key");
    assert_false(ret == MOD_OK);
    
    /* NULL key */
    ret = map_remove(my_map, NULL);
    assert_false(ret == MOD_OK);
    
    /* NULL key */
    ret = map_remove(my_map, "key");
    assert_true(ret == MOD_OK);
    
    int len = map_length(my_map);
    assert_int_equal(len, 0);
}

void test_map_free(void **state) {
    (void) state; /* unused */
    
    /* NULL map */
    map_ret_code ret = map_free(NULL);
    assert_false(ret == MOD_OK);
    
    ret = map_free(my_map);
    assert_true(ret == MOD_OK);
}
