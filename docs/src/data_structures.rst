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
    typedef enum { IDLE = 0x1, RUNNING = 0x2, PAUSED = 0x4 } mod_states;

    /* PubSub message types */
    typedef enum { USER, LOOP_STARTED, LOOP_STOPPED, MODULE_STARTED, MODULE_STOPPED, MODULE_POISONPILL } ps_msg_type;

    /* Module event sources' flags */
    typedef enum { FD_AUTOCLOSE = 0x1, PS_DUPTOPIC = 0x2, PS_AUTOFREE = 0x04 } mod_src_flags;

    typedef struct {
        const char *topic;
        const void *message;
        ssize_t size;
        const self_t *sender;
        ps_msg_type type;
    } ps_msg_t;

    typedef struct {
        int fd;
    } fd_msg_t;

    typedef struct {
        bool is_pubsub;
        union {
            const fd_msg_t      *fd_msg;
            const ps_msg_t      *ps_msg;
        };
    } msg_t;

    /* Callbacks typedefs */
    typedef bool (*init_cb)(void);
    typedef bool (*evaluate_cb)(void);
    typedef void (*recv_cb)(const msg_t *const msg, const void *userdata);
    typedef void (*destroy_cb)(void);

    /* Logger callback */
    typedef void (*log_cb)(const self_t *self, const char *fmt, va_list args);

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

.. _mod_ret:  

Return Codes
------------

.. code::

    /* Module return codes */
    typedef enum {
        MOD_REF_ERR = -9,
        MOD_WRONG_PARAM,
        MOD_NO_MEM,
        MOD_WRONG_STATE,
        MOD_NO_PARENT,
        MOD_NO_CTX,
        MOD_NO_MOD,
        MOD_NO_SELF,
        MOD_ERR,
        MOD_OK
    } mod_ret;
