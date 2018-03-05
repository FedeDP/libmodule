#include <modules.h>

MODULE(A);

void init(void) {
    
}

int check(void) {
    return 0;
}

int state_change(void) {
    return 0;
}

void destroy(void) {
    
}

void callback(void) {
    
}

void test_A() {
    printf("%s: %d\n", self->name, self->id);
}
