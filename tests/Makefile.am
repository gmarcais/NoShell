#
# Run tests
#
AM_CPPFLAGS = -Wall -I$(top_srcdir)/include $(GTEST_CFLAGS) $(GTESTMAIN_CFLAGS)
LDFLAGS = $(GTEST_LIBS) $(GTESTMAIN_LIBS)

AM_DEFAULT_SOURCE_EXT=.cc
EXTRA_DIST = libtest_misc.hpp

# Helper programs
check_LTLIBRARIES =  libtest_misc.la
check_PROGRAMS = kill_self puts_to check_open_fd

# test programs
TESTS = test_fd_type test_simple_command test_cmd_redirection test_pipeline	\
        test_extra_fds test_literal test_error test_resources
check_PROGRAMS += $(TESTS)
LDADD = ../libnoshell.la  libtest_misc.la


clean-local: clean-local-check
.PHONY: clean-local-check
clean-local-check:
	rm -f *_tmp

# CMake support
EXTRA_DIST += CMakeLists.txt
