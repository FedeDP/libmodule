#pragma once

/**
 * Iterator interface to easily iterate over data structs APIs.
 */

#include "map.h"
#include "list.h"
#include "stack.h"
#include "queue.h"
#include "btree.h"

#define m_itr_new(X) _Generic((X), \
    m_map_t *: map_itr_new, \
    m_list_t *: list_itr_new, \
    m_stack_t *: stack_itr_new, \
    m_queue_t *: queue_itr_new, \
    m_btree_t *: m_btree_itr_new \
    )(X)

#define m_itr_next(X) _Generic((X), \
    m_map_itr_t *: map_itr_next, \
    m_list_itr_t *: list_itr_next, \
    m_stack_itr_t *: stack_itr_next, \
    m_queue_itr_t *: queue_itr_next, \
    m_btree_itr_t *: m_btree_itr_next \
    )(X)
    
#define m_itr_data(X) _Generic((X), \
    m_map_itr_t *: map_itr_get_data, \
    m_list_itr_t *: list_itr_get_data, \
    m_stack_itr_t *: stack_itr_get_data, \
    m_queue_itr_t *: queue_itr_get_data, \
    m_btree_itr_t *: m_btree_itr_get_data \
    )(X)
    
#define m_itr_foreach(X, fn) \
    for (__auto_type itr = m_itr_new(X); itr; itr = m_itr_next(itr)) fn;
