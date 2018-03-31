cmake_minimum_required (VERSION 3.3.2)

project(module VERSION 1.0.0 DESCRIPTION "Easily create modular C programs")

set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_BUILD_TYPE Release)

include(GNUInstallDirs)

# Find source files
file(GLOB SOURCES Lib/*.c)
set(PUBLIC_H Lib/module.h Lib/modules.h)

# Include header files
include_directories(Lib)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
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
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wl,--no-undefined -Wshadow -Wtype-limits -Wstrict-overflow -fno-strict-aliasing -Wformat -Wformat-security") 
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -D_GNU_SOURCE -fvisibility=hidden -DMAX_EVENTS=${MAX_EVENTS}")

configure_file(Extra/libmodule.pc.in libmodule.pc @ONLY)

target_include_directories(${PROJECT_NAME} PRIVATE Lib/)

install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})

install(FILES ${CMAKE_BINARY_DIR}/libmodule.pc
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
