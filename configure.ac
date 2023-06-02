AC_INIT([noshell],[0.2.0],[gmarcais@cmu.edu])
AC_CANONICAL_HOST
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([subdir-objects foreign parallel-tests color-tests])
AM_SILENT_RULES([yes])
AC_CONFIG_SRCDIR([lib])
AC_CONFIG_HEADERS([config.h])
LT_INIT

# Change default compilation flags
# CXXFLAGS="-std=c++0x $CXXFLAGS"
AC_LANG(C++)
AC_PROG_CXX

# Find gtest
PKG_CHECK_MODULES([GTEST], [gtest])
PKG_CHECK_MODULES([GTESTMAIN], [gtest_main])

AC_CONFIG_FILES([Makefile tests/Makefile noshell.pc])
AC_OUTPUT
