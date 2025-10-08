cmake_minimum_required (VERSION 3.5)

project(libmodule VERSION 5.0.2 LANGUAGES C CXX)

if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(GNUInstallDirs)

find_package (Threads)

#
# Options
#
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

option(BUILD_DOCS "build ${PROJECT_NAME} docs" OFF)
if(BUILD_DOCS)
    find_package(Sphinx)
    if(SPHINX_FOUND)
        add_subdirectory(docs)
        message(STATUS "Docs building enabled.")
    else()
        message(WARNING "Missing sphinx.")
    endif()
endif()

option(STATIC_MODULE "build ${PROJECT_NAME} as static library" OFF)
if(STATIC_MODULE)
    set(LIBRARY_TYPE STATIC)
else()
    set(LIBRARY_TYPE SHARED)
endif()

#
# Configured files
#
configure_file(Extra/libmodule.pc.in libmodule.pc @ONLY)
configure_file(Lib/public/module/module_cmn.h.in ${PROJECT_SOURCE_DIR}/Lib/public/module/module_cmn.h @ONLY)

#
# Set source files
#
file(GLOB SOURCES Lib/*.c)
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(SOURCES ${SOURCES} Lib/poll_plugins/epoll_priv.c)
else()
    include(CheckFunctionExists)
    check_function_exists(kqueue KQUEUE_IN_LIBC)
    if(NOT KQUEUE_IN_LIBC)
        find_package(Kqueue REQUIRED)
    else()
        set(KQUEUE_LIBRARIES "")
    endif()
    set(SOURCES ${SOURCES} Lib/poll_plugins/kqueue_priv.c)
endif()

#
# Some pretty printings
#
message(STATUS "Building ${PROJECT_NAME}-${PROJECT_VERSION}")
MESSAGE(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
MESSAGE(STATUS "Library Type: ${LIBRARY_TYPE}")
MESSAGE(STATUS "Target OS: ${CMAKE_SYSTEM_NAME}")

file(GLOB PUBLIC_H Lib/public/module/*.h)

include_directories(Lib Lib/public/module)

add_library(${PROJECT_NAME} ${LIBRARY_TYPE} ${SOURCES})

set_target_properties(
    ${PROJECT_NAME} PROPERTIES 
    PREFIX ""
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${PUBLIC_H}"
)

target_include_directories(${PROJECT_NAME} PRIVATE Lib/ Lib/public/module/)

#
# KQUEUE_LIBRARIES will be empty where native epoll/kqueue are supported
#
target_link_libraries(${PROJECT_NAME} ${KQUEUE_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} dl)

install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/module
    )

install(FILES ${CMAKE_BINARY_DIR}/libmodule.pc
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)

install(FILES LICENSE
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/licenses/${PROJECT_NAME})

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wtype-limits -Wstrict-overflow -fno-strict-aliasing -Wformat -Wformat-security")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -fvisibility=hidden")

#
# Packaging support
# THANKS to libkqueue CMakeLists.txt for packaging support :)
#
SET(CPACK_SET_DESTDIR "on")
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

#
#  Metadata common to all packaging systems
#
set(CPACK_PACKAGE_CONTACT "Federico Di Pierro <nierro92@gmail.com>")
set(CPACK_PACKAGE_DESCRIPTION "Libmodule offers a small and simple implementation of an actor library, aiming at letting developers easily create modular C projects in a way which is both simple and elegant.")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Small and simple implementation of an actor library for C.")

#
#  RPM Specific configuration
#
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_URL "https://github.com/FedeDP/libmodule")
set(CPACK_RPM_PACKAGE_GROUP "System Environment/Libraries")
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "${CMAKE_INSTALL_DATAROOTDIR}/licenses ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig")
set(CPACK_RPM_PACKAGE_DESCRIPTION ${CPACK_PACKAGE_DESCRIPTION})
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)

#
#  DEB Specific configuration
#
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/FedeDP/libmodule")
set(CPACK_DEBIAN_PACKAGE_SECTION "libs")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

include(CPack)
