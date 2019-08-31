Data Structures
===============

Macros
------

.. code::

    #define MODULE_VERSION_MAJ @PROJECT_VERSION_MAJOR@
    #define MODULE_VERSION_MIN @PROJECT_VERSION_MINOR@
    #define MODULE_VERSION_PAT @PROJECT_VERSION_PATCH@
    
    #define MODULE_DEFAULT_CTX  "default"
    #define MODULE_MAX_EVENTS   64

Types
-----

.. code::
    
    /* Incomplete structure declaration to self handler */
    typedef struct _self self_t;

    /* Modules states */
    enum module_states { IDLE = 0x1, RUNNING = 0x2, PAUSED = 0x4, STOPPED = 0x8 };

    /* PubSub message types */
    enum msg_type { USER, LOOP_STARTED, LOOP_STOPPED, MODULE_STARTED, MODULE_STOPPED, MODULE_POISONPILL };

    typedef struct {
        const char *topic;
        const void *message;
        ssize_t size;
        const self_t *sender;
        enum msg_type type;
    } ps_msg_t;

    typedef struct {
        int fd;
        const void *userptr;
    } fd_msg_t;

    typedef struct {
        bool is_pubsub;
        union {
            const fd_msg_t      *fd_msg;
            const ps_msg_t      *ps_msg;
        };
    } msg_t;

    /* Callbacks typedefs */
    typedef void (*init_cb)(void);
    typedef bool (*evaluate_cb)(void);
    typedef void (*recv_cb)(const msg_t *const msg, const void *userdata);
    typedef void (*destroy_cb)(void);

    /* Logger callback */
    typedef void (*log_cb)(const self_t *self, const char *fmt, va_list args, const void *userdata);

    /* Memory management user-passed functions */
    typedef void *(*malloc_fn)(size_t size);
    typedef void *(*realloc_fn)(void *ptr, size_t size);
    typedef void *(*calloc_fn)(size_t nmemb, size_t size);
    typedef void (*free_fn)(void *ptr);

    /* Struct that holds user defined callbacks */
    typedef struct {
        init_cb init;                           // module's init function (should return a FD)
        evaluate_cb evaluate;                   // module's state changed function
        recv_cb recv;                           // module's recv function
        destroy_cb destroy;                     // module's destroy function
    } userhook_t;

    /* Struct that holds user defined memory functions */
    typedef struct {
        malloc_fn _malloc;
        realloc_fn _realloc;
        calloc_fn _calloc;
        free_fn _free;
    } memhook_t;

.. _module_ret_code:  

Return Codes
------------

.. code::

    typedef enum {
        MOD_WRONG_PARAM = -8,
        MOD_NO_MEM,
        MOD_WRONG_STATE,
        MOD_NO_PARENT,
        MOD_NO_CTX,
        MOD_NO_MOD,
        MOD_NO_SELF,
        MOD_ERR,
        MOD_OK
    } module_ret_code;
