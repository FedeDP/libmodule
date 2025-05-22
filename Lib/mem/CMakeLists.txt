cmake_minimum_required (VERSION 3.5)

file(GLOB PUBLIC_H_mem Lib/mem/public/module/mem/*.h)
file(GLOB SRCS_mem Lib/mem/*.c)

add_library(${PROJECT_NAME}_mem ${LIBRARY_TYPE} ${SRCS_mem})
set_target_properties(
    ${PROJECT_NAME}_mem PROPERTIES 
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${PUBLIC_H_mem}"
    C_VISIBILITY_PRESET hidden
)
target_link_libraries(${PROJECT_NAME}_mem PRIVATE ${PROJECT_NAME}_utils_internal)
target_include_directories(${PROJECT_NAME}_mem PRIVATE Lib/utils/ Lib/mem/)
target_compile_definitions(${PROJECT_NAME}_mem PRIVATE LIBMODULE_LOG_CTX=MEM)

fill_pc_vars(${PROJECT_NAME}_mem "Libmodule mem utilities library")
configure_file(Extra/libmodule.pc.in libmodule_mem.pc @ONLY)

install(TARGETS ${PROJECT_NAME}_mem
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/module/mem/
)

install(FILES ${CMAKE_BINARY_DIR}/libmodule_mem.pc
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
