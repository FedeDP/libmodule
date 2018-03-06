#include <module.h>

extern void test_A();
extern void test_B();
extern void test_C();

int main() {
    test_A();
    test_B();
    test_C();
    
    modules_loop();
    return 0;
}
