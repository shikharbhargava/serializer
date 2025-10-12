cmake_minimum_required(VERSION 3.10)

project(main)

# Compiler flags
include(cmake/CompilerFlags.cmake)

if(DEFINED ENV{OPENCV_INSTALL})
  set(OPENCV_PATH $ENV{OPENCV_INSTALL})
  message(STATUS "Found OPENCV PATH from environment variable OPENCV_INSTALL: " ${OPENCV_PATH})
  set(OpenCV_DIR "${OPENCV_PATH}/lib/cmake/opencv4")
endif()

project(parser)

# Find OpenCV (Make sure OpenCV is installed on your system)
find_package(OpenCV REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(include)

add_subdirectory(utils)
add_subdirectory(ArgParser)
add_subdirectory(json)

add_subdirectory(serializer)

add_executable(main main/main.cpp)
target_link_libraries(main serializer)

# Enable testing
enable_testing()

# Add gtest directory (so it can be built as a subproject)
add_subdirectory(deps)

# Add tests
add_subdirectory(tests)
