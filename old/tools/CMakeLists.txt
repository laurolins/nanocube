set(CMAKE_INCLUDE_CURRENT_DIR on)
include_directories(../alloc)
include_directories(../hierarchy)
# include_directories(../../thirdparty)
include_directories(../../thirdparty)

add_executable (compressed-nanocube-main main.cc)
target_link_libraries(compressed-nanocube-main alloc_core hierarchy_core)

add_executable (compressed-nanocube-dmp-hierarchy tool_dmp_hierarchy.cc)
target_link_libraries(compressed-nanocube-dmp-hierarchy alloc_core hierarchy_core)

add_executable (test_normalization test_normalization.cc)

install_targets(/bin test_normalization compressed-nanocube-dmp-hierarchy)
