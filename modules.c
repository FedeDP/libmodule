#include "modules.h" 
// #include <string.h>
#include <stdlib.h>

/* Modules states */
enum module_states { IDLE, DISABLED, STARTED, DESTROYED };

/* Struct that holds data for each module */
typedef struct {
    void (*init)(void);                   // module's init function
    int (*check)(void);                   // module's check-before-init 
    int (*stateChange)(void);             // module's check-before-init function
    void (*destroy)(void);                // module's destroy function
    void (*pollCb)(void);                 // module's poll callback
    enum module_states state;             // module's state
    self_t self;                          // module's info available to external world
} module;

static void evaluate_new_state(void);
static int is_started(int idx);

static module *modules = NULL;
static int num_modules = 0;

static __attribute__((constructor))  void modules_init(void) {
    printf("Initializing libmodules library.\n");
}

static __attribute__((destructor))  void modules_destroy(void) {
    printf("Destroying libmodules library.\n");
    free(modules);
}

self_t *modules_set_self(const char *name, mod_void_cb init, mod_bool_cb check, 
                      mod_bool_cb state_change, mod_void_cb callback, mod_void_cb destroy) {
    int idx = num_modules;
    modules = realloc(modules, (idx + 1) * sizeof(module));
    modules[idx].init = init;
    modules[idx].check = check;
    modules[idx].stateChange = state_change;
    modules[idx].pollCb = callback;
    modules[idx].destroy = destroy;
    modules[idx].state = IDLE;
    modules[idx].self.name = name; // strdup needed here?
    modules[idx].self.id = idx;
    printf("%d) My name is: %s\n", modules[idx].self.id, modules[idx].self.name);
    num_modules++;
    
    return &modules[idx].self;
}

void modules_loop() {
    // poll...
    
//     evaluate_new_state();
}

static void evaluate_new_state(void) {
    for (int i = 0; i < num_modules; i++) {
        if (!is_started(i) && modules[i].stateChange()) {
            modules[i].init();
        }
    }
}

static int is_started(int idx) {
    return modules[idx].state == STARTED;
}
