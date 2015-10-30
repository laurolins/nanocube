cmake_minimum_required (VERSION 2.8)
project (nanocube_registry)

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
   set(CMAKE_EXE_LINKER_FLAGS "-stdlib=libc++" CACHE STRING "compile flags" FORCE)
   set(CMAKE_OSX_DEPLOYMENT_TARGET "10.10" CACHE STRING "Deployment target for OSX" FORCE)
endif(APPLE)

if(UNIX)
   set(CMAKE_CXX_FLAGS "-std=c++11" CACHE STRING "compile flags" FORCE)
endif(UNIX)

set(CMAKE_INCLUDE_CURRENT_DIR on)
add_executable(nanocube-registry
Server.cc
mongoose.c
tokenizer.cc
registry.cc)

install_targets(/bin nanocube-registry)