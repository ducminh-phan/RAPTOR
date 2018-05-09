cmake_minimum_required(VERSION 3.10)
project(raptor)

set(CMAKE_CXX_STANDARD 11)

add_executable(raptor src/main.cpp src/data_structure.hpp src/csv_reader.hpp src/utilities.cpp src/utilities.hpp src/data_structure.cpp)
