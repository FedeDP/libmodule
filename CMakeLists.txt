cmake_minimum_required (VERSION 3.3.2)

project(libmodule VERSION 4.1.0 LANGUAGES C CXX)

if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(GNUInstallDirs)

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
target_link_libraries(${PROJECT_NAME} ${KQUEUE_LIBRARIES})

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
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

#
#  Metadata common to all packaging systems
#
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Small and simple implementation of an actor library for C.")
set(CPACK_COMPONENT_MAIN_DESCRIPTION "Libmodule offers a small and simple implementation of an actor library, aiming at letting developers easily create modular C projects in a way which is both simple and elegant.")
set(CPACK_COMPONENT_DEVEL_DESCRIPTION "Development files for ${PROJECT_NAME}-${PROJECT_VERSION}")

#
#  RPM Specific configuration
#
set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0")
set(CPACK_RPM_PACKAGE_URL "https://github.com/FedeDP/libmodule")

set(CPACK_RPM_MAIN_PACKAGE_GROUP "System Environment/Libraries")
set(CPACK_RPM_MAIN_PACKAGE_DESCRIPTION ${CPACK_COMPONENT_MAIN_DESCRIPTION})

set(CPACK_RPM_DEVEL_PACKAGE_GROUP "Development/Libraries")
set(CPACK_RPM_DEVEL_PACKAGE_SUMMARY ${CPACK_COMPONENT_DEVEL_DESCRIPTION})
set(CPACK_RPM_DEVEL_PACKAGE_REQUIRES "${CPACK_PACKAGE_NAME} = %{version}-%{release}")

set(CPACK_RPM_COMPONENT_INSTALL OFF)            # Enable component based packaging (generate multiple packages)
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)            # Use rpmbuild's package naming scheme

#
#  DEB Specific configuration
#
set(CPACK_DEBIAN_MAIN_PACKAGE_NAME "${CPACK_PACKAGE_NAME}")
set(CPACK_DEBIAN_MAIN_PACKAGE_SECTION "libs")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Federico Di Pierro <nierro92@gmail.com>")

set(CPACK_DEBIAN_DEVEL_PACKAGE_NAME "${CPACK_PACKAGE_NAME}-dev")
set(CPACK_DEBIAN_DEVEL_PACKAGE_SECTION "libdevel")
set(CPACK_DEBIAN_DEVEL_PACKAGE_DEPENDS "${CPACK_PACKAGE_NAME} (= ${PROJECT_VERSION})")

set(CPACK_DEB_COMPONENT_INSTALL OFF)            # Enable component based packaging (generate multiple packages)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)         # Use default Debian package naming scheme

include(CPack)
