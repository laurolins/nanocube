set(CMAKE_INCLUDE_CURRENT_DIR on)
include_directories(../alloc)

add_library(hierarchy_core STATIC hierarchy.cc nested_hierarchy.cc)
