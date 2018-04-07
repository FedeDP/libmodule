cmake_minimum_required(VERSION 3.0)

find_package(CMocka REQUIRED)

file(GLOB SOURCES *.c)

include_directories("${PROJECT_SOURCE_DIR}/Lib")
add_executable(ModuleTest ${SOURCES})
add_dependencies(ModuleTest ModuleTest)
add_test(ModuleTest ModuleTest)
if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    add_test(ModuleTest_valgrind valgrind
         --error-exitcode=1 --read-var-info=yes
         --leak-check=full --show-leak-kinds=all
         ./ModuleTest)
endif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
target_link_libraries(ModuleTest cmocka module)
