cmake_minimum_required (VERSION 3.3.2)

project(module VERSION 1.0.0 DESCRIPTION "Easily create modular C programs")

set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_BUILD_TYPE Release)

include(GNUInstallDirs)

# Find source files
file(GLOB SOURCES Lib/*.c)
set(PUBLIC_H "Lib/module.h Lib/modules.h")
set(PRIVATE_H "Lib/module_priv.h")

# Include header files
include_directories(Lib)

add_library(module SHARED ${SOURCES})

set_target_properties(module PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER ${PUBLIC_H}
    PRIVATE_HEADERS ${PRIVATE_H})

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wl,--no-undefined -Wshadow -Wtype-limits -Wstrict-overflow -fno-strict-aliasing -Wformat -Wformat-security") 
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -D_GNU_SOURCE -fvisibility=hidden")

configure_file(Extra/libmodule.pc.in libmodule.pc @ONLY)

target_include_directories(module PRIVATE Lib/)

install(TARGETS module
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(FILES ${CMAKE_BINARY_DIR}/libmodule.pc
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig) 
    
