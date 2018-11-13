.. |br| raw:: html

   <br />

Libmodule Map API
==============

Libmodule offers an easy to use hashmap implementation, provided by <module/map.h> header. |br|
It is used internally to store context's modules and modules' subscriptions/topics. |br|

Map structures
--------------

.. code::

    typedef enum {
        MAP_ERR = -4,
        MAP_MISSING,
        MAP_FULL,
        MAP_OMEM,
        MAP_OK
    } map_ret_code;

    /* Callback for module_map_iterate */
    typedef map_ret_code (*map_cb)(void *, void *);

    /* Incomplete struct declaration for hashmap */
    typedef struct _map map_t;

Map API
-------

Where not specified, these functions return a map_ret_code.

.. c:function:: map_new()

  Create a new map_t object.
    
  :returns: pointer to newly allocated map_t.
  
.. c:function:: map_iterate(m, fn, userptr)

  Iterate an hashmap.

  :param m: pointer to map_t
  :param fn: callback to be called
  :param userptr: userdata to be passed to callback as first parameter
  :type m: :c:type:`map_t *`
  :type fn: :c:type:`const map_cb`
  :type userptr: :c:type:`void *`
  
.. c:function:: map_put(m, key, val, dupkey)

  Put a value inside hashmap. Note that if dupkey is true, key will be strdupped; it will also be automatically freed upon deletion.

  :param m: pointer to map_t
  :param key: key for this value
  :param val: value to be put inside map
  :param dupkey: whether to strdup key
  :type m: :c:type:`map_t *`
  :type key: :c:type:`const char *`
  :type val: :c:type:`void *`
  :type dupkey: :c:type:`const bool`

.. c:function:: map_get(m, key)

  Get an hashmap value.

  :param m: pointer to map_t
  :param key: key for this value
  :type m: :c:type:`map_t *`
  :type key: :c:type:`const char *`
  :returns: void pointer to value.
  
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
  
.. c:function:: map_free(m)

  Free a map object.

  :param m: pointer to map_t
  :type m: :c:type:`map_t *`
  
.. c:function:: map_length(m)

  Get map length.

  :param m: pointer to map_t
  :type m: :c:type:`map_t *`
  :returns: map length or a module_map_code if any error happens (map_t is null).
