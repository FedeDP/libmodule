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
