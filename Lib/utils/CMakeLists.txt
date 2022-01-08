cmake_minimum_required(VERSION 3.0)

file(GLOB SRCS Lib/utils/*.c)

add_library(${PROJECT_NAME}_utils_internal ${LIBRARY_TYPE} ${SRCS})
set_target_properties(
    ${PROJECT_NAME}_utils_internal PROPERTIES 
    PREFIX ""
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)
target_include_directories(${PROJECT_NAME}_utils_internal PRIVATE)

install(TARGETS ${PROJECT_NAME}_utils_internal
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
