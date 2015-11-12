cmake_minimum_required (VERSION 2.8)
project (nanocube_registry)


#
# add local scripts
#
list(APPEND CMAKE_MODULE_PATH
${CMAKE_CURRENT_SOURCE_DIR}/CMake/Packages
)


#
# Check C++11
#
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
if(NOT COMPILER_SUPPORTS_CXX11)
       message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

if(APPLE)
   set(CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libc++" CACHE STRING "compile flags" FORCE)
   set(CMAKE_LINKER_FLAGS "-stdlib=libc++" CACHE STRING "compile flags" FORCE)
   set(CMAKE_OSX_DEPLOYMENT_TARGET "10.11" CACHE STRING "Deployment target for OSX" FORCE)
endif(APPLE)

if(UNIX)
   set(CMAKE_CXX_FLAGS "-std=c++11" CACHE STRING "compile flags" FORCE)
endif(UNIX)

#
# nanomsg
#
find_package(nanomsg REQUIRED)
include_directories(${NANOMSG_INCLUDE_DIR})
message("---- nanomsg ----")
message("NANOMSG_INCLUDE_DIR: ${NANOMSG_INCLUDE_DIR}")
message("NANOMSG_LIBRARY:     ${NANOMSG_LIBRARY}")


add_executable(nanocube-registry registry.cc utf8.cc)

target_link_libraries(nanocube-registry PUBLIC ${NANOMSG_LIBRARY})

if(UNIX)
    target_link_libraries(nanocube-registry PUBLIC -lpthread)
endif(UNIX)


install_targets(/bin nanocube-registry)

