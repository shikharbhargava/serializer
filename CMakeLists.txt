cmake_minimum_required(VERSION 3.12)

# Compiler flags first
include(cmake/CompilerFlags.cmake)

project(serializer VERSION 1.0 LANGUAGES CXX)

# OpenCV
if(DEFINED ENV{OPENCV_INSTALL})
  set(OPENCV_PATH $ENV{OPENCV_INSTALL})
  message(STATUS "Found OPENCV PATH from environment variable OPENCV_INSTALL: ${OPENCV_PATH}")
  set(OpenCV_DIR "${OPENCV_PATH}/lib/cmake/opencv4")
endif()
find_package(OpenCV REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS} include)

# Submodules
add_subdirectory(utils)
add_subdirectory(ArgParser)
add_subdirectory(json)
add_subdirectory(serializer)

# Main executable
add_executable(main main/main.cpp)
target_link_libraries(main serializer ArgParser)

# Enable testing
enable_testing()
add_subdirectory(deps)
add_subdirectory(tests)
