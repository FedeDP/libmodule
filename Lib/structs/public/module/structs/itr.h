#pragma once

/**
 * Iterator interface to easily iterate over data structs APIs.
 */

#include <module/structs/map.h>
#include <module/structs/list.h>
#include <module/structs/stack.h>
#include <module/structs/queue.h>
#include <module/structs/bst.h>

#define m_itr_new(X) _Generic((X), \
    m_map_t *: m_map_itr_new, \
    const m_map_t *: m_map_itr_new, \
    m_list_t *: m_list_itr_new, \
    const m_list_t *: m_list_itr_new, \
    m_stack_t *: m_stack_itr_new, \
    const m_stack_t *: m_stack_itr_new, \
    m_queue_t *: m_queue_itr_new, \
    const m_queue_t *: m_queue_itr_new, \
    m_bst_t *: m_bst_itr_new, \
    const m_bst_t *: m_bst_itr_new \
    )(X)

#define m_itr_next(X) _Generic((X), \
    m_map_itr_t **: m_map_itr_next, \
    m_list_itr_t **: m_list_itr_next, \
    m_stack_itr_t **: m_stack_itr_next, \
    m_queue_itr_t **: m_queue_itr_next, \
    m_bst_itr_t **: m_bst_itr_next \
    )(X)
    
#define m_itr_get(X) _Generic((X), \
    m_map_itr_t *: m_map_itr_get_data, \
    m_list_itr_t *: m_list_itr_get_data, \
    m_stack_itr_t *: m_stack_itr_get_data, \
    m_queue_itr_t *: m_queue_itr_get_data, \
    m_bst_itr_t *: m_bst_itr_get_data \
    )(X)
    
/* Unavailable for bst API */
#define m_itr_set(X, data) _Generic((X), \
    m_map_itr_t *: m_map_itr_set_data, \
    m_list_itr_t *: m_list_itr_set_data, \
    m_stack_itr_t *: m_stack_itr_set_data, \
    m_queue_itr_t *: m_queue_itr_set_data \
    )(X, data)
    
#define m_itr_rm(X) _Generic((X), \
    m_map_itr_t *: m_map_itr_remove, \
    m_list_itr_t *: m_list_itr_remove, \
    m_stack_itr_t *: m_stack_itr_remove, \
    m_queue_itr_t *: m_queue_itr_remove, \
    m_bst_itr_t *: m_bst_itr_remove \
    )(X)
    
#define m_itr_foreach(X, fn) { \
        size_t m_idx = 0; \
        for (__auto_type m_itr = m_itr_new(X); m_itr; m_itr_next(&m_itr), m_idx++) fn; \
    }

#define m_iterate(X, cb, up) _Generic((X), \
    m_map_t *: m_map_iterate, \
    const m_map_t *: m_map_iterate, \
    m_list_itr_t *: m_list_iterate, \
    const m_list_itr_t *: m_list_iterate, \
    m_stack_itr_t *: m_stack_iterate, \
    const m_stack_itr_t *: m_stack_iterate, \
    m_queue_itr_t *: m_queue_iterate, \
    const m_queue_itr_t *: m_queue_iterate, \
    m_bst_itr_t *: m_bst_iterate, \
    const m_bst_itr_t *: m_bst_iterate \
    )(X, cb, up)
