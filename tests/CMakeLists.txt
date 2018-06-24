cmake_minimum_required(VERSION 3.0)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(CMocka REQUIRED)

file(GLOB SOURCES *.c)

include_directories("${PROJECT_SOURCE_DIR}/Lib/public")
add_executable(ModuleTest ${SOURCES})
add_dependencies(ModuleTest ModuleTest)
add_test(ModuleTest ModuleTest)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    find_package(Valgrind REQUIRED)
    add_test(ModuleTest_valgrind valgrind
         --error-exitcode=1 --read-var-info=yes
         --leak-check=full --show-leak-kinds=all
         ./ModuleTest)
endif()

target_link_libraries(ModuleTest cmocka module)
