cmake_minimum_required(VERSION 3.10)
project(raptor)

set(CMAKE_CXX_STANDARD 11)

add_executable(raptor src/main.cpp src/parser.cpp src/parser.hpp src/data_structure.hpp src/csv_reader.hpp)