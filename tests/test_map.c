#include "test_map.h"
#include <module/map.h>

static map_t *my_map;
static int val = 5;

void test_map_put(void **state) {
    (void) state; /* unused */
    
    /* NULL map */
    map_ret_code ret = map_put(my_map, "key", &val, false, false);
    assert_false(ret == MAP_OK);
    
    my_map = map_new();
    
    /* NULL value */
    ret = map_put(my_map, "key", NULL, false, false);
    assert_false(ret == MAP_OK);
    
    /* NULL key */
    ret = map_put(my_map, NULL, &val, false, false);
    assert_false(ret == MAP_OK);
    
    ret = map_put(my_map, "key", &val, false, false);
    assert_true(ret == MAP_OK);
    assert_true(map_has_key(my_map, "key"));
    
    ret = map_put(my_map, "key2", &val, false, false);
    assert_true(ret == MAP_OK);
    assert_true(map_has_key(my_map, "key2"));
}

void test_map_get(void **state) {
    (void) state; /* unused */
    
    /* NULL map */
    int *value = map_get(NULL, "key");
    assert_null(value);
    
    /* NULL key */
    value = map_get(my_map, NULL);
    assert_null(value);
    
    /* Unhexistent key */
    value = map_get(my_map, "keykey");
    assert_null(value);
    
    value = map_get(my_map, "key");
    assert_non_null(value);
    assert_int_equal(*value, 5);
}

void test_map_length(void **state) {
    (void) state; /* unused */
    
    int len = map_length(NULL);
    assert_false(len >= 0);
    
    len = map_length(my_map);
    assert_int_equal(len, 2);
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
    assert_int_equal(len, 1); // "key2" still inside
}

void test_map_clear(void **state) {
    (void) state; /* unused */
    
    /* NULL map */
    map_ret_code ret = map_clear(NULL);
    assert_false(ret == MOD_OK);
    
    ret = map_clear(my_map);
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
