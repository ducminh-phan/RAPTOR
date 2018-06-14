cmake_minimum_required(VERSION 3.10)
project(raptor)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "-Wall")

include_directories(include)
add_executable(raptor
        src/main.cpp
        src/data_structure.cpp src/data_structure.hpp
        src/raptor.cpp src/raptor.hpp
        src/experiments.cpp src/experiments.hpp)
add_executable(gen_query
        gen_query/main.cpp
        gen_query/hub_labelling.cpp gen_query/hub_labelling.hpp)

target_link_libraries(raptor z)
target_link_libraries(gen_query z)
