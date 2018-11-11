.. |br| raw:: html

   <br />

Libmodule Map API
==============

Libmodule offers an easy to use hashmap implementation. |br|
It is used internally to store context's modules and modules' subscriptions/topics. |br|

Module Map structures
---------------------

.. code::

    typedef enum {
        MAP_ERR = -4,
        MAP_MISSING,
        MAP_FULL,
        MAP_OMEM,
        MAP_OK
    } module_map_code;

    /* Callback for module_map_iterate */
    typedef module_map_code (*map_cb)(void *, void *);

    /* Incomplete struct declaration for hashmap */
    typedef struct _map map_t;

Module Map API
--------------

Where not specified, these functions return a module_map_code.

.. c:function:: module_map_new()

  Create a new map_t object.
    
  :returns: pointer to newly allocated map_t.
  
.. c:function:: module_map_iterate(m, f, item)

  Iterate an hashmap.

  :param m: pointer to map_t
  :param f: callback to be called
  :param item: userdata to be passed to callback as second parameter
  :type m: :c:type:`map_t *`
  :type f: :c:type:`const map_cb`
  :type item: :c:type:`void *`
  
.. c:function:: module_map_put(m, key, val)

  Put a value inside hashmap.

  :param m: pointer to map_t
  :param key: key for this value
  :param val: value to be put inside map
  :type m: :c:type:`map_t *`
  :type key: :c:type:`const char *`
  :type val: :c:type:`void *`

.. c:function:: module_map_get(m, key, arg)

  Get an hashmap value.

  :param m: pointer to map_t
  :param key: key for this value
  :param arg: pointer where to store data pointer extracted from map
  :type m: :c:type:`map_t *`
  :type key: :c:type:`const char *`
  :type arg: :c:type:`void **`
  
.. c:function:: module_map_remove(m, key)

  Remove a key from hashmap.

  :param m: pointer to map_t
  :param key: key to be removed
  :type m: :c:type:`map_t *`
  :type key: :c:type:`const char *`
  
.. c:function:: module_map_free(m)

  Free a map object.

  :param m: pointer to map_t
  :type m: :c:type:`map_t *`
  
.. c:function:: module_map_length(m)

  Get map length.

  :param m: pointer to map_t
  :type m: :c:type:`map_t *`
  :returns: map length or a module_map_code if any error happens (map_t is null).
