ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = -Wall -I$(top_srcdir)/include
AM_CXXFLAGS = -O3

# Build library
lib_LTLIBRARIES = libnoshell.la
libnoshell_la_SOURCES = lib/noshell.cc lib/setters.cc lib/utils.cc

# Install headers
basedir = $(includedir)/noshell-@PACKAGE_VERSION@
subdir = $(basedir)/noshell
dist_base_HEADERS = include/noshell.hpp
INCDIR = include/noshell
dist_sub_HEADERS = $(INCDIR)/noshell.hpp $(INCDIR)/handle.hpp	\
                   $(INCDIR)/setters.hpp $(INCDIR)/utils.hpp

# Install pc file
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = noshell.pc
