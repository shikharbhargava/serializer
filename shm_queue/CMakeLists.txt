
# Shared interface library
add_library(shm_queue INTERFACE)
target_include_directories(shm_queue INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Conditional compilation for platform-specific source and headers
if(WIN32)
    message(STATUS "Building for Windows")
    set(SHM_PRODUCER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/producer_win.cpp)
    set(SHM_CONSUMER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/consumer_win.cpp)
    target_compile_definitions(shm_queue INTERFACE WIN_32)
elseif(UNIX)
    message(STATUS "Building for Linux/Unix")
    set(SHM_PRODUCER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/producer_linux.cpp)
    set(SHM_CONSUMER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/consumer_linux.cpp)
    target_compile_definitions(shm_queue INTERFACE LINUX)
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

# Executable
add_executable(shm_producer ${SHM_PRODUCER_SRC})
target_link_libraries(shm_producer shm_queue)

add_executable(shm_consumer ${SHM_CONSUMER_SRC})
target_link_libraries(shm_consumer shm_queue)

# If pthreads are needed on Linux
if(UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(shm_producer Threads::Threads)
endif()
