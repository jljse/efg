#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([FULL-PACKAGE-NAME], [VERSION], [BUG-REPORT-ADDRESS])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([compiler/p_node.cpp])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
#AC_PROG_YACC
AC_CHECK_PROG(BISON,bison,bison)
AC_PROG_CXX
AC_PROG_CC
#AC_PROG_LEX
AC_CHECK_PROG(YACC,yacc,yacc)
AC_PROG_MAKE_SET
AC_PROG_RANLIB

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_REALLOC

AC_CONFIG_FILES([compiler/Makefile
                 runtime/Makefile])
AC_OUTPUT
