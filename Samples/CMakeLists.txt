cmake_minimum_required (VERSION 3.5)

file(GLOB EASY_SRC Easy/*.c)
file(GLOB SHAREDSRC_SRC SharedSrc/*.c)
file(GLOB POLL_SRC Poll/*.c)
file(GLOB TASK_SRC Task/*.c)
file(GLOB THPOOL_SRC Thpool/*.c)

add_executable(Easy ${EASY_SRC})
add_executable(SharedSrc ${SHAREDSRC_SRC})
add_executable(Poll ${POLL_SRC})
add_executable(Task ${TASK_SRC})
add_executable(Thpool ${THPOOL_SRC})

add_library(testmod SHARED testModule.c)
target_link_libraries(testmod ${PROJECT_NAME}_core)

target_include_directories(testmod PRIVATE ${PROJECT_SOURCE_DIR}/Lib/core/public/ ${PROJECT_SOURCE_DIR}/Lib/structs/public/)

target_link_libraries(Easy ${PROJECT_NAME}_core)
target_include_directories(Easy PRIVATE ${PROJECT_SOURCE_DIR}/Lib/core/public/ ${PROJECT_SOURCE_DIR}/Lib/structs/public/ ${PROJECT_SOURCE_DIR}/Lib/mem/public/)

target_link_libraries(SharedSrc ${PROJECT_NAME}_core)
target_include_directories(SharedSrc PRIVATE ${PROJECT_SOURCE_DIR}/Lib/core/public/ ${PROJECT_SOURCE_DIR}/Lib/structs/public/ ${PROJECT_SOURCE_DIR}/Lib/mem/public/)

target_link_libraries(Poll ${PROJECT_NAME}_core)
target_include_directories(Poll PRIVATE ${PROJECT_SOURCE_DIR}/Lib/core/public/ ${PROJECT_SOURCE_DIR}/Lib/structs/public/ ${PROJECT_SOURCE_DIR}/Lib/mem/public/)

target_link_libraries(Task ${PROJECT_NAME}_core)
target_include_directories(Task PRIVATE ${PROJECT_SOURCE_DIR}/Lib/core/public/ ${PROJECT_SOURCE_DIR}/Lib/structs/public/ ${PROJECT_SOURCE_DIR}/Lib/mem/public/)

target_link_libraries(Thpool ${PROJECT_NAME}_core)
target_include_directories(Thpool PRIVATE ${PROJECT_SOURCE_DIR}/Lib/core/public/ ${PROJECT_SOURCE_DIR}/Lib/structs/public/ ${PROJECT_SOURCE_DIR}/Lib/mem/public/ ${PROJECT_SOURCE_DIR}/Lib/thpool/public/)
