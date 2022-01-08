cmake_minimum_required(VERSION 3.0)

file(GLOB PUBLIC_H Lib/mem/public/module/mem/*.h)
file(GLOB SRCS Lib/mem/*.c)

add_library(${PROJECT_NAME}_mem ${LIBRARY_TYPE} ${SRCS})
set_target_properties(
    ${PROJECT_NAME}_thpool PROPERTIES 
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${PUBLIC_H}"
    C_VISIBILITY_PRESET hidden
)
target_link_libraries(${PROJECT_NAME}_mem PRIVATE ${PROJECT_NAME}_utils_internal)
target_include_directories(${PROJECT_NAME}_mem PRIVATE Lib/utils/ Lib/mem/)

fill_pc_vars(${PROJECT_NAME}_mem "Libmodule mem utilities library")
configure_file(Extra/libmodule.pc.in libmodule_mem.pc @ONLY)

install(TARGETS ${PROJECT_NAME}_structs
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/module/mem/
)

install(FILES ${CMAKE_BINARY_DIR}/libmodule_mem.pc
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
