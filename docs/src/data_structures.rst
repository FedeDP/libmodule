Libmodule Data Structures
=========================

.. code::
    
    /* Modules states */
    enum module_states { IDLE, RUNNING, PAUSED, STOPPED };

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
    typedef int(*get_fd_cb)(void);
    typedef int(*evaluate_cb)(void);
    typedef void(*recv_cb)(msg_t *msg, const void *userdata);
    typedef void(*destroy_cb)(void);

    /* Struct that holds user defined callbacks */
    typedef struct {
        get_fd_cb get_fd;                       // module's get_fd function (should return a FD)
        evaluate_cb evaluate;                   // module's state changed function
        recv_cb recv;                           // module's recv function
        destroy_cb destroy;                     // module's destroy function
    } userhook;
