# Libmodule Tests

Libmodule is unit-tested through cmocka; here you can find all tests.  

* test_module.c tests module.h API
* test_modules.c tests modules.h API

These tests are automatically executed and valgrind tested on travis ci after every commit.  

## Build and test

To build, you need cmocka and valgrind; tests are automatically built when libmodule is built in Debug mode.  
To run, from libmodule/build folder, issue:

    $ ctest -V
