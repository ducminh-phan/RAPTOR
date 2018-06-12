cmake_minimum_required(VERSION 3.10)
project(raptor)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "-Wall")
set(CMAKE_CXX_EXTENSIONS OFF)


include_directories(include)
add_executable(raptor
        src/main.cpp
        src/data_structure.cpp src/data_structure.hpp
        src/raptor.cpp src/raptor.hpp)
target_link_libraries(raptor z)
