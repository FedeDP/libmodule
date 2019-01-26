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
    typedef stack_ret_code(*stack_dtor)(void *);

    /* Incomplete struct declaration for stack */
    typedef struct _stack stack_t;

API
---

Where not specified, these functions return a stack_ret_code.

.. c:function:: stack_new()

  Create a new stack_t object.
    
  :returns: pointer to newly allocated stack_t.
  
.. c:function:: stack_iterate(s, fn, userptr)

  Iterate a stack calling cb on each element until STACK_OK is returned (or end of stack is reached). Returns STACK_MISSING if stack is NULL.

  :param s: pointer to stack_t
  :param fn: callback to be called
  :param userptr: userdata to be passed to callback as first parameter
  :type s: :c:type:`stack_t *`
  :type fn: :c:type:`const stack_cb`
  :type userptr: :c:type:`void *`
  
.. c:function:: stack_push(s, val, autofree)

  Push a value on top of stack. Note that if autofree is true, data willbe automatically freed when calling stack_free() on the stack.

  :param s: pointer to stack_t
  :param val: value to be put inside stack
  :param autofree: whether to autofree val upon stack_pop/stack_clear/stack_free
  :type s: :c:type:`stack_t *`
  :type val: :c:type:`void *`
  :type autofree: :c:type:`const bool`

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

  Clears a stack object by deleting any object inside stack, and eventually freeing it too if marked with autofree.

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

.. c:function:: stack_set_dtor(s, fn)

  Set a function to be called upon data deletion for autofree elements.

  :param s: pointer to stack_t
  :param fn: pointer dtor callback
  :type s: :c:type:`stack_t *`
  :type fn: :c:type:`stack_dtor`
