cmake_minimum_required (VERSION 3.3.2)

option(STATIC_MODULE "build ${PROJECT_NAME} as static library" OFF)
if(STATIC_MODULE)
    set(LIBRARY_TYPE STATIC)
else()
    set(LIBRARY_TYPE SHARED)
endif()

macro(fill_pc_vars target desc)
    set(PKG_TARGET ${target})
    set(PKG_DESC ${desc})
    
    get_target_property(int_link_dirs ${target} INTERFACE_LINK_DIRECTORIES)
    if(NOT int_link_dirs)
        set(PKG_DIRS "")
    else()
        string(REPLACE ";" " -L" PKG_DIRS ";${int_link_dirs}")  # -L flags
    endif()

    get_target_property(int_link_libs ${target} INTERFACE_LINK_LIBRARIES)
    if(NOT int_link_libs)
        string(REPLACE ";" " -l" PKG_DEPS ";${target}")       # -l flags
    else()
        string(REPLACE ";" " -l" PKG_DEPS ";${int_link_libs};${target}")       # -l flags
    endif()
    
    get_target_property(int_incl_dirs ${target} INTERFACE_INCLUDE_DIRECTORIES)
    if(NOT int_incl_dirs)
        set(PKG_INCS "")
    else()
        string(REPLACE ";" " -I" PKG_INCS ";${int_incl_dirs}")        # -I flags
    endif()
endmacro()

include(Lib/core/CMakeLists.txt)

# Some pretty printings
MESSAGE(STATUS "Building ${PROJECT_NAME}-${PROJECT_VERSION}")
MESSAGE(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
MESSAGE(STATUS "Library Type: ${LIBRARY_TYPE}")
MESSAGE(STATUS "Poll plugin: ${POLL_PLUGIN}")
MESSAGE(STATUS "Target OS: ${CMAKE_SYSTEM_NAME}")


option(BUILD_OOT_TEST "build ${PROJECT_NAME} out of tree test (system integration test)" OFF)
if(BUILD_OOT_TEST)
    # Script that tests that pkg-config is able to find libmodule_core
    # and a small binary can be compiled succesfully (Task example)
    install(
    CODE
    "
    message(STATUS \"Testing out of tree build.\")
    set(ENV{PKG_CONFIG_PATH} \"${CMAKE_INSTALL_PREFIX}/share/pkgconfig/\")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PKG_LIBMODULE REQUIRED libmodule_core)
    execute_process(
      COMMAND wget -q -O ${CMAKE_CURRENT_BINARY_DIR}/pippo.c https://raw.githubusercontent.com/FedeDP/libmodule/devel/Samples/Task/pippo.c
    )
    execute_process(
      COMMAND ${CMAKE_C_COMPILER} ${CORE_PKG_DEPS} ${CORE_PKG_INCS} ${CMAKE_CURRENT_BINARY_DIR}/pippo.c -o ${CMAKE_CURRENT_BINARY_DIR}/pippo
      COMMAND_ECHO STDOUT
      RESULT_VARIABLE IS_ERR
    )
    if(IS_ERR)
        message(FATAL_ERROR \"error building the source file.\")
    endif()
    set(ENV{LD_LIBRARY_PATH} \"${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/\")
    execute_process(
      COMMAND ${CMAKE_CURRENT_BINARY_DIR}/pippo
      COMMAND_ECHO STDOUT
      RESULT_VARIABLE IS_ERR
    )
    if(IS_ERR)
        message(FATAL_ERROR \"error running the source file.\")
    else()
        message(STATUS \"SUCCESS: source file built and run.\")
    endif()
    "
    COMPONENT runtime
    )
endif()
