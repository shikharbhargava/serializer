# Create the test executable
add_executable(tests tests.cpp)

# Link to gtest and calculator
target_link_libraries(tests
  gtest
  gtest_main
  json
  KeyValueParser
  ${OpenCV_LIBS}
)

# Optionally set include directories if needed
target_include_directories(tests PRIVATE
  ${gtest_SOURCE_DIR}/include
  ${gtest_SOURCE_DIR}
)

# Register with CTest
include(GoogleTest)
gtest_discover_tests(tests)
