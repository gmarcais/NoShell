cmake_minimum_required(VERSION 3.10)
project(noshell_tests CXX)

if(NOT TARGET noshell)
    # Stand-alone build
    find_package(noshell REQUIRED)
endif()

list(APPEND NOSHELL_TESTS_HELPER
    check_open_fd.cc
    kill_self.cc
    puts_to.cc)

list(APPEND NOSHELL_TESTS_LIST
    libtest_misc.cc
    test_cmd_redirection.cc
    test_error.cc
    test_extra_fds.cc
    test_fd_type.cc
    test_literal.cc
    test_pipeline.cc
    test_resources.cc
    test_simple_command.cc)

find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

foreach(src ${NOSHELL_TESTS_HELPER})
    GET_FILENAME_COMPONENT(target_name ${src} NAME_WE)
    add_executable(${target_name} ${src})
endforeach()

add_executable(noshell_tests ${NOSHELL_TESTS_LIST})
target_link_libraries(noshell_tests GTest::Main GTest::GTest Threads::Threads noshell::noshell)

add_test(noshell_tests noshell_tests)
