cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(CMocka REQUIRED)

file(GLOB SOURCES *.c)

include_directories("${PROJECT_SOURCE_DIR}/Lib/public")
add_executable(ModuleTest ${SOURCES})
add_dependencies(ModuleTest ModuleTest)
add_test(ModuleTest ModuleTest)

find_package(Valgrind)
if(VALGRIND_FOUND)
    message(STATUS "Tests will be checked with valgrind.")
    add_test(ModuleTest_valgrind valgrind
         --error-exitcode=1 --read-var-info=yes
         --leak-check=full --show-leak-kinds=all
         ./ModuleTest)
else()
    message(STATUS "Tests won't be checked with valgrind.")
endif()

target_include_directories(ModuleTest PRIVATE ${CMOCKA_INCLUDE_DIR})
target_link_libraries(ModuleTest ${CMOCKA_LIBRARIES} ${PROJECT_NAME})
