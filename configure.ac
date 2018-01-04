AC_PREREQ([2.69])
AC_INIT([test], [0.0.1], [lauro.lins@gmail.com])
AC_CONFIG_SRCDIR([src/app.c])
AC_CONFIG_HEADERS([config.h])

# added mannually into the output of autoscan
AC_CONFIG_AUX_DIR([ac_aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])


# 
# AC_ARG_ENABLE(option-name, help-string, action-if-present, action-if-not-present)
# AC_ARG_WITH([polycover], AS_HELP_STRING([--without-foo], [Ignore presence of foo and disable it]))
#
AC_ARG_WITH([polycover], 
	    [AS_HELP_STRING([--with-polycover], [polycover; needs c++ and its stdlib dependencies])],
	    [],
	    [with_polycover=no])

# add an option
AS_IF([test "x$with_polycover" != "xno"],
      [ AM_CONDITIONAL(WITH_POLYCOVER, true)  ], 
      [ AM_CONDITIONAL(WITH_POLYCOVER, false) ])

###############################################################
# AC_CANONICAL_HOST is needed to access the 'host_os' variable
###############################################################
AC_CANONICAL_HOST
build_linux=no
build_windows=no
build_mac=no
# Detect the target system
case "${host_os}" in
linux*)
        build_linux=yes ;;
cygwin*|mingw*)
        build_windows=yes ;;
darwin*)
        build_mac=yes ;;
*)
        AC_MSG_ERROR(["OS $host_os is not supported"]) ;;
esac
AM_CONDITIONAL([LINUX], [test "$build_linux" = "yes"])
AM_CONDITIONAL([OSX], [test "$build_mac" = "yes"])
AM_CONDITIONAL([WINDOWS], [test "$build_windows" = "yes"])


# echo "........................OS: $build_linux"

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
# the macro AC_PROG_CPP seems to find the default compiler (gcc or clang)
#AC_PROG_CPP
AM_PROG_AR
AC_PROG_LIBTOOL

LT_LIB_DLLOAD
# LT_INIT([dlopen])
LT_INIT


# echo "****************************************** --with-polycover=$with_polycover"

# include this only if the --with-polycover option is set
# AM_COND_IF([WITH_POLYCOVER], [AC_PROG_CXX AC_LANG([C++]) AX_CXX_COMPILE_STDCXX_11(noext)], [])
# AS_IF([test "x$with_polycover" != "xno"], [ ], [ ])

AC_PROG_CXX 
AC_LANG([C++]) 
AX_CXX_COMPILE_STDCXX_11(noext)
AC_LANG([C]) 


# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([limits.h malloc.h stddef.h stdlib.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset sqrt])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

