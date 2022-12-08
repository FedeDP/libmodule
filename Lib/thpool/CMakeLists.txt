cmake_minimum_required(VERSION 3.1)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

file(GLOB PUBLIC_H_thpool Lib/thpool/public/module/thpool/*.h)
file(GLOB SRCS_thpool Lib/thpool/*.c)

add_library(${PROJECT_NAME}_thpool ${LIBRARY_TYPE} ${SRCS_thpool})
set_target_properties(
    ${PROJECT_NAME}_thpool PROPERTIES 
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${PUBLIC_H_thpool}"
    C_VISIBILITY_PRESET hidden
)
target_link_libraries(${PROJECT_NAME}_thpool PUBLIC ${CMAKE_THREAD_LIBS_INIT} ${PROJECT_NAME}_structs)
target_link_libraries(${PROJECT_NAME}_thpool PRIVATE ${PROJECT_NAME}_utils_internal)
target_include_directories(${PROJECT_NAME}_thpool PRIVATE Lib/utils/ Lib/structs/ Lib/structs/public/ Lib/thpool/)
target_compile_definitions(${PROJECT_NAME}_thpool PRIVATE LIBMODULE_LOG_CTX=THPOOL)

fill_pc_vars(${PROJECT_NAME}_thpool "Libmodule thread pool library")
# avoid "-l-pthread" string
string(REPLACE "-l-pthread" "-pthread" PKG_DEPS ${PKG_DEPS})
configure_file(Extra/libmodule.pc.in libmodule_thpool.pc @ONLY)

install(TARGETS ${PROJECT_NAME}_thpool
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/module/thpool/
)

install(FILES ${CMAKE_BINARY_DIR}/libmodule_thpool.pc
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
