cmake_minimum_required(VERSION 3.10)
project(raptor)

set(CMAKE_CXX_STANDARD 11)

set(CATCH_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/catch)
add_library(Catch INTERFACE)
target_include_directories(Catch INTERFACE ${CATCH_INCLUDE_DIR})

file(MAKE_DIRECTORY ${CATCH_INCLUDE_DIR}/build)

include_directories(src)
add_subdirectory(src)
add_subdirectory(tests)
