cmake_minimum_required (VERSION 3.3.2)

project(module VERSION 1.0.0 LANGUAGES C DESCRIPTION "Easily create modular C programs")

set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_BUILD_TYPE Release)

include(GNUInstallDirs)

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    enable_testing()
    add_subdirectory(tests)
    message(STATUS "Building tests.")
endif (${CMAKE_BUILD_TYPE} STREQUAL "Debug")

if (BUILD_SAMPLES)
     add_subdirectory(Samples)
     message(STATUS "Building examples.")
endif (BUILD_SAMPLES)

configure_file(Extra/libmodule.pc.in libmodule.pc @ONLY)
configure_file(Lib/public/module_cmn.h.in ${PROJECT_SOURCE_DIR}/Lib/public/module_cmn.h @ONLY)

# Find source files
file(GLOB SOURCES Lib/*.c)
if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(SOURCES ${SOURCES} Lib/poll_plugins/epoll_priv.c)
else()
    set(SOURCES ${SOURCES} Lib/poll_plugins/kqueue_priv.c)
endif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")

message(STATUS "Building Libmodule ${PROJECT_VERSION} for ${CMAKE_SYSTEM_NAME}.")

# Find public headers
file(GLOB PUBLIC_H Lib/public/*.h)

# Include header files
include_directories(Lib Lib/public)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

set_target_properties(
    ${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "${PUBLIC_H}"
)

# build with -DMAX_EVENTS=X to overwrite the default value of 512
# MAX_EVENTS are max number of fds for each context.
if (NOT MAX_EVENTS)
    set(MAX_EVENTS 512)
endif (NOT MAX_EVENTS)
message(STATUS "MAX_EVENTS=${MAX_EVENTS}")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wtype-limits -Wstrict-overflow -fno-strict-aliasing -Wformat -Wformat-security") 
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -D_GNU_SOURCE -fvisibility=hidden -DMAX_EVENTS=${MAX_EVENTS}")

target_include_directories(${PROJECT_NAME} PRIVATE Lib/ Lib/public/)

install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})

install(FILES ${CMAKE_BINARY_DIR}/libmodule.pc
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
