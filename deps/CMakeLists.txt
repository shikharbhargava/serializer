set(gtest_force_shared_crt ON CACHE BOOL "" FORCE) # Ensure gtest uses /MD
add_subdirectory(googletest)
