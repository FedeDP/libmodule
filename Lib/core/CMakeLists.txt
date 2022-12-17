cmake_minimum_required (VERSION 3.3.2)

include(GNUInstallDirs)

# Find sources
file(GLOB SOURCES Lib/core/*.c)

# Internal deps
include(Lib/utils/CMakeLists.txt)
include(Lib/structs/CMakeLists.txt)
include(Lib/thpool/CMakeLists.txt)
include(Lib/mem/CMakeLists.txt)

option(WITH_FS "build ${PROJECT_NAME} with libfuse support" OFF)
if (WITH_FS)
    find_package(PkgConfig)
    pkg_check_modules(FUSE REQUIRED fuse3)
    set(SOURCES ${SOURCES} Lib/core/fs/fs.c)
else()
    set(SOURCES ${SOURCES} Lib/core/fs/fs_noop.c)
endif()

# Set requested poll plugin
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    option(WITH_LIBURING "build ${PROJECT_NAME} with liburing support" OFF)
    option(WITH_LIBKQUEUE "build ${PROJECT_NAME} with libkqueue support" OFF)
    add_compile_definitions(_GNU_SOURCE)
else()
    include(CheckFunctionExists)
    check_function_exists(kqueue KQUEUE_IN_LIBC)
    if(NOT KQUEUE_IN_LIBC)
        set(WITH_LIBKQUEUE true)
    else()
        set(WITH_KQUEUE true)
    endif()
endif()

if(WITH_LIBURING)
    find_package(PkgConfig)
    pkg_check_modules(URING REQUIRED liburing)
    set(SOURCES ${SOURCES} Lib/core/poll/uring.c Lib/core/poll/cmn_linux.c)
    set(POLL_PLUGIN "liburing")
elseif(WITH_LIBKQUEUE)
    find_package(PkgConfig)
    pkg_check_modules(KQUEUE REQUIRED libkqueue)
    set(SOURCES ${SOURCES} Lib/core/poll/kqueue.c)
    set(POLL_PLUGIN "libkqueue")
elseif(WITH_KQUEUE)
    set(SOURCES ${SOURCES} Lib/core/poll/kqueue.c)
    set(POLL_PLUGIN "kqueue")
else()
    set(SOURCES ${SOURCES} Lib/core/poll/epoll.c Lib/core/poll/cmn_linux.c)
    set(POLL_PLUGIN "epoll")
endif()

# Configure headers
configure_file(Lib/core/public/module/cmn.h.in ${PROJECT_SOURCE_DIR}/Lib/core/public/module/cmn.h @ONLY)
if(WITH_FS)
    configure_file(Lib/core/public/module/fs.h.in ${PROJECT_SOURCE_DIR}/Lib/core/public/module/fs.h @ONLY)
endif()

file(GLOB PUBLIC_H Lib/core/public/module/*.h)

add_library(${PROJECT_NAME}_core ${LIBRARY_TYPE} ${SOURCES})

set_target_properties(
    ${PROJECT_NAME}_core PROPERTIES 
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${PUBLIC_H}"
    C_VISIBILITY_PRESET hidden
)

target_include_directories(${PROJECT_NAME}_core PRIVATE Lib/core/ Lib/core/public/ Lib/core/fs/ Lib/core/poll/ Lib/utils/ Lib/structs/ Lib/structs/public/ Lib/mem/ Lib/mem/public/ Lib/thpool/)
target_include_directories(${PROJECT_NAME}_core PUBLIC ${URING_INCLUDE_DIRS} ${KQUEUE_INCLUDE_DIRS} ${FUSE_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME}_core PUBLIC ${URING_LIBRARIES} ${KQUEUE_LIBRARIES} ${FUSE_LIBRARIES} ${PROJECT_NAME}_structs ${PROJECT_NAME}_thpool ${PROJECT_NAME}_mem dl)
target_link_libraries(${PROJECT_NAME}_core PRIVATE ${PROJECT_NAME}_utils_internal)
target_link_directories(${PROJECT_NAME}_core PUBLIC ${URING_LIBRARY_DIRS} ${KQUEUE_LIBRARY_DIRS} ${FUSE_LIBRARY_DIRS})
target_compile_definitions(${PROJECT_NAME}_core PRIVATE LIBMODULE_LOG_CTX=CORE)

fill_pc_vars(${PROJECT_NAME}_core "Libmodule core library")
configure_file(Extra/libmodule.pc.in libmodule_core.pc @ONLY)

install(TARGETS ${PROJECT_NAME}_core
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/module
)

install(FILES ${CMAKE_BINARY_DIR}/libmodule_core.pc
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)

install(FILES LICENSE
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/licenses/lib${PROJECT_NAME})

# Packaging support
# THANKS to libkqueue CMakeLists.txt for packaging support :)
SET(CPACK_SET_DESTDIR "on")
set(CPACK_PACKAGE_NAME lib${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

# Metadata common to all packaging systems
set(CPACK_PACKAGE_CONTACT "Federico Di Pierro <nierro92@gmail.com>")
set(CPACK_PACKAGE_DESCRIPTION "Libmodule offers an elegant and simple implementation of an actor library, aiming at letting developers easily create modular C projects.")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Simple and elegant implementation of an actor library for C.")

# RPM Specific configuration
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_URL "https://github.com/FedeDP/libmodule")
set(CPACK_RPM_PACKAGE_GROUP "System Environment/Libraries")
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "${CMAKE_INSTALL_DATAROOTDIR}/licenses ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig")
if(WITH_FS)
    set(CPACK_RPM_PACKAGE_REQUIRES "fuse3-libs")
endif()
if(WITH_LIBURING)
    set(CPACK_RPM_PACKAGE_REQUIRES "liburing")
elseif(WITH_LIBKQUEUE)
    set(CPACK_RPM_PACKAGE_REQUIRES "libkqueue")
endif()
set(CPACK_RPM_PACKAGE_DESCRIPTION ${CPACK_PACKAGE_DESCRIPTION})
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)

# DEB Specific configuration
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/FedeDP/libmodule")
set(CPACK_DEBIAN_PACKAGE_SECTION "libs")
if(WITH_FS)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libfuse3-3")
endif()
if(WITH_LIBURING)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "liburing")
elseif(WITH_LIBKQUEUE)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libkqueue0")
endif()
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

set(CPACK_GENERATOR DEB RPM TGZ)

include(CPack)
