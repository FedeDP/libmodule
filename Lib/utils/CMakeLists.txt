cmake_minimum_required(VERSION 3.0)

file(GLOB SRCS_utils_internal Lib/utils/*.c)

add_library(${PROJECT_NAME}_utils_internal ${LIBRARY_TYPE} ${SRCS_utils_internal})
set_target_properties(
    ${PROJECT_NAME}_utils_internal PROPERTIES 
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

install(TARGETS ${PROJECT_NAME}_utils_internal
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
