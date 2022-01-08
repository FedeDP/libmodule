cmake_minimum_required(VERSION 3.0)

file(GLOB PUBLIC_H Lib/structs/public/module/structs/*.h)
file(GLOB SRCS Lib/structs/*.c)

add_library(${PROJECT_NAME}_structs ${LIBRARY_TYPE} ${SRCS})
set_target_properties(
    ${PROJECT_NAME}_structs PROPERTIES 
    PREFIX ""
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${PUBLIC_H}"
    C_VISIBILITY_PRESET hidden
)
target_link_libraries(${PROJECT_NAME}_structs PUBLIC ${PROJECT_NAME}_utils_internal)
target_include_directories(${PROJECT_NAME}_structs PRIVATE Lib/utils/ Lib/structs/)

install(TARGETS ${PROJECT_NAME}_structs
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/module/structs/
)
