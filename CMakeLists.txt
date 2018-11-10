cmake_minimum_required (VERSION 3.3.2)

project(module VERSION 3.0.1 LANGUAGES C CXX)

if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(GNUInstallDirs)

if(BUILD_TESTS)
    find_package(Cmocka)
    if (CMOCKA_FOUND)
        enable_testing()
        add_subdirectory(tests)
        message(STATUS "Building tests.")
    else()
        message(WARNING "Missing cmocka.")
    endif()
endif()

if(BUILD_SAMPLES)
     add_subdirectory(Samples)
     message(STATUS "Building examples.")
endif()

if(BUILD_DOCS)
    find_package(Sphinx)
    if(SPHINX_FOUND)
        add_subdirectory(docs)
        message(STATUS "Building docs.")
    else()
        message(WARNING "Missing sphinx.")
    endif()
endif()

configure_file(Extra/libmodule.pc.in libmodule.pc @ONLY)
configure_file(Lib/public/module/module_cmn.h.in ${PROJECT_SOURCE_DIR}/Lib/public/module/module_cmn.h @ONLY)

# Find source files
file(GLOB SOURCES Lib/*.c)
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(SOURCES ${SOURCES} Lib/poll_plugins/epoll_priv.c)
else()
    set(SOURCES ${SOURCES} Lib/poll_plugins/kqueue_priv.c)
endif()

message(STATUS "Building Libmodule ${PROJECT_VERSION}-${CMAKE_BUILD_TYPE} for ${CMAKE_SYSTEM_NAME}.")

# Find public headers
file(GLOB PUBLIC_H Lib/public/module/*.h)

# Include private and public headers
include_directories(Lib Lib/public/module)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

set_target_properties(
    ${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${PUBLIC_H}"
)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wtype-limits -Wstrict-overflow -fno-strict-aliasing -Wformat -Wformat-security") 
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -D_GNU_SOURCE -fvisibility=hidden")

target_include_directories(${PROJECT_NAME} PRIVATE Lib/ Lib/public/module/)

install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})

install(FILES ${CMAKE_BINARY_DIR}/libmodule.pc
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)

install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/licenses/libmodule/)
