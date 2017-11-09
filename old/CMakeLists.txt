include_directories(../alloc)
include_directories(../hierarchy)

# enable_testing()

add_executable(compressed-nanocube-test-hierarchy test_hierarchy.cc)
target_link_libraries(compressed-nanocube-test-hierarchy alloc_core hierarchy_core gtest gtest_main)