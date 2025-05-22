cmake_minimum_required (VERSION 3.5)

project(module VERSION 6.0.0 LANGUAGES C)

if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wtype-limits -Wstrict-overflow -fno-strict-aliasing -Wformat -Wformat-security -fsanitize=undefined")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

option(BUILD_TESTS "build ${PROJECT_NAME} tests" OFF)
if(BUILD_TESTS)
    find_package(Cmocka)
    if (CMOCKA_FOUND)
        enable_testing()
        add_subdirectory(tests)
        message(STATUS "Tests building enabled.")
    else()
        message(WARNING "Missing cmocka.")
    endif()
endif()

option(BUILD_SAMPLES "build ${PROJECT_NAME} examples" OFF)
if(BUILD_SAMPLES)
     add_subdirectory(Samples)
     message(STATUS "Examples building enabled.")
endif()

include(Lib/CMakeLists.txt)
