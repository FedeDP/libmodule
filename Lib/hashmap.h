#pragma once

enum map_errors {
    MAP_MISSING = -3,
    MAP_FULL,
    MAP_OMEM,
    MAP_OK
};

/*
 * any_t is a pointer.  This allows you to put arbitrary structures in
 * the hashmap.
 */
typedef void *any_t;

/*
 * PFany is a pointer to a function that can take two any_t arguments
 * and return an integer. Returns status code..
 */
typedef int (*PFany)(any_t, any_t);

/*
 * map_t is a pointer to an internally maintained data structure.
 * Clients of this package do not need to know how hashmaps are
 * represented.  They see and manipulate only map_t's.
 */
typedef any_t map_t;

map_t hashmap_new();
int hashmap_iterate(map_t in, PFany f, any_t item);
int hashmap_put(map_t in, const char *key, any_t value);
int hashmap_get(map_t in, const char *key, any_t *arg);
int hashmap_remove(map_t in, const char *key);
void hashmap_free(map_t in);
int hashmap_length(map_t in);
