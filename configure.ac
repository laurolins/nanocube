#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.65])
AC_INIT([nanocube], [0.0.1], [lauro.lins@gmail.com])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([aux-scripts])

AM_INIT_AUTOMAKE([foreign])

AC_PROG_CC
AC_PROG_CPP
AC_PROG_LIBTOOL

# Checks for programs.
host_os=`uname`
if test "$host_os" = "Darwin"; then
   AS_VAR_SET([cpp_compiler],[clang++])
   CXX="clang++ "
   CXXFLAGS+=" -stdlib=libc++ -mmacosx-version-min=10.7"
else
   AS_VAR_SET([cpp_compiler],[])
fi

# echo "----------------> $CC"

AC_PROG_CXX([AS_VAR_GET([cpp_compiler])])

LT_LIB_DLLOAD
LT_INIT([dlopen])

AC_LANG([C++])

# turn on c++11
AX_CXX_COMPILE_STDCXX_11(noext)

# check zlib
AX_CHECK_ZLIB

#check boost
BOOST_REQUIRE([1.48])
BOOST_THREADS
BOOST_SYSTEM
BOOST_ASIO

#BOOST_REQUIRE([1.48])
#BOOST_THREADS
#BOOST_SERIALIZATION
#BOOST_MPI

TS_CHECK_TCMALLOC

# Checks for header files.
AC_CHECK_HEADERS([stdint.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 scripts/Makefile])
AC_OUTPUT
