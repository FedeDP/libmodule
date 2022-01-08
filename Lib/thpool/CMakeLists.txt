cmake_minimum_required(VERSION 3.0)

find_package (Threads)

file(GLOB PUBLIC_H Lib/thpool/public/module/thpool/*.h)
file(GLOB SRCS Lib/thpool/*.c)

add_library(${PROJECT_NAME}_thpool ${LIBRARY_TYPE} ${SRCS})
set_target_properties(
    ${PROJECT_NAME}_thpool PROPERTIES 
    PREFIX ""
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${PUBLIC_H}"
    C_VISIBILITY_PRESET hidden
)
target_link_libraries(${PROJECT_NAME}_thpool PUBLIC ${CMAKE_THREAD_LIBS_INIT} ${PROJECT_NAME}_structs)
target_include_directories(${PROJECT_NAME}_thpool PRIVATE Lib/utils/ Lib/structs/ Lib/thpool/)

install(TARGETS ${PROJECT_NAME}_structs
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/module/thpool/
)
