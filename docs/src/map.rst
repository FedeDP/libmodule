.. |br| raw:: html

   <br />

Map API
=======

Libmodule offers an easy to use hashmap implementation, provided by <module/map.h> header. |br|
It is used internally to store context's modules and modules' subscriptions/topics. |br|

Structures
----------

.. code::

    typedef enum {
        MAP_EPERM = -6,
        MAP_WRONG_PARAM,
        MAP_ERR,
        MAP_MISSING,
        MAP_FULL,
        MAP_OMEM,
        MAP_OK
    } map_ret_code;

    /* Callback for map_iterate */
    typedef map_ret_code (*map_cb)(void *, const char *, void *);

    /* Fn for map_set_dtor */
    typedef void (*map_dtor)(void *);

    /* Incomplete struct declaration for hashmap */
    typedef struct _map map_t;

    /* Incomplete struct declaration for hashmap iterator */
    typedef struct _map_itr map_itr_t;

API
---

Where not specified, these functions return a map_ret_code.

.. c:function:: map_new(const bool keysdup, const map_dtor fn)

  Create a new map_t object.
  
  :param keysdup: whether keys lifetime should be managed by map
  :param fn: callback called on value destroy. If NULL, values won't be automatically destroyed.
  :type keysdup: :c:type:`const bool`
  :type fn: :c:type:`const map_dtor`
    
  :returns: pointer to newly allocated map_t.
  
.. c:function:: map_itr_new(m)

  Create a new map iterator object. Note that it can be freed with free().
  
  :param m: pointer to map_t
  :type m: :c:type:`const map_t *`
  
  :returns: pointer to newly allocated map_itr_t.
  
.. c:function:: map_itr_next(itr)

  Get next iterator. If next iterator is past last element, iterator will be automatically freed for you.
  
  :param itr: pointer to map_itr_t
  :type itr: :c:type:`map_itr_t *`
  
  :returns: pointer to next iterator.
  
.. c:function:: map_itr_remove(itr)

  Remove current iterator {key, value} from map.
  
  :param itr: pointer to map_itr_t
  :type itr: :c:type:`map_itr_t *`
  
.. c:function:: map_itr_get_key(itr)

  Get current iterator's key.
  
  :param itr: pointer to map_itr_t
  :type itr: :c:type:`const map_itr_t *`
    
  :returns: current iterator's key
  
.. c:function:: map_itr_get_data(itr)

  Get current iterator's data.
  
  :param itr: pointer to map_itr_t
  :type itr: :c:type:`const map_itr_t *`
    
  :returns: current iterator's data
  
.. c:function:: map_itr_set_data(itr)

  Set current iterator's data.
  
  :param itr: pointer to map_itr_t
  :type itr: :c:type:`const map_itr_t *`

.. c:function:: map_iterate(m, fn, userptr)

  Iterate an hashmap calling cb on each element until MAP_OK is returned (or end of hashmap is reached). Returns MAP_MISSING if map is NULL or empty. |br|
  If fn() returns a value != MAP_OK, iteration will stop and: if value < MAP_OK, value will be returned, else MAP_OK will be returned.

  :param m: pointer to map_t
  :param fn: callback to be called
  :param userptr: userdata to be passed to callback as first parameter
  :type m: :c:type:`map_t *`
  :type fn: :c:type:`const map_cb`
  :type userptr: :c:type:`void *`
  
.. c:function:: map_put(m, key, val)

  Put a value inside hashmap. |br|
  If key is already present, old value will be updated (and, if dtor is set, destroyed). Note that if new value and old value are the same pointer, nothing will be done.

  :param m: pointer to map_t
  :param key: key for this value
  :param val: value to be put inside map
  :type m: :c:type:`map_t *`
  :type key: :c:type:`const char *`
  :type val: :c:type:`void *`

.. c:function:: map_get(m, key)

  Get an hashmap value.

  :param m: pointer to map_t
  :param key: key for this value
  :type m: :c:type:`map_t *`
  :type key: :c:type:`const char *`
  :returns: void pointer to value, on NULL on error.
  
.. c:function:: map_has_key(m, key)

  Check if key exists in map.

  :param m: pointer to map_t
  :param key: desired key
  :type m: :c:type:`map_t *`
  :type key: :c:type:`const char *`
  :returns: true if key exists, false otherwise.
  
.. c:function:: map_remove(m, key)

  Remove a key from hashmap.

  :param m: pointer to map_t
  :param key: key to be removed
  :type m: :c:type:`map_t *`
  :type key: :c:type:`const char *`
  
.. c:function:: map_clear(m)

  Clears a map object by deleting any object inside map.

  :param s: pointer to map_t
  :type s: :c:type:`map_t *`
  
.. c:function:: map_free(m)

  Free a map object (it internally calls map_clear too).

  :param m: pointer to map_t
  :type m: :c:type:`map_t *`
  
.. c:function:: map_length(m)

  Get map length.

  :param m: pointer to map_t
  :type m: :c:type:`map_t *`
  :returns: map length or a map_ret_code if any error happens (map_t is null).
