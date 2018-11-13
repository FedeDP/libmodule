include (FindPackageHandleStandardArgs)

find_package(Threads)

find_path(KQUEUE_INCLUDE_DIRS sys/event.h PATH_SUFFIXES kqueue)

find_library(_KQUEUE_LIB "kqueue")
mark_as_advanced(_KQUEUE_LIB)
set(KQUEUE_LIBRARIES "${_KQUEUE_LIB}" ${CMAKE_THREAD_LIBS_INIT}
    CACHE STRING "Libraries to link libkqueue")

find_package_handle_standard_args(kqueue DEFAULT_MSG
    KQUEUE_LIBRARIES
    KQUEUE_INCLUDE_DIRS
    THREADS_FOUND
)
