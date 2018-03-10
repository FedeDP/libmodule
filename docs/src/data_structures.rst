Libmodule Data Structures
=========================

.. code::
    
    /* Possible module's states */
    enum module_states { IDLE, RUNNING, PAUSED };

    /* Message received in poll callback */
    typedef struct {
        int fd;
        const char *message;                    // field unused for now
        const char *sender;                     // field unused for now
    } message_t;

    /* Callbacks typedefs */
    typedef void(*error_cb)(const char *error_msg, const char *ctx);
    typedef int(*init_cb)(void);
    typedef int(*evaluate_cb)(void);
    typedef void(*recv_cb)(message_t *msg, const void *userdata);
    typedef void(*destroy_cb)(void);

    /* Struct that holds user defined callbacks */
    typedef struct {
        init_cb init;                           // module's init function (should return a FD)
        evaluate_cb evaluate;                   // module's state changed function
        recv_cb recv;                           // module's recv function
        destroy_cb destroy;                     // module's destroy function
    } userhook;

    
