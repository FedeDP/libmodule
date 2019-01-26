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
        MAP_WRONG_PARAM = -5,
        MAP_ERR,
        MAP_MISSING,
        MAP_FULL,
        MAP_OMEM,
        MAP_OK
    } map_ret_code;

    /* Callback for map_iterate */
    typedef map_ret_code (*map_cb)(void *, void *);
    
    /* Fn for map_set_dtor */
    typedef map_ret_code (*map_dtor)(void *);

    /* Incomplete struct declaration for hashmap */
    typedef struct _map map_t;

API
---

Where not specified, these functions return a map_ret_code.

.. c:function:: map_new()

  Create a new map_t object.
    
  :returns: pointer to newly allocated map_t.
  
.. c:function:: map_iterate(m, fn, userptr)

  Iterate an hashmap calling cb on each element until MAP_OK is returned (or end of hashmap is reached). Returns MAP_MISSING if map is NULL.

  :param m: pointer to map_t
  :param fn: callback to be called
  :param userptr: userdata to be passed to callback as first parameter
  :type m: :c:type:`map_t *`
  :type fn: :c:type:`const map_cb`
  :type userptr: :c:type:`void *`
  
.. c:function:: map_put(m, key, val, dupkey, autofree)

  Put a value inside hashmap. Note that if dupkey is true, key will be strdupped and its lifetime will be managed by libmodule.

  :param m: pointer to map_t
  :param key: key for this value
  :param val: value to be put inside map
  :param dupkey: whether to strdup key
  :param autofree: whether to automatically free val upon map_remove/map_clear/map_free
  :type m: :c:type:`map_t *`
  :type key: :c:type:`const char *`
  :type val: :c:type:`void *`
  :type dupkey: :c:type:`const bool`
  :type autofree: :c:type:`const bool`

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

  Clears a map object by deleting any object inside map, and eventually freeing it too if marked with autofree.

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
  
.. c:function:: map_set_dtor(m, fn)

  Set a function to be called upon data deletion for autofree elements.

  :param m: pointer to map_t
  :param fn: pointer dtor callback
  :type m: :c:type:`map_t *`
  :type fn: :c:type:`map_dtor`
