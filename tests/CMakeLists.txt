cmake_minimum_required(VERSION 3.0)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(CMocka REQUIRED)

file(GLOB SOURCES *.c)

add_executable(ModuleTest ${SOURCES})
add_dependencies(ModuleTest ModuleTest)
add_test(ModuleTest ModuleTest)

option(WITH_VALGRIND "check tests with valgrind" ON)
if(WITH_VALGRIND)
    find_package(Valgrind)
    if(VALGRIND_FOUND)
        message(STATUS "Tests will be checked with valgrind.")
        add_test(ModuleTest_valgrind valgrind
             --error-exitcode=1 --read-var-info=yes
             --leak-check=full --show-leak-kinds=all
             ./ModuleTest)
    else()
        message(STATUS "Valgrind unavailable. Tests won't be checked with it.")
    endif()
endif()

target_include_directories(ModuleTest PRIVATE ${CMOCKA_INCLUDE_DIR} ${PROJECT_SOURCE_DIR}/Lib/core/public ${PROJECT_SOURCE_DIR}/Lib/structs/public ${PROJECT_SOURCE_DIR}/Lib/thpool/public ${PROJECT_SOURCE_DIR}/Lib/mem/public)
target_link_libraries(ModuleTest ${CMOCKA_LIBRARIES} ${PROJECT_NAME}_core)
