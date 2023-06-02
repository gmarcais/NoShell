cmake_minimum_required(VERSION 3.10)
project(noshell VERSION 0.2.0 LANGUAGES CXX)

file(GLOB NOSHELL_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/lib/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/noshell/*.hpp")

add_library(noshell STATIC ${NOSHELL_SRCS})
add_library(noshell::noshell ALIAS noshell)
target_include_directories(noshell 
    PUBLIC 
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
set_target_properties(noshell
    PROPERTIES
      CXX_STANDARD 11)