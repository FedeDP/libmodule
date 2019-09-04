.. |br| raw:: html

   <br />

Stack API
=========

Libmodule offers an easy to use stack implementation, provided by <module/stack.h> header. |br|
It is used internally to store module's stack for become/unbecome methods. |br|

Structures
----------

.. code::

    typedef enum {
        STACK_WRONG_PARAM = -4,
        STACK_MISSING,
        STACK_ERR,
        STACK_OMEM,
        STACK_OK
    } stack_ret_code;

    /* Callback for stack_iterate */
    typedef stack_ret_code (*stack_cb)(void *, void *);

    /* Fn for stack_set_dtor */
    typedef void (*stack_dtor)(void *);

    /* Incomplete struct declaration for stack */
    typedef struct _stack stack_t;

    /* Incomplete struct declaration for stack iterator */
    typedef struct _stack_itr stack_itr_t;

API
---

Where not specified, these functions return a stack_ret_code.

.. c:function:: stack_new(fn)

  Create a new stack_t object.
  
  :param fn: callback called on value destroy. If NULL, values won't be automatically destroyed.
  :type fn: :c:type:`const stack_dtor`
    
  :returns: pointer to newly allocated stack_t.
  
.. c:function:: stack_itr_new(s)

  Create a new stack_itr_t object. Note that it can be freed with free().
  
  :param s: pointer to stack_t
  :type s: :c:type:`const stack_t *`
    
  :returns: pointer to newly allocated stack_itr_t.
  
.. c:function:: stack_itr_next(itr)

  Get next iterator. If next iterator is past last element, iterator will be automatically freed.
  
  :param itr: pointer to stack_itr_t
  :type itr: :c:type:`stack_itr_t *`
    
  :returns: pointer to next iterator.
  
.. c:function:: stack_itr_get_data(itr)

  Get iterator's data.
  
  :param itr: pointer to stack_itr_t
  :type itr: :c:type:`const stack_itr_t *`
    
  :returns: pointer to current iterator's data.
  
.. c:function:: stack_itr_set_data(itr)

  Set iterator's data.
  
  :param itr: pointer to stack_itr_t
  :type itr: :c:type:`const stack_itr_t *`

.. c:function:: stack_iterate(s, fn, userptr)

  Iterate a stack calling cb on each element until STACK_OK is returned (or end of stack is reached). Returns STACK_MISSING if stack is NULL or empty. |br|
  If fn() returns a value != STACK_OK, iteration will stop and: if value < STACK_OK, value will be returned, else STACK_OK will be returned.

  :param s: pointer to stack_t
  :param fn: callback to be called
  :param userptr: userdata to be passed to callback as first parameter
  :type s: :c:type:`stack_t *`
  :type fn: :c:type:`const stack_cb`
  :type userptr: :c:type:`void *`
  
.. c:function:: stack_push(s, val)

  Push a value on top of stack.

  :param s: pointer to stack_t
  :param val: value to be put inside stack
  :type s: :c:type:`stack_t *`
  :type val: :c:type:`void *`

.. c:function:: stack_pop(s)

  Pop a value from top of stack, removing it from stack.

  :param s: pointer to stack_t
  :type s: :c:type:`stack_t *`
  :returns: void pointer to value, on NULL on error.
  
.. c:function:: stack_peek(s)

  Return top-of-stack element, without removing it.

  :param s: pointer to stack_t
  :type s: :c:type:`stack_t *`
  :returns: void pointer to value, on NULL on error.

.. c:function:: stack_clear(s)

  Clears a stack object by deleting any object inside stack.

  :param s: pointer to stack_t
  :type s: :c:type:`stack_t *`
  
.. c:function:: stack_free(s)

  Free a stack object (it internally calls stack_clear too).

  :param s: pointer to stack_t
  :type s: :c:type:`stack_t *`
  
.. c:function:: stack_length(s)

  Get stack length.

  :param s: pointer to stack_t
  :type s: :c:type:`stack_t *`
  :returns: stack length or a stack_ret_code if any error happens (stack_t is null).
