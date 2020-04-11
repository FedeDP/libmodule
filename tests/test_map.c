#include "test_map.h"
#include <module/map.h>

static mod_map_t *my_map;
static int val = 5;
static int count;

void test_map_put(void **state) {
    (void) state; /* unused */
    
    /* NULL map */
    int ret = map_put(my_map, "key", &val);
    assert_false(ret == 0);
    
    my_map = map_new(false, NULL);
    
    /* NULL value */
    ret = map_put(my_map, "key", NULL);
    assert_false(ret == 0);
    
    /* NULL key */
    ret = map_put(my_map, NULL, &val);
    assert_false(ret == 0);
    
    ret = map_put(my_map, "key", &val);
    assert_true(ret == 0);
    assert_true(map_has_key(my_map, "key"));
    assert_int_equal(map_length(my_map), 1);
    
    /* Update val; check that map size was not increased */
    ret = map_put(my_map, "key", &val);
    assert_true(ret == 0);
    assert_int_equal(map_length(my_map), 1);
    
    ret = map_put(my_map, "key2", &val);
    assert_true(ret == 0);
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

void test_map_iterator(void **state) {
    (void) state; /* unused */
    
    /* NULL map */
    mod_map_itr_t *itr = map_itr_new(NULL);
    assert_null(itr);
    
    itr = map_itr_new(my_map);
    assert_non_null(itr);
    
    count = map_length(my_map);
    while (itr) {
        count--;
        printf("%s -> %p\n", map_itr_get_key(itr), map_itr_get_data(itr));
        itr = map_itr_next(itr);
    }
    assert_int_equal(count, 0);
    assert_null(itr);
}

int iterate_cb(void *userptr, const char *key, void *data) {
    count++;
    return 0;
}

void test_map_iterate(void **state) {
    (void) state; /* unused */
    
    /* NULL map */
    int ret = map_iterate(NULL, iterate_cb, NULL);
    assert_false(ret == 0);
    
    /* NULL cb */
    ret = map_iterate(my_map, NULL, NULL);
    assert_false(ret == 0);
    
    ret = map_iterate(my_map, iterate_cb, NULL);
    assert_true(ret == 0);
    assert_int_equal(count, map_length(my_map));
}

void test_map_remove(void **state) {
    (void) state; /* unused */
    
    /* NULL map */
    int ret = map_remove(NULL, "key");
    assert_false(ret == 0);
    
    /* NULL key */
    ret = map_remove(my_map, NULL);
    assert_false(ret == 0);
    
    /* NULL key */
    ret = map_remove(my_map, "key");
    assert_true(ret == 0);
    
    int len = map_length(my_map);
    assert_int_equal(len, 1); // "key2" still inside
}

void test_map_clear(void **state) {
    (void) state; /* unused */
    
    /* NULL map */
    int ret = map_clear(NULL);
    assert_false(ret == 0);
    
    ret = map_clear(my_map);
    assert_true(ret == 0);
    
    int len = map_length(my_map);
    assert_int_equal(len, 0);
}

void test_map_free(void **state) {
    (void) state; /* unused */
    
    /* NULL map */
    int ret = map_free(NULL);
    assert_false(ret == 0);
    
    ret = map_free(&my_map);
    assert_true(ret == 0);
    assert_null(my_map);
}

void test_map_stress(void **state) {
    (void) state; /* unused */
    
    int ret;
    my_map = map_new(true, NULL);
    const int size = 1000000;
    for (int i = 0; i < size; i++) {
        char key[20];
        snprintf(key, sizeof(key), "key%d", i);
        ret = map_put(my_map, key, "top");
        assert_true(ret == 0);
        assert_true(map_has_key(my_map, key));
    }
    assert_int_equal(map_length(my_map), size);
    
    count = 0;
    ret = map_iterate(my_map, iterate_cb, NULL);
    assert_true(ret == 0);
    assert_int_equal(count, map_length(my_map));
    
    ret = map_free(&my_map);
    assert_true(ret == 0);
    assert_null(my_map);
}
