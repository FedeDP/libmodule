# Libmodule Tests

Libmodule is unit-tested through cmocka; here you can find all tests.  

* test_module.c tests module.h API
* test_modules.c tests modules.h API

These tests are automatically executed and valgrind tested on travis ci after every commit.  

## Build and test

To build tests, you need to issue a:

    $ cmake -DBUILD_TESTS=true ../

To run them, you need cmocka and valgrind, then, from libmodule/build folder, issue:

    $ ctest -V
