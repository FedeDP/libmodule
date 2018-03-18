Libmodule Data Structures
=========================

Types
-----

.. code::
    
    /* Incomplete structure declaration to self handler */
    typedef struct _self self_t;

    /* Modules states */
    enum module_states { IDLE = 0x1, RUNNING = 0x2, PAUSED = 0x4, STOPPED = 0x8 };

    typedef struct {
        const char *topic;
        const char *message;
        const char *sender;
    } pubsub_msg_t;

    typedef struct {
        int fd;
        pubsub_msg_t *message;
    } msg_t;

    /* Callbacks typedefs */
    typedef int(*init_cb)(void);
    typedef int(*evaluate_cb)(void);
    typedef void(*recv_cb)(msg_t *msg, const void *userdata);
    typedef void(*destroy_cb)(void);

    /* Struct that holds user defined callbacks */
    typedef struct {
        init_cb init;                           // module's init function (should return a FD)
        evaluate_cb evaluate;                   // module's state changed function
        recv_cb recv;                           // module's recv function
        destroy_cb destroy;                     // module's destroy function
    } userhook;


.. _module_ret_code:  

Return Codes
------------

.. code::

    typedef enum {
        MOD_WRONG_STATE = -6,
        MOD_NO_PARENT,
        MOD_NO_CTX,
        MOD_NO_MOD,
        MOD_NO_SELF,
        MOD_ERR,
        MOD_OK
    } module_ret_code;
