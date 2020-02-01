/*
 * hashmap implementation provided by David Leeds (thank you!) with some changes.
 * https://github.com/DavidLeeds/hashmap
 */

#include "poll_priv.h"

#define MAP_PARAM_ASSERT(cond)      MOD_RET_ASSERT(cond, MAP_WRONG_PARAM);

#define MAP_SIZE_DEFAULT            (1 << 8)    /* 256 */
#define MAP_SIZE_MOD(map, val)      ((val) & ((map)->table_size - 1))

/* Limit for probing is 1/2 of table_size */
#define MAP_PROBE_LEN(map)          ((map)->table_size >> 1)
/* Return the next linear probe index */
#define MAP_PROBE_NEXT(map, index)  MAP_SIZE_MOD(map, (index) + 1)

/* Check if index b is less than or equal to index a */
#define MAP_INDEX_LE(map, a, b)     \
    ((a) == (b) || (((b) - (a)) & ((map)->table_size >> 1)) != 0)

#define MAP_FOREACH(table, size, fn)  \
    for (map_elem *entry = table; entry < &table[size]; ++entry) { fn }

typedef struct {
    const char *key;
    void *data;
} map_elem;

/* 
 * A hashmap has some maximum size and current size,
 * as well as the data to hold. 
 */
struct _map {
    size_t table_size;
    size_t length;
    bool dupkeys;
    map_elem *table;
    map_dtor dtor;
};

struct _map_itr {
    map_t *m;
    map_elem *curr;
    bool removed;
};

static map_elem *hashmap_entry_find(const map_t *m, const char *key, bool find_empty);
static size_t hashmap_table_min_size_calc(size_t num_entries);
static size_t hashmap_calc_index(const map_t *m, const char *key);
static size_t hashmap_hash_string(const char *key);
static map_ret_code hashmap_rehash(map_t *m);
static map_ret_code hashmap_put(map_t *m, const char *key, void *value);
static void clear_elem(map_t *m, map_elem *entry);

/*
 * Find the hashmap entry with the specified key, or an empty slot.
 * Returns NULL if the entire table has been searched without finding a match.
 */
static map_elem *hashmap_entry_find(const map_t *m, const char *key, bool find_empty) {
    const size_t probe_len = MAP_PROBE_LEN(m);    
    size_t index = hashmap_calc_index(m, key);
    
    /* Linear probing */
    for (size_t i = 0; i < probe_len; i++) {
        map_elem *entry = &m->table[index];
        if (!entry->key) {
            if (find_empty) {
                return entry;
            }
            return NULL;
        }
        if (strcmp(key, entry->key) == 0) {
            return entry;
        }
        index = MAP_PROBE_NEXT(m, index);
    }
    return NULL;
}

/*
 * Enforce a maximum 0.75 load factor.
 */
static inline size_t hashmap_table_min_size_calc(size_t num_entries) {
    return num_entries + (num_entries / 3);
}

/*
 * Get a valid hash table index from a key.
 */
static inline size_t hashmap_calc_index(const map_t *m, const char *key) {
    return MAP_SIZE_MOD(m, hashmap_hash_string(key));
}

/*
 * Hash function for string keys.
 * This is an implementation of the well-documented Jenkins 
 * one-at-a-time hash function.
 */
static size_t hashmap_hash_string(const char *key) {
    size_t hash = 0;
    
    for (; *key; ++key) {
        hash += *key;
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

/*
 * Doubles the size of the hashmap, and rehashes all the elements
 */
static map_ret_code hashmap_rehash(map_t *m) {
    map_elem *new_table = memhook._calloc(m->table_size << 1, sizeof(map_elem));
    MOD_RET_ASSERT(new_table, MAP_OMEM);
    
    /* Backup old elements in case of rehash failure */
    size_t old_size = m->table_size;
    map_elem *old_table = m->table;

    m->table_size = m->table_size << 1;
    m->table = new_table;
    
    /* Rehash */
    MAP_FOREACH(old_table, old_size, {
        if (entry->key) {
            map_elem *new_entry = hashmap_entry_find(m, entry->key, true);
            if (!new_entry) {
                /*
                * The load factor is too high with the new table
                * size, or a poor hash function was used.
                */
                goto revert;
            }
            memcpy(new_entry, entry, sizeof(map_elem));
        }
    });
    memhook._free(old_table);
    return MAP_OK;
    
revert:
    m->table_size = old_size;
    m->table = old_table;
    memhook._free(new_table);
    return MAP_ERR;
}

static map_ret_code hashmap_put(map_t *m, const char *key, void *value) {
    MOD_RET_ASSERT(key, MAP_OMEM);
    
    map_ret_code ret;
    
    /* Rehash with 2x capacity if load factor is approaching 0.75 */
    if (m->table_size <= hashmap_table_min_size_calc(m->length)) {
        ret = hashmap_rehash(m);
        if (ret != MAP_OK) {
            return ret;
        }
    }
    
    map_elem *entry = hashmap_entry_find(m, key, true);
    if (!entry) {
        /*
         * Cannot find an empty slot.  Either out of memory, or using
         * a poor hash function.  Attempt to rehash once to reduce
         * chain length.
         */
        ret = hashmap_rehash(m);
        if (ret != MAP_OK) {
            return ret;
        }
        entry = hashmap_entry_find(m, key, true);
        if (!entry) {
            return MAP_FULL;
        }
    }
    if (!entry->key) {
        entry->key = key;
        m->length++;
    } else if (m->dtor && entry->data != value) {
        /* Destroy old value if needed */
        m->dtor(entry->data);
    }
    /* Update value in any case */
    entry->data = value;
    return MAP_OK;
}

/*
 * Removes the specified entry and processes the preceeding entries to reduce
 * the load factor and keep the chain continuous. 
 * This is a required step for hashmaps using linear probing.
 */
static void clear_elem(map_t *m, map_elem *removed_entry) {
    /* Blank out the fields */
    if (m->dupkeys) {
        memhook._free((void *)removed_entry->key);
    }
    removed_entry->key = NULL;
    
    if (m->dtor) {
        m->dtor(removed_entry->data);
    }
    removed_entry->data = NULL;
    
    /* Reduce the size */
    m->length--;
    
    size_t removed_index = (removed_entry - m->table);
    const size_t probe_len = MAP_PROBE_LEN(m);    
    size_t index = MAP_PROBE_NEXT(m, removed_index);
    for (size_t i = 0; i < probe_len; i++) {
        map_elem *entry = &m->table[index];
        if (!entry->key) {
            /* Reached end of chain */
            break;
        }
        const size_t entry_index = hashmap_calc_index(m, entry->key);
        /* Shift in entries with an index <= to the removed slot */
        if (MAP_INDEX_LE(m, removed_index, entry_index)) {
            memcpy(removed_entry, entry, sizeof(map_elem));
            removed_index = index;
            removed_entry = entry;
        }
        index = MAP_PROBE_NEXT(m, index);
    }
    /* Clear the last removed entry */
    memset(removed_entry, 0, sizeof(map_elem));
}

/** Public API **/

/*
 * Return an empty hashmap, or NULL on failure.
 */
map_t *map_new(const bool keysdup, const map_dtor fn) {
    map_t *m = memhook._calloc(1, sizeof(map_t));
    if (m) {
        m->table = memhook._calloc(MAP_SIZE_DEFAULT, sizeof(map_elem));
        if (m->table) {
            m->table_size = MAP_SIZE_DEFAULT;
            m->dtor = fn;
            m->dupkeys = keysdup;
        } else {
            map_free(m);
            m = NULL;
        }
    }
    return m;
}

map_itr_t *map_itr_new(const map_t *m) {
    MOD_RET_ASSERT(map_length(m) > 0, NULL);
    
    map_itr_t *itr = memhook._calloc(1, sizeof(map_itr_t));
    if (itr) {
        itr->m = (map_t *)m;
        itr = map_itr_next(itr);
    }
    return itr;
}

map_itr_t *map_itr_next(map_itr_t *itr) {
    MOD_RET_ASSERT(itr, NULL);
    
    if (!itr->curr) {
        /* First time: start from first elem */
        itr->curr = &itr->m->table[0];
    } else {
        /* Normally: start from subsequent element */
        itr->curr = itr->curr + 1 - itr->removed;
    }
    
    itr->removed = false;
    bool found = false;
    for (; itr->curr < &itr->m->table[itr->m->table_size]; itr->curr++) {
        if (itr->curr->key) {
            found = true;
            break;
        }
    }
    
    /* Automatically free it */
    if (!found) {
        memhook._free(itr);
        itr = NULL;
    }
    return itr;
}

map_ret_code map_itr_remove(map_itr_t *itr) {
    MAP_PARAM_ASSERT(itr);
    
    if (!itr->removed) {
        clear_elem(itr->m, itr->curr);
        itr->removed = true;
        return MAP_OK;
    }
    return MAP_EPERM;
}

const char *map_itr_get_key(const map_itr_t *itr) {
    MOD_RET_ASSERT(itr, NULL);
    
    if (!itr->removed) {
        return itr->curr->key;
    }
    return NULL;
}

void *map_itr_get_data(const map_itr_t *itr) {
    MOD_RET_ASSERT(itr, NULL);
    
    if (!itr->removed) {
        return itr->curr->data;
    }
    return NULL;
}

map_ret_code map_itr_set_data(const map_itr_t *itr, void *value) {
    MAP_PARAM_ASSERT(itr);
    MAP_PARAM_ASSERT(value);
    
    itr->curr->data = value;
    return MAP_OK;
}

/*
 * Add a pointer to the hashmap with strdupped key if dupkey is true
 */
map_ret_code map_put(map_t *m, const char *key, void *value) {
    MAP_PARAM_ASSERT(m);
    MAP_PARAM_ASSERT(key);
    MAP_PARAM_ASSERT(value);
    
    /* Find a place to put our value */
    return hashmap_put(m, m->dupkeys ? mem_strdup(key) : key, value);
}

/*
 * Get your pointer out of the hashmap with a key
 */
void *map_get(const map_t *m, const char *key) {
    MOD_RET_ASSERT(key, NULL);
    MOD_RET_ASSERT(map_length(m) > 0, NULL);
    
    /* Find data location */
    map_elem *entry = hashmap_entry_find(m, key, false);
    if (!entry) {
        return NULL;
    }
    return entry->data;
}

bool map_has_key(const map_t *m, const char *key) {
    return map_get(m, key) != NULL;
}

/*
 * Invoke fn for each entry in the hashmap with userptr as first argument.
 * This function supports calls to hashmap_remove() during iteration.
 * However, it is an error to put or remove an entry other than the current one,
 * and doing so will immediately halt iteration and return an error.
 * Iteration is stopped if func returns non-zero.  
 * Returns func's return value if it is < 0, otherwise, 0.
 */
map_ret_code map_iterate(map_t *m, const map_cb fn, void *userptr) {
    MAP_PARAM_ASSERT(fn);
    MOD_RET_ASSERT(map_length(m) > 0, MAP_MISSING);
    
    MAP_FOREACH(m->table, m->table_size, {
        if (!entry->key) {
            continue;
        }
        const size_t num_entries = m->length;
        const char *key = entry->key;
        map_ret_code rc = fn(userptr, entry->key, entry->data);
        if (rc < MAP_OK) {
            /* Stop right now with error */
            return rc;
        }
        if (rc > MAP_OK) {
            /* Stop right now with MAP_OK */
            return MAP_OK;
        }
        if (entry->key != key) {
            /* Run this entry again if fn() deleted it */
            --entry;
        } else if (num_entries != m->length) {
            /* Stop immediately if fn put/removed another entry */
            return MAP_ERR;
        }
    });
    return MAP_OK;
}

/*
 * Remove an element with that key from the map
 */
map_ret_code map_remove(map_t *m, const char *key) {
    MAP_PARAM_ASSERT(key);
    MOD_RET_ASSERT(map_length(m) > 0, MAP_MISSING);
    
    map_elem *entry = hashmap_entry_find(m, key, false);
    if (!entry) {
        return MAP_MISSING;
    }
    clear_elem(m, entry);
    return MAP_OK;
}

/* Remove all elements from map */
map_ret_code map_clear(map_t *m) {
    MAP_PARAM_ASSERT(m);
    
    for (map_itr_t *itr = map_itr_new(m); itr; itr = map_itr_next(itr)) {
        map_itr_remove(itr);
    }
    return MAP_OK;
}

/* Deallocate the hashmap (it clears it too) */
map_ret_code map_free(map_t *m) {
    map_ret_code ret = map_clear(m);
    if (ret == MAP_OK) {
        memhook._free(m->table);
        memhook._free(m);
    }
    return ret;
}

/* Return the length of the hashmap */
ssize_t map_length(const map_t *m) {
    MAP_PARAM_ASSERT(m);
    
    return m->length;
}
