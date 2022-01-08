cmake_minimum_required (VERSION 3.3.2)

option(STATIC_MODULE "build ${PROJECT_NAME} as static library" OFF)
if(STATIC_MODULE)
    set(LIBRARY_TYPE STATIC)
else()
    set(LIBRARY_TYPE SHARED)
endif()

include(Lib/core/CMakeLists.txt)

# Some pretty printings
MESSAGE(STATUS "Building ${PROJECT_NAME}-${PROJECT_VERSION}")
MESSAGE(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
MESSAGE(STATUS "Library Type: ${LIBRARY_TYPE}")
MESSAGE(STATUS "Poll plugin: ${POLL_PLUGIN}")
MESSAGE(STATUS "Target OS: ${CMAKE_SYSTEM_NAME}")
