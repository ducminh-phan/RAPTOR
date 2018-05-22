cmake_minimum_required(VERSION 3.10)
project(raptor)

set(CMAKE_CXX_STANDARD 11)


include_directories(include)
add_executable(raptor
        src/main.cpp
        src/csv_reader.hpp
        src/data_structure.cpp src/data_structure.hpp
        src/utilities.cpp src/utilities.hpp
        src/raptor.cpp src/raptor.hpp
        src/bag.cpp src/bag.hpp)
target_link_libraries(raptor z)
