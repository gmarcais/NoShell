cmake_minimum_required(VERSION 3.10)
set(project_name noshell)

include(GNUInstallDirs)

set(NOSHELL_MASTER_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(NOSHELL_MASTER_PROJECT ON)
endif()

option(NOSHELL_BUILD_TESTS "Set to ON to build tests" ${NOSHELL_MASTER_PROJECT})
option(NOSHELL_ENABLE_INSTALL "Generate the install target" ${NOSHELL_MASTER_PROJECT})

# get version
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/configure.ac" configure_ac)
string(REGEX MATCH "AC_INIT\\\(\\\[noshell\\\],\\\[(.*)\\\],\\\[gmarcais@cmu.edu\\\]\\\)" _ "${configure_ac}")
if(NOT CMAKE_MATCH_COUNT EQUAL 1)
    message(FATAL_ERROR "Could not extract major version number from configure.ac")
endif()
set(noshell_version ${CMAKE_MATCH_1})
message(STATUS "noshell version: ${noshell_version}")


project(${project_name} VERSION "${noshell_version}" LANGUAGES CXX)


set(NOSHELL_SRCS lib/noshell.cc lib/setters.cc lib/utils.cc)

add_library(${project_name} SHARED ${NOSHELL_SRCS})
add_library(${project_name}-static STATIC ${NOSHELL_SRCS})
add_library(${project_name}::${project_name} ALIAS ${project_name})
add_library(${project_name}::${project_name}-static ALIAS ${project_name}-static)

target_include_directories(${project_name} 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/noshell-${noshell_version}>)
target_include_directories(${project_name}-static
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/noshell-${noshell_version}>)
set_target_properties(${project_name}
    PROPERTIES
        CXX_STANDARD 11)
set_target_properties(${project_name}-static
    PROPERTIES
        CXX_STANDARD 11
        POSITION_INDEPENDENT_CODE 1)

if(NOSHELL_ENABLE_INSTALL)
    set(project_config_in "${CMAKE_CURRENT_LIST_DIR}/noshellConfig.cmake.in")
    set(project_config_out "${CMAKE_CURRENT_BINARY_DIR}/noshellConfig.cmake")
    set(config_targets_file "noshellConfigTargets.cmake")
    set(version_config_file "${CMAKE_CURRENT_BINARY_DIR}/noshellConfigVersion.cmake")
    set(export_dest_dir "${CMAKE_INSTALL_LIBDIR}/cmake/noshell")
    set(pkgconfig_install_dir "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
    set(pkgconfig "${CMAKE_CURRENT_BINARY_DIR}/noshell.pc")

    set(prefix "${CMAKE_INSTALL_PREFIX}")
    set(exec_prefix "\${prefix}")
    if (IS_ABSOLUTE "${CMAKE_INSTALL_LIBDIR}")
        set(libdir "${CMAKE_INSTALL_LIBDIR}")
    else()
        set(libdir "\${exec_prefix}/${CMAKE_INSTALL_LIBDIR}")
    endif()
    if (IS_ABSOLUTE "${CMAKE_INSTALL_INCLUDEDIR}")
        set(includedir "${CMAKE_INSTALL_INCLUDEDIR}")
    else()
        set(includedir "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
    endif()
    set(PACKAGE_VERSION "${noshell_version}")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/noshell.pc.in"
        "${pkgconfig}"
        @ONLY)

    install(FILES "${pkgconfig}"
            DESTINATION "${pkgconfig_install_dir}")

    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/noshell-${noshell_version}")

    install(TARGETS ${project_name} ${project_name}-static
            EXPORT ${project_name}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

    export(
        TARGETS ${project_name}
        NAMESPACE ${project_name}::
        FILE "${CMAKE_CURRENT_BINARY_DIR}/${config_targets_file}")

    install(EXPORT ${project_name}
            DESTINATION ${export_dest_dir} 
            NAMESPACE ${project_name}:: 
            FILE ${config_targets_file})

    include(CMakePackageConfigHelpers)
    configure_package_config_file("${project_config_in}" "${project_config_out}"
        INSTALL_DESTINATION ${export_dest_dir})


    write_basic_package_version_file("${version_config_file}"
                                    VERSION "${noshell_version}"
                                    COMPATIBILITY AnyNewerVersion)

    install(FILES "${project_config_out}" "${version_config_file}" DESTINATION "${export_dest_dir}")
endif(NOSHELL_ENABLE_INSTALL)
        

if(NOSHELL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
