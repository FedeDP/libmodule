cmake_minimum_required(VERSION 3.0)

file(GLOB PUBLIC_H_structs Lib/structs/public/module/structs/*.h)
file(GLOB SRCS_structs Lib/structs/*.c)

add_library(${PROJECT_NAME}_structs ${LIBRARY_TYPE} ${SRCS_structs})
set_target_properties(
    ${PROJECT_NAME}_structs PROPERTIES 
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${PUBLIC_H_structs}"
    C_VISIBILITY_PRESET hidden
)
target_link_libraries(${PROJECT_NAME}_structs PRIVATE ${PROJECT_NAME}_utils_internal)
target_include_directories(${PROJECT_NAME}_structs PRIVATE Lib/utils/ Lib/structs/)

fill_pc_vars(structs "Libmodule data structures library")
configure_file(Extra/libmodule.pc.in libmodule_structs.pc @ONLY)

install(TARGETS ${PROJECT_NAME}_structs
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/module/structs/
)

install(FILES ${CMAKE_BINARY_DIR}/libmodule_structs.pc
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
